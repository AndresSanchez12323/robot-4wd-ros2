"""Prueba DDS con un S3 HTTP simulado. No realiza conexiones con el carro."""

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import math
import os
from pathlib import Path
from tempfile import TemporaryDirectory
from threading import Thread
from time import monotonic
from urllib.parse import parse_qs, urlsplit

import rclpy
from nav_msgs.msg import Odometry
from rclpy.executors import SingleThreadedExecutor
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Imu, LaserScan
from std_srvs.srv import Trigger

from aprendizaje import Giros
from control import Control, sensores_devkit
from odometria import MODULO, Odometria, Recorrido


class S3Simulado(BaseHTTPRequestHandler):
    d = {"version": "ROS2-1", "sesion": 300, "orden": 0, "corrida": 0, "fase": 0,
         "giro": 1, "configurada": 1, "frente": 90, "horario": 1, "barridos": 1,
         "muestraDevkit": 0, "lidar": 1, "imu": 1, "enlace": 1, "permiso": 1,
         "activa": 0, "moviendo": 0, "espacioIzq": 700, "espacioDer": 720,
         "mapa": [0] + [900] * 71}
    llamadas = []
    sin_wifi = False

    def log_message(self, *args):
        pass

    def responder(self, dato):
        self.send_response(200)
        self.end_headers()
        self.wfile.write(json.dumps(dato).encode())

    def do_GET(self):
        assert self.path == "/telemetria"
        if self.sin_wifi:
            self.send_error(503)
            return
        self.d["muestraDevkit"] += 200
        self.d["barridos"] += 1
        self.d["devkit"] = (
            f"A1 300 0 0 0 1 0.0 OK | 0 0 0 0 0.0 0.0 1.0 5.0 "
            f"| O1 70 {self.d['muestraDevkit']} 0 0 0 0 1.0 2.0")
        self.responder(self.d)

    def do_POST(self):
        url = urlsplit(self.path)
        args = {k: int(v[0]) for k, v in parse_qs(url.query).items()}
        self.llamadas.append((url.path, args))
        assert self.headers["X-Robot"] == "1"
        if url.path == "/ros/orden":
            assert args["sesion"] == 300 and args["id"] > self.d["orden"]
            self.d["orden"] = args["id"]
            if args["inicio"]:
                self.d.update(activa=1, corrida=10, fase=1)
        elif url.path == "/stop":
            self.d.update(activa=0, corrida=0, fase=0)
        else:
            assert url.path == "/configurar"
        self.responder({})


def esperar(executor, condicion, segundos=5):
    fin = monotonic() + segundos
    while not condicion() and monotonic() < fin:
        executor.spin_once(timeout_sec=0.08)
    assert condicion()


def probar_transporte(ruta):
    servidor = ThreadingHTTPServer(("127.0.0.1", 0), S3Simulado)
    Thread(target=servidor.serve_forever, daemon=True).start()
    os.environ["ROBOT_URL"] = f"http://127.0.0.1:{servidor.server_port}"
    os.environ["GIROS_JSON"] = str(ruta)
    rclpy.init()
    nodo, cliente = Control(), Node("prueba_control")
    nodo.set_parameters([rclpy.parameter.Parameter("trocha_m", value=0.20)])
    ex = SingleThreadedExecutor()
    ex.add_node(nodo)
    ex.add_node(cliente)
    sensores = {}
    for tipo, topic in ((LaserScan, "/scan"), (Imu, "/imu/data_raw"), (Odometry, "/odom")):
        cliente.create_subscription(tipo, topic, lambda m, t=topic: sensores.update({t: m}),
                                    qos_profile_sensor_data)
    try:
        esperar(ex, lambda: len(sensores) == 3)
        assert sum(math.isnan(x) for x in sensores["/scan"].ranges) == 1
        assert math.isclose(sensores["/imu/data_raw"].linear_acceleration.z, 9.80665)
        assert not S3Simulado.llamadas
        arranque = cliente.create_client(Trigger, "/robot/arrancar")
        futura = arranque.call_async(Trigger.Request())
        esperar(ex, futura.done)
        assert futura.result().success
        esperar(ex, lambda: len(S3Simulado.llamadas) >= 3)
        assert sum(args["inicio"] for _, args in S3Simulado.llamadas) == 1
        S3Simulado.sin_wifi = True
        esperar(ex, lambda: nodo.d is None)
        assert nodo.sesionActiva is not None
        breve = len(S3Simulado.llamadas)
        S3Simulado.sin_wifi = False
        esperar(ex, lambda: len(S3Simulado.llamadas) > breve)
        assert sum(args["inicio"] for _, args in S3Simulado.llamadas) == 1
        S3Simulado.sin_wifi = True
        esperar(ex, lambda: nodo.sesionActiva is None)
        ultima = len(S3Simulado.llamadas)
        S3Simulado.sin_wifi = False
        esperar(ex, lambda: nodo.d is not None)
        comienzo = monotonic()
        esperar(ex, lambda: monotonic() - comienzo > 1)
        assert len(S3Simulado.llamadas) == ultima
        parada = cliente.create_client(Trigger, "/robot/parar")
        futura = parada.call_async(Trigger.Request())
        esperar(ex, futura.done)
        assert futura.result().success and S3Simulado.llamadas[-1][0] == "/stop"
    finally:
        ex.shutdown()
        cliente.destroy_node()
        nodo.destroy_node()
        rclpy.shutdown()
        servidor.shutdown()
        servidor.server_close()


def probar_aprendizaje(ruta):
    politica = Giros(ruta)
    d = dict(S3Simulado.d, corrida=3, fase=2, giro=1, moviendo=1)
    assert politica.preferencia(d) == 1
    cero = (70, 100, [0, 0, 0, 0])
    izquierda = (70, 300, [MODULO-3, MODULO-3, 4, 4])
    derecha = (70, 500, [0, 0, 0, 0])
    politica.observar(d, cero)
    politica.observar(dict(d, fase=3), cero)
    assert politica.observar(dict(d, fase=1), cero) is None
    politica.observar(d, cero)
    politica.observar(dict(d, fase=3), izquierda)
    assert politica.observar(dict(d, fase=2, giro=-1), izquierda)["salida"] is False
    politica.observar(dict(d, fase=4, giro=-1), derecha)
    assert politica.observar(dict(d, fase=1, giro=-1), derecha) is None
    avance = (70, 700, [2, 2, 2, 2])
    assert politica.observar(dict(d, fase=1, giro=-1), avance)["salida"] is True
    nueva = Giros(ruta)
    assert nueva.preferencia(d) == -1
    assert nueva.preferencia(dict(d, espacioIzq=1000)) == 0
    nueva.observar(d, cero)
    nueva.observar(dict(d, fase=3), izquierda)
    assert nueva.observar(dict(d, corrida=0), izquierda) is None
    asimetrico = dict(d, corrida=4, espacioIzq=1000)
    nueva.observar(asimetrico, cero)
    nueva.observar(dict(asimetrico, fase=3), izquierda)
    assert nueva.observar(dict(asimetrico, fase=6, lidar=0, moviendo=0), izquierda) is None
    assert nueva.observar(dict(asimetrico, fase=5, moviendo=0), izquierda) is None
    assert nueva.observar(dict(asimetrico, fase=1, moviendo=0), izquierda) is None
    avance = (70, 700, [0, 0, 8, 8])
    dato = nueva.observar(dict(asimetrico, fase=1), avance)
    assert dato["contexto"] == "mas_espacio_izquierda" and dato["salida"]
    assert nueva.observar(dict(asimetrico, fase=1), avance) is None
    assert nueva.preferencia(asimetrico) == 0

    nueva.observar(dict(d, corrida=5), cero)
    nueva.observar(dict(d, corrida=5, fase=3), izquierda)
    nueva.observar(dict(d, corrida=5, fase=6, lidar=0), izquierda)
    nueva.observar(dict(d, corrida=5, fase=0), izquierda)
    assert nueva.observar(dict(d, corrida=6, fase=1), avance) is None


def probar_odometria():
    cuentas, _, _ = sensores_devkit({"devkit":
        "A1 300 0 0 0 1 0.0 OK | 0 10378 0 489 0 0 1 0 | O1 70 200 0 0 0 0 0 0 | E2 0 0 0 0"})
    assert cuentas == [0, 0, 0, 0]
    odom = Odometria()
    assert odom.actualizar(1, 0, [0] * 4, 0.2) is None
    x, y, yaw, lineal, angular = odom.actualizar(1, 400, [27, 25, 26, 24], 0.2)
    assert math.isclose(x, 0.132) and y == yaw == angular == 0
    assert math.isclose(lineal, 0.33)
    _, _, yaw, _, _ = odom.actualizar(1, 800, [0, 0, 52, 48], 0.2)
    assert math.isclose(yaw, 1.32)
    assert odom.actualizar(2, 0, [0] * 4, 0.2) is None
    assert odom.x == odom.y == odom.yaw == 0
    recorrido = Recorrido()
    assert recorrido.actualizar(0, (1, 0, [0]*4)) == 0
    assert math.isclose(recorrido.actualizar(1, (1, 200, [27, 25, 26, 24])), 13.2)
    assert math.isclose(recorrido.actualizar(0, (1, 400, [27, 25, 26, 24])), 13.2)
    assert math.isclose(recorrido.actualizar(0, (1, 600, [27, 25, 26, 24])), 13.2)


if __name__ == "__main__":
    with TemporaryDirectory() as carpeta:
        ruta = Path(carpeta) / "giros.json"
        probar_transporte(ruta)
        probar_aprendizaje(ruta)
        probar_odometria()
    print("OK: scan, IMU, odometría, orden explícita, latidos, STOP, pérdida de red y aprendizaje persistente.")
