"""Orquesta la mision. Si un servicio no responde, no manda mover el carro."""

import base64
import json
import os
import sys
import threading
import time
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request, urlopen

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from comun.servicio import Base, leer_json, responder, servir, token_valido
from orquestador.logica import anotar, evaluar, llegada, nombre_visto, pistas_de, resumen_lidar, robot_util, seguir_busqueda

TOKEN = os.environ.get("TOKEN", "")
URLS = {
    "llm": os.environ.get("LLM_URL", "").rstrip("/"),
    "vlm": os.environ.get("VLM_URL", "").rstrip("/"),
    "vla": os.environ.get("VLA_URL", "").rstrip("/"),
    "gpu": os.environ.get("GPU_URL", "").rstrip("/"),
    "robot": os.environ.get("ROBOT_URL", "").rstrip("/"),
}
EXIGIR_GPU = os.environ.get("EXIGIR_GPU", "1") == "1"
PUERTO = int(os.environ.get("PUERTO", "8080"))
PASOS = int(os.environ.get("PASOS_POR_LUGAR", "40"))
GIRO_SEGUNDOS = 0.35
AVANCE_SEGUNDOS = 0.8
ACERCAR_SEGUNDOS = 1.2
RAIZ = Path(__file__).resolve().parent
CATALOGO = json.loads((RAIZ / "catalogo.json").read_text(encoding="utf-8"))


def viene_del_gateway(handler):
    return handler.headers.get("X-Entrada") == "graphql"

candado = threading.Lock()
en_curso = threading.Event()
mision = {
    "estado": "quieto",
    "detalle": "Esperando una frase",
    "lugares": [],
    "nombres": [],
    "lugar": "",
    "accion": "",
    "servicios": {},
}
detener = threading.Event()
bitacora = []


def _get(url, ruta, timeout=2.5):
    with urlopen(url + ruta, timeout=timeout) as respuesta:
        bruto = respuesta.read(2_100_000)
        tipo = respuesta.headers.get_content_type()
        return tipo, bruto


def _json(url, ruta, cuerpo=None, token=False, timeout=2.5):
    datos = json.dumps(cuerpo).encode() if cuerpo is not None else None
    cabeceras = {"Accept": "application/json"}
    if cuerpo is not None:
        cabeceras["Content-Type"] = "application/json"
    if token:
        cabeceras["Authorization"] = "Bearer " + TOKEN
    solicitud = Request(url + ruta, data=datos, headers=cabeceras, method="POST" if cuerpo is not None else "GET")
    try:
        with urlopen(solicitud, timeout=timeout) as respuesta:
            return json.loads(respuesta.read(1_000_000).decode())
    except HTTPError as exc:
        try:
            return json.loads(exc.read(8000).decode())
        except (OSError, ValueError, json.JSONDecodeError):
            return None


def _salud_http(url):
    try:
        return _json(url, "/salud", timeout=2.5)
    except (OSError, ValueError, json.JSONDecodeError):
        return None


def _estado_robot():
    try:
        _tipo, bruto = _get(URLS["robot"], "/estado", 2)
        estado = json.loads(bruto.decode())
    except (OSError, ValueError, json.JSONDecodeError):
        return None
    if not estado.get("operador"):
        time.sleep(0.4)
        try:
            _tipo, bruto = _get(URLS["robot"], "/estado", 2)
            estado = json.loads(bruto.decode())
        except (OSError, ValueError, json.JSONDecodeError):
            return None
    return estado


def revisar():
    servicios = {nombre: _salud_http(url) for nombre, url in URLS.items() if nombre != "robot"}
    if not EXIGIR_GPU:
        servicios.pop("gpu", None)
    estado = _estado_robot()
    servicios["robot"] = {
        "sano": robot_util(estado),
        "detalle": "carro listo" if robot_util(estado) else "carro sin permiso o sin enlace",
    }
    return evaluar(servicios), servicios, estado


def _publicar(**cambios):
    with candado:
        mision.update(cambios)
        anotar(bitacora, mision)


def _orden_robot(accion):
    ruta = "/mision?accion=" + accion
    solicitud = Request(URLS["robot"] + ruta, data=b"", method="POST", headers={"X-Robot": "1"})
    with urlopen(solicitud, timeout=2) as respuesta:
        respuesta.read(200)


def _parar_robot():
    try:
        _orden_robot("parar")
    except (OSError, ValueError):
        return


def _captura():
    tipo, bruto = _get(URLS["robot"], "/captura", 5)
    if tipo != "image/jpeg" or not 4 <= len(bruto) <= 1_200_000 or not bruto.startswith(b"\xff\xd8"):
        raise ValueError("captura")
    return base64.b64encode(bruto).decode("ascii")


def _esperar_quieto():
    limite = time.monotonic() + 2.0
    while time.monotonic() < limite:
        if detener.is_set():
            return True
        estado = _estado_robot()
        if estado and not estado.get("activa") and not estado.get("moviendo"):
            time.sleep(0.15)
            return True
        time.sleep(0.1)
    return False


def _latir(fin):
    while not fin.wait(0.4):
        try:
            _estado_robot()
        except (OSError, ValueError, json.JSONDecodeError):
            continue


def _ciclo(texto):
    detener.clear()
    with candado:
        bitacora.clear()
    _publicar(detalle="Frase: " + texto[:200], lugar="", accion="")
    fin = threading.Event()
    threading.Thread(target=_latir, args=(fin,), daemon=True).start()
    try:
        _recorrer(texto)
    except Exception:
        _parar_robot()
        _publicar(estado="detenida", detalle="La mision se interrumpio y el carro quedo quieto")
    finally:
        fin.set()
        en_curso.clear()


def _recorrer(texto):
    revision, servicios, estado = revisar()
    _publicar(servicios=servicios, estado="revisando", detalle="Revisando servicios")
    if not revision["listo"] or not estado:
        _publicar(estado="rechazada", detalle="Faltan servicios: " + ", ".join(revision["faltan"]))
        return
    try:
        interpretacion = _json(URLS["llm"], "/interpretar",
                               {"texto": texto, "catalogo": CATALOGO}, token=True, timeout=70)
    except (OSError, ValueError, json.JSONDecodeError):
        interpretacion = None
    lugares = (interpretacion or {}).get("lugares")
    nombres = (interpretacion or {}).get("nombres")
    if not isinstance(lugares, list) or not lugares or not isinstance(nombres, list):
        _parar_robot()
        _publicar(estado="rechazada", detalle="El LLM no devolvio lugares. El carro sigue quieto.")
        return
    _publicar(lugares=lugares, nombres=nombres, estado="en_marcha")
    for nombre in nombres:
        if detener.is_set():
            break
        _publicar(lugar=nombre, detalle="Buscando " + nombre)
        parada_inicial = estado.get("ultimaParada")
        ultima = ""
        ordenes = 0
        sentido = 1
        pasos_sin_ver = 0
        for _paso in range(PASOS):
            if detener.is_set():
                break
            estado = _estado_robot()
            if not estado:
                _parar_robot()
                _publicar(estado="detenida", detalle="Se perdio el carro")
                return
            if ordenes >= 1:
                resultado = llegada(ultima or "explorar", estado, parada_inicial, ordenes)
                if resultado == "llegue":
                    _parar_robot()
                    ultima = ""
                    break
                if resultado == "fallo":
                    _parar_robot()
                    _publicar(estado="detenida", detalle="El carro paro por " + str(estado.get("ultimaParada")))
                    return
            _publicar(accion="parar", detalle="Quieto para mirar " + nombre)
            try:
                _orden_robot("parar")
            except (OSError, ValueError):
                _publicar(estado="detenida", detalle="El carro no quedo quieto para la foto")
                return
            if not _esperar_quieto():
                if detener.is_set():
                    break
                _parar_robot()
                _publicar(estado="detenida", detalle="El carro no quedo quieto para la foto")
                return
            if detener.is_set():
                break
            estado = _estado_robot()
            if not estado:
                _parar_robot()
                _publicar(estado="detenida", detalle="Se perdio el carro")
                return
            try:
                foto = _captura()
                vision = _json(URLS["vlm"], "/ver", {
                    "objeto": nombre, "imagen": foto, "pistas": pistas_de(nombre, CATALOGO),
                }, token=True, timeout=80)
                if isinstance(vision, dict):
                    vision["objeto"] = nombre_visto(nombre, vision.get("objeto"), CATALOGO)
                decision = _json(URLS["vla"], "/decidir", {
                    "destino": nombre, "vision": vision, "lidar": resumen_lidar(estado),
                }, token=True, timeout=4)
            except (OSError, ValueError, json.JSONDecodeError, TypeError):
                _parar_robot()
                _publicar(estado="detenida", detalle="Un servicio dejo de responder y el carro se detuvo")
                return
            if not vision or not vision.get("sano") or not decision or decision.get("accion") not in (
                    "explorar", "avanzar", "girar_izquierda", "girar_derecha", "parar"):
                _parar_robot()
                _publicar(estado="detenida", detalle="VLM o VLA no contestaron. Motores quietos.")
                return
            visto = decision["accion"] == "avanzar"
            lidar = resumen_lidar(estado)
            accion, sentido, pasos_sin_ver = seguir_busqueda(
                decision["accion"], sentido, bool(lidar.get("frente_libre")), pasos_sin_ver)
            _publicar(accion=accion, detalle=decision.get("motivo", nombre))
            if accion == "parar" or detener.is_set():
                _parar_robot()
                ultima = ""
                break
            try:
                _orden_robot(accion)
            except (OSError, ValueError):
                _publicar(estado="detenida", detalle="El carro rechazo la orden")
                return
            ultima = accion
            ordenes += 1
            if accion.startswith("girar_"):
                time.sleep(GIRO_SEGUNDOS)
            elif visto:
                time.sleep(ACERCAR_SEGUNDOS)
            else:
                time.sleep(AVANCE_SEGUNDOS)
        else:
            _parar_robot()
            _publicar(estado="detenida", detalle="No encontro " + nombre + " a tiempo")
            return
    if detener.is_set():
        _parar_robot()
        _publicar(estado="detenida", detalle="Parada pedida por el operador")
        return
    _publicar(estado="completa", detalle="Llego a todos los lugares y quedo detenido", accion="parar")


class Orquestador(Base):
    def do_GET(self):
        ruta = self.path.split("?", 1)[0]
        if ruta == "/":
            self.rechazar(404, "La entrada es el gateway GraphQL")
            return
        if not viene_del_gateway(self):
            self.rechazar(403, "usa GraphQL")
            return
        if ruta == "/api/salud":
            revision, servicios, _estado = revisar()
            responder(self, 200 if revision["listo"] else 503, {"servicio": "orquestador", **revision, "servicios": servicios})
            return
        if ruta == "/api/mision":
            with candado:
                datos = dict(mision)
                datos["bitacora"] = [dict(paso) for paso in bitacora]
            responder(self, 200, datos)
            return
        self.rechazar(404, "ruta")

    def do_POST(self):
        if not viene_del_gateway(self):
            self.rechazar(403, "usa GraphQL")
            return
        if not token_valido(self, TOKEN):
            self.rechazar(401, "token")
            return
        ruta = self.path.split("?", 1)[0]
        if ruta == "/api/parar":
            detener.set()
            _parar_robot()
            _publicar(estado="detenida", detalle="Parada pedida por el operador", accion="parar")
            responder(self, 200, {"sano": True, "detalle": "STOP"})
            return
        if ruta != "/api/mision":
            self.rechazar(404, "ruta")
            return
        try:
            datos = leer_json(self, 8_000)
        except ValueError as exc:
            self.rechazar(400, str(exc))
            return
        texto = datos.get("texto")
        if not isinstance(texto, str) or not texto.strip() or len(texto) > 500:
            self.rechazar(400, "texto")
            return
        if en_curso.is_set():
            self.rechazar(409, "ya hay una mision")
            return
        en_curso.set()
        threading.Thread(target=_ciclo, args=(texto.strip(),), daemon=True).start()
        responder(self, 202, {"sano": True, "detalle": "mision aceptada"})


if __name__ == "__main__":
    if not TOKEN or not all(URLS.values()):
        raise SystemExit("Faltan TOKEN o las direcciones de los servicios")
    servir(Orquestador, PUERTO)
