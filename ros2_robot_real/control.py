"""Puente entre ROS 2 y el supervisor local de movimiento del S3."""

import json
import math
import os
from time import monotonic
from urllib.parse import urlencode
from urllib.request import Request, urlopen

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from nav_msgs.msg import Odometry
from sensor_msgs.msg import Imu, LaserScan
from std_msgs.msg import String, UInt32MultiArray
from std_srvs.srv import Trigger

from aprendizaje import Giros
from odometria import Odometria, Recorrido


# El S3 revoca por separado el permiso tras 900 ms sin una orden válida.
MAXIMO_SIN_LATIDO_S = 0.65


def pedir(base, ruta, parametros=None):
    if parametros is None:
        solicitud = Request(base + ruta)
    else:
        solicitud = Request(base + ruta + "?" + urlencode(parametros),
                           data=b"", headers={"X-Robot": "1"}, method="POST")
    with urlopen(solicitud, timeout=0.35) as respuesta:
        return respuesta.read(8192)


def telemetria(base):
    d = json.loads(pedir(base, "/telemetria"))
    if not isinstance(d, dict) or d.get("version") != "ROS2-1":
        raise ValueError("El S3 requiere el firmware ROS2-1")
    for campo in ("sesion", "orden", "corrida", "fase", "configurada", "frente",
                  "horario", "barridos", "muestraDevkit", "lidar", "imu", "enlace",
                  "permiso", "activa", "moviendo", "espacioIzq", "espacioDer"):
        if type(d.get(campo)) is not int or not 0 <= d[campo] <= 0xffffffff:
            raise ValueError(f"Telemetría inválida: {campo}")
    if (not 0 < d["sesion"] or d["fase"] > 9 or d.get("giro") not in (-1, 1)
            or not 0 <= d["frente"] < 360 or d["horario"] not in (0, 1)
            or not isinstance(d.get("mapa"), list) or len(d["mapa"]) != 72
            or any(type(mm) is not int or not 0 <= mm <= 12000 for mm in d["mapa"])):
        raise ValueError("Telemetría LiDAR inválida")
    return d


def barrido(d):
    sentido = -1 if d["horario"] else 1
    sectores = sorted(
        (((sentido * (i * 5 + 2.5 - d["frente"]) + 180) % 360) - 180,
         mm / 1000 if mm else math.nan)
        for i, mm in enumerate(d["mapa"]))
    scan = LaserScan()
    scan.header.frame_id = "laser_link"
    scan.angle_min = math.radians(sectores[0][0])
    scan.angle_max = math.radians(sectores[-1][0])
    scan.angle_increment = math.radians(5)
    scan.range_min, scan.range_max = 0.001, 12.0
    scan.ranges = [distancia for _, distancia in sectores]
    return scan


def sensores_devkit(d):
    partes = d["devkit"].split("|")
    datos = partes[1].split()
    cuentas = [int(c) for c in datos[:4]]
    valores = [float(v) for v in datos[4:]]
    if (len(cuentas) != 4 or len(valores) != 4
            or any(not 0 <= n <= 0xffffffff for n in cuentas)
            or any(not math.isfinite(v) for v in valores)):
        raise ValueError("Muestra DevKit inválida")
    mensaje = Imu()
    mensaje.header.frame_id = "imu_link"
    mensaje.orientation_covariance[0] = -1.0
    mensaje.angular_velocity_covariance[0] = 1e6
    mensaje.angular_velocity_covariance[4] = 1e6
    mensaje.linear_acceleration.x, mensaje.linear_acceleration.y, mensaje.linear_acceleration.z = (
        v * 9.80665 for v in valores[:3])
    mensaje.angular_velocity.z = math.radians(valores[3])
    odom = None
    if len(partes) >= 3:
        version, *extra = partes[2].split()
        if version != "O1" or len(extra) != 8:
            raise ValueError("Extensión de odometría inválida")
        identidad, ms, *firmados = (int(v) for v in extra[:6])
        gx, gy = (float(v) for v in extra[6:])
        if (not identidad or any(not 0 <= v <= 0xffffffff for v in [identidad, ms, *firmados])
                or not math.isfinite(gx) or not math.isfinite(gy)):
            raise ValueError("Extensión de odometría inválida")
        mensaje.angular_velocity.x, mensaje.angular_velocity.y = math.radians(gx), math.radians(gy)
        mensaje.angular_velocity_covariance[0] = mensaje.angular_velocity_covariance[4] = 0.0
        odom = identidad, ms, firmados
    if len(partes) == 4:
        version, *extra = partes[3].split()
        cuentas = [int(v) for v in extra]
        if version != "E2" or len(cuentas) != 4 or any(not 0 <= n <= 0xffffffff for n in cuentas):
            raise ValueError("Muestra de encoders filtrados inválida")
    return cuentas, mensaje, odom


class Control(Node):
    def __init__(self):
        super().__init__("control_robot")
        self.base = os.environ.get("ROBOT_URL", "http://192.168.4.1").rstrip("/")
        self.aprendizaje = Giros(os.environ.get("GIROS_JSON", "/datos/giros.json"))
        self.declare_parameter("frente_grados", -1)
        self.declare_parameter("sentido_lidar", 0)
        self.declare_parameter("radio_mm", 150)
        self.declare_parameter("trocha_m", 0.0)
        self.scan = self.create_publisher(LaserScan, "/scan", qos_profile_sensor_data)
        self.imu = self.create_publisher(Imu, "/imu/data_raw", qos_profile_sensor_data)
        self.encoders = self.create_publisher(UInt32MultiArray, "/robot/encoders", 10)
        self.odom = self.create_publisher(Odometry, "/odom", 10)
        self.odometria = Odometria()
        self.recorrido = Recorrido()
        self.distanciaFiltrada = None
        self.muestraGiro = None
        self.estado = self.create_publisher(String, "/robot/estado", 10)
        self.giros = self.create_publisher(String, "/robot/aprendizaje/giros", 10)
        self.create_service(Trigger, "/robot/configurar", self.configurar)
        self.create_service(Trigger, "/robot/arrancar", self.arrancar)
        self.create_service(Trigger, "/robot/parar", self.parar)
        self.d = None
        self.ultimoDato = 0.0
        self.ultimoLatido = 0.0
        self.sesionActiva = None
        self.corridaActiva = None
        self.inicioPendiente = None
        self.id = 0
        self.ultimoScan = None
        self.ultimaIMU = None
        self.create_timer(0.16, self.actualizar)

    def publicar(self, publicador, mensaje):
        publicador.publish(String(data=json.dumps(mensaje, ensure_ascii=False)))

    def enviar(self, d, inicio):
        siguiente = max(self.id, d["orden"]) + 1
        if siguiente > 0xffffffff:
            raise ValueError("Secuencia agotada. Reinicia el S3.")
        self.id = siguiente
        preferencia = self.aprendizaje.preferencia(d)
        pedir(self.base, "/ros/orden", {"sesion": d["sesion"], "id": self.id,
              "inicio": int(inicio), "preferencia": preferencia + 1})
        self.ultimoLatido = monotonic()

    def desarmar(self):
        self.sesionActiva = self.corridaActiva = self.inicioPendiente = None

    def configurar(self, _peticion, rsp):
        frente = self.get_parameter("frente_grados").value
        radio = self.get_parameter("radio_mm").value
        sentido = self.get_parameter("sentido_lidar").value
        if frente not in range(360) or radio not in range(100, 1001) or sentido not in (-1, 1):
            rsp.message = "Verifica el ángulo físico y fija frente_grados y sentido_lidar (1 o -1)"
            return rsp
        if not self.d or monotonic() - self.ultimoDato > 0.5 or self.d["activa"]:
            rsp.message = "Configura con el S3 conectado y el carro detenido"
            return rsp
        try:
            pedir(self.base, "/configurar", {"frente": frente, "radio": radio,
                  "sentido": sentido, "confirmar": 1})
            self.d = None
            rsp.success, rsp.message = True, "Geometría enviada. Comprueba /robot/estado."
        except OSError as error:
            rsp.message = str(error)
        return rsp

    def arrancar(self, _peticion, rsp):
        d = self.d
        if (not d or monotonic() - self.ultimoDato > 0.5 or d.get("puedeArmar", d["permiso"]) != 1
                or d["activa"] or self.sesionActiva is not None):
            rsp.message = "Sin permiso de arranque. Comprueba /robot/estado."
            return rsp
        try:
            self.enviar(d, True)
            self.sesionActiva = d["sesion"]
            self.inicioPendiente = self.id
            rsp.success = True
            rsp.message = ("Orden en cola. Confirma el estado en /robot/estado." if d["permiso"]
                           else "Orden en cola. El S3 esperará una trayectoria confirmada.")
        except (OSError, ValueError) as error:
            rsp.message = f"Orden sin confirmación: {error}. Caduca en el S3."
        return rsp

    def parar(self, _peticion, rsp):
        self.desarmar()
        try:
            pedir(self.base, "/stop", {})
            rsp.success, rsp.message = True, "STOP enviado. Comprueba /robot/estado."
        except OSError as error:
            rsp.message = f"STOP sin confirmación: {error}. El permiso local caduca."
        return rsp

    def renovar(self, d):
        if self.sesionActiva is None:
            return
        if (d["sesion"] != self.sesionActiva or
                monotonic() - self.ultimoLatido >= MAXIMO_SIN_LATIDO_S):
            self.desarmar()
            return
        if self.inicioPendiente is not None:
            if d["orden"] < self.inicioPendiente and monotonic() - self.ultimoLatido < 0.5:
                return
            self.corridaActiva = d["corrida"] if d["orden"] == self.inicioPendiente else None
            self.inicioPendiente = None
        if not d["activa"] or not d["corrida"] or d["corrida"] != self.corridaActiva:
            self.desarmar()
        elif monotonic() - self.ultimoLatido >= 0.24:
            self.enviar(d, False)

    def publicar_odom(self, muestra, stamp):
        trocha = self.get_parameter("trocha_m").value
        if not muestra or not 0.05 <= trocha <= 1.0:
            self.odometria.anterior = None
            return
        valores = self.odometria.actualizar(*muestra, trocha)
        if valores is None:
            return
        x, y, yaw, lineal, angular = valores
        msg = Odometry()
        msg.header.stamp, msg.header.frame_id, msg.child_frame_id = stamp, "odom", "base_link"
        msg.pose.pose.position.x, msg.pose.pose.position.y = x, y
        msg.pose.pose.orientation.z, msg.pose.pose.orientation.w = math.sin(yaw / 2), math.cos(yaw / 2)
        msg.twist.twist.linear.x, msg.twist.twist.angular.z = lineal, angular
        # Valores iniciales conservadores hasta calibrar el deslizamiento del chasis.
        for i, valor in enumerate((0.25, 0.25, 1e6, 1e6, 1e6, 0.4)):
            msg.pose.covariance[i * 7] = msg.twist.covariance[i * 7] = valor
        self.odom.publish(msg)

    def actualizar(self):
        try:
            d = telemetria(self.base)
            if self.d and self.d["sesion"] != d["sesion"]:
                self.id = 0
            self.d, self.ultimoDato = d, monotonic()
            self.renovar(d)
        except (OSError, ValueError) as error:
            self.d = None
            self.odometria.anterior = None
            self.recorrido.anterior = None
            self.muestraGiro = None
            self.aprendizaje.intento = None
            self.aprendizaje.reanudacion = None
            if (self.sesionActiva is not None and
                    (isinstance(error, ValueError) or
                     monotonic() - self.ultimoLatido >= MAXIMO_SIN_LATIDO_S)):
                self.desarmar()
                self.get_logger().error(f"Marcha desarmada: {error}")
            else:
                self.get_logger().warning(
                    f"Sin telemetría ROS2-1: {error}", throttle_duration_sec=10)
            self.publicar(self.estado, {"conectado": False, "detalle": str(error)})
            return
        stamp = self.get_clock().now().to_msg()
        scan = (d["sesion"], d["barridos"])
        if d["lidar"] and d["configurada"] and scan != self.ultimoScan:
            mensaje = barrido(d)
            mensaje.header.stamp = stamp
            self.scan.publish(mensaje)
            self.ultimoScan = scan
        muestra = (d["sesion"], d["muestraDevkit"])
        if not d["enlace"]:
            self.muestraGiro = None
        if d["enlace"] and muestra != self.ultimaIMU:
            self.ultimaIMU = muestra
            try:
                cuentas, imu, odom = sensores_devkit(d)
                self.muestraGiro = odom
                if odom:
                    corrida = int(d["devkit"].split()[2])
                    self.distanciaFiltrada = self.recorrido.actualizar(corrida, odom)
                self.encoders.publish(UInt32MultiArray(data=cuentas))
                if d["imu"]:
                    imu.header.stamp = stamp
                    self.imu.publish(imu)
                self.publicar_odom(odom, stamp)
            except (ValueError, TypeError, KeyError, IndexError) as error:
                self.muestraGiro = None
                self.get_logger().warning(f"Muestra DevKit descartada: {error}")
        try:
            resultado = self.aprendizaje.observar(d, self.muestraGiro)
            if resultado:
                self.publicar(self.giros, resultado)
        except OSError as error:
            self.get_logger().error(f"No se pudo guardar el aprendizaje: {error}")
        estado = {"conectado": True, **d}
        if self.distanciaFiltrada is not None and " | E2 " not in d.get("devkit", ""):
            estado["distancia_s3_cm"] = d.get("distancia")
            estado["distancia"] = round(self.distanciaFiltrada, 1)
        self.publicar(self.estado, estado)


def main():
    rclpy.init()
    nodo = Control()
    try:
        rclpy.spin(nodo)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        nodo.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
