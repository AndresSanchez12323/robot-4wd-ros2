"""Publica la cámara y verifica objetivos visuales en dos observaciones."""

import base64
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
import os
from pathlib import Path
from threading import Condition
from urllib.request import Request, urlopen

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import CompressedImage
from std_msgs.msg import String


MODELO = "qwen3-vl-2b"
LIMITE_JPEG = 2_000_000
EXPLORAR = ("Identifica el objeto fijo más claro que pueda ser relevante para circular, "
            "como columna, silla o mesa. Nombra solo uno con un sustantivo singular en español. "
            "Si no se distingue uno, indica que no está visible.")
ESQUEMA = {
    "type": "object",
    "properties": {
        "objeto": {"type": "string"},
        "relacion": {"enum": ["izquierda", "derecha", "frente", "indeterminada"]},
        "visible": {"type": "boolean"},
        "evidencia": {"type": "string"},
    },
    "required": ["objeto", "relacion", "visible", "evidencia"],
    "additionalProperties": False,
}


def fotografiar(url):
    with urlopen(url + "/captura", timeout=2) as respuesta:
        datos = respuesta.read(LIMITE_JPEG + 1)
        if (respuesta.headers.get_content_type() != "image/jpeg"
                or not 4 <= len(datos) <= LIMITE_JPEG
                or not datos.startswith(b"\xff\xd8") or not datos.endswith(b"\xff\xd9")):
            raise ValueError("La cámara no devolvió un JPEG completo")
        return datos


def interpretar(url, objetivo, imagen, previa=None):
    datos = {"instruccion": objetivo}
    if previa is not None:
        datos["observacion_anterior"] = previa
        datos["tarea"] = "Revisa la respuesta anterior con la imagen nueva y corrígela si hace falta."
    solicitud = {
        "model": MODELO,
        "stream": False,
        "response_format": {"type": "json_schema", "schema": ESQUEMA},
        "temperature": 0,
        "max_tokens": 220,
        "messages": [
            {"role": "system", "content": (
                "Interpreta la instrucción y busca el objeto de referencia en la imagen. "
                "Relación es el lado solicitado respecto al objeto, desde la vista inicial. "
                "Si el objeto falta o hay varios candidatos indistinguibles, marca visible=false. "
                "Describe solo evidencia visual, sin estimar distancias ni afirmar que existe una ruta."
            )},
            {"role": "user", "content": [
                {"type": "text", "text": json.dumps(datos, ensure_ascii=False)},
                {"type": "image_url", "image_url": {
                    "url": "data:image/jpeg;base64," + base64.b64encode(imagen).decode("ascii")}},
            ]},
        ],
    }
    peticion = Request(url + "/v1/chat/completions", data=json.dumps(solicitud).encode(),
                       headers={"Content-Type": "application/json"})
    with urlopen(peticion, timeout=75) as respuesta:
        resultado = json.loads(json.load(respuesta)["choices"][0]["message"]["content"])
    if (not isinstance(resultado, dict) or resultado.keys() != ESQUEMA["properties"].keys()
            or type(resultado["visible"]) is not bool
            or resultado["relacion"] not in ESQUEMA["properties"]["relacion"]["enum"]
            or any(not isinstance(resultado[campo], str) or not resultado[campo].strip()
                   for campo in ("objeto", "evidencia"))):
        raise ValueError("Respuesta del modelo fuera del esquema")
    return resultado


def guardar_objeto(ruta, resultado):
    inicial, final = resultado["inicial"], resultado["revisada"]
    etiqueta = " ".join(final["objeto"].casefold().split())
    if (not inicial["visible"] or not final["visible"] or
            etiqueta != " ".join(inicial["objeto"].casefold().split()) or
            not etiqueta or len(etiqueta) > 64):
        return None
    try:
        memoria = json.loads(ruta.read_text(encoding="utf-8"))
    except FileNotFoundError:
        memoria = {}
    ahora = datetime.now(timezone.utc).isoformat()
    registro = {"objeto": etiqueta, "observaciones": memoria.get(etiqueta, {}).get("observaciones", 0) + 1,
                "ultima_observacion": ahora, "evidencia": final["evidencia"][:400]}
    memoria[etiqueta] = registro
    if len(memoria) > 128:
        del memoria[min(memoria, key=lambda nombre: memoria[nombre]["ultima_observacion"])]
    temporal = ruta.with_suffix(".tmp")
    temporal.write_text(json.dumps(memoria, ensure_ascii=False) + "\n", encoding="utf-8")
    temporal.replace(ruta)
    return registro


class Observador(Node):
    def __init__(self):
        super().__init__("observador_robot")
        self.robot = os.environ.get("ROBOT_URL", "http://192.168.4.1").rstrip("/")
        self.modelo = os.environ.get("MODELO_URL", "http://modelo:8080").rstrip("/")
        self.camara = self.create_publisher(
            CompressedImage, "/camera/image_raw/compressed", qos_profile_sensor_data)
        self.informe = self.create_publisher(String, "/robot/vision/revision", 10)
        self.objetos = self.create_publisher(String, "/robot/aprendizaje/objetos", 10)
        self.memoria = Path(os.environ.get("OBJETOS_JSON", "/datos/objetos.json"))
        self.create_subscription(String, "/robot/vision/instruccion", self.recibir, 10)
        self.pool = ThreadPoolExecutor(max_workers=1)
        self.condicion = Condition()
        self.secuencia = 0
        self.imagen = b""
        self.trabajo = None
        self.cierre = False
        self.create_timer(1.0, self.publicar_camara)
        self.create_timer(0.2, self.publicar_revision)
        self.create_timer(60.0, self.explorar)
        self.explorar()

    def explorar(self):
        if self.trabajo is None and not self.cierre:
            self.trabajo = self.pool.submit(self.revisar, EXPLORAR)

    def publicar_camara(self):
        try:
            imagen = fotografiar(self.robot)
        except (OSError, ValueError) as error:
            self.get_logger().warning(f"Cámara sin captura: {error}", throttle_duration_sec=10)
            return
        mensaje = CompressedImage()
        mensaje.header.stamp = self.get_clock().now().to_msg()
        mensaje.header.frame_id = "camera_optical_frame"
        mensaje.format = "jpeg"
        mensaje.data = imagen
        self.camara.publish(mensaje)
        with self.condicion:
            self.imagen = imagen
            self.secuencia += 1
            self.condicion.notify_all()

    def siguiente_captura(self):
        with self.condicion:
            anterior = self.secuencia
            recibida = self.condicion.wait_for(
                lambda: self.cierre or self.secuencia > anterior, timeout=6)
            if self.cierre or not recibida:
                raise TimeoutError("No llegó una captura nueva")
            return self.imagen, self.secuencia

    def recibir(self, mensaje):
        objetivo = mensaje.data.strip()
        if not 0 < len(objetivo) <= 500:
            return
        if self.trabajo is not None:
            self.get_logger().warning("Hay una revisión visual en curso")
            return
        self.trabajo = self.pool.submit(self.revisar, objetivo)

    def revisar(self, objetivo):
        primera, captura_inicial = self.siguiente_captura()
        inicial = interpretar(self.modelo, objetivo, primera)
        segunda, captura_final = self.siguiente_captura()
        final = interpretar(self.modelo, objetivo, segunda, inicial)
        campos = ("objeto", "relacion", "visible")
        hubo_cambio = any(inicial[campo] != final[campo] for campo in campos)
        return {
            "instruccion": objetivo, "capturas": [captura_inicial, captura_final],
            "inicial": inicial, "revisada": final, "hubo_cambio": hubo_cambio,
            "estado": "observado" if final["visible"] and final["relacion"] != "indeterminada"
            else "sin_evidencia",
        }

    def publicar_revision(self):
        if self.trabajo is None or not self.trabajo.done():
            return
        try:
            resultado = self.trabajo.result()
        except (OSError, ValueError, TypeError, KeyError, IndexError) as error:
            resultado = {"estado": "error", "detalle": str(error)}
        self.trabajo = None
        self.informe.publish(String(data=json.dumps(resultado, ensure_ascii=False)))
        self.get_logger().info(json.dumps(resultado, ensure_ascii=False))
        if resultado.get("instruccion") == EXPLORAR:
            try:
                registro = guardar_objeto(self.memoria, resultado)
                if registro:
                    self.objetos.publish(String(data=json.dumps(registro, ensure_ascii=False)))
            except (OSError, ValueError, TypeError, KeyError, AttributeError) as error:
                self.get_logger().error(f"No se pudo guardar la memoria visual: {error}")

    def cerrar(self):
        with self.condicion:
            self.cierre = True
            self.condicion.notify_all()
        self.pool.shutdown(wait=False, cancel_futures=True)


def main():
    rclpy.init()
    nodo = Observador()
    try:
        rclpy.spin(nodo)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        nodo.cerrar()
        nodo.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
