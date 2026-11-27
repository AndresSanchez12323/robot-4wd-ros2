"""Resume el LiDAR como el firmware y decide si la mision puede seguir."""

import math

ECO_PROPIO = 80
ANTICIPACION = 150


def seguir_busqueda(accion, sentido):
    """Si el objeto no se ve, el carro sigue girando hacia el mismo lado."""
    if accion == "girar_izquierda":
        return "girar_izquierda", 1
    if accion == "girar_derecha":
        return "girar_derecha", -1
    if accion == "explorar":
        if sentido < 0:
            return "girar_derecha", -1
        return "girar_izquierda", 1
    return accion, sentido


def evaluar(servicios):
    faltan = [nombre for nombre, estado in servicios.items() if not estado or not estado.get("sano")]
    return {"listo": not faltan, "faltan": faltan}


def llegada(accion, estado, parada_inicial, ordenes):
    if accion == "parar":
        return "llegue"
    if ordenes < 1 or not isinstance(estado, dict):
        return "seguir"
    parada = estado.get("ultimaParada") or ""
    if estado.get("activa"):
        return "seguir"
    if parada == parada_inicial:
        return "seguir"
    if "Parado al lado" in parada:
        return "llegue"
    if parada not in ("Ninguna", "STOP o cambio de configuracion", ""):
        return "fallo"
    return "seguir"


def _medida(mapa, indice):
    a, b = mapa[2 * indice], mapa[2 * indice + 1]
    if not a:
        return b
    if not b:
        return a
    return min(a, b)


def _relativo(celda, frente, horario):
    angulo = celda * 10.0 + 5.0 - frente
    while angulo > 180.0:
        angulo -= 360.0
    while angulo <= -180.0:
        angulo += 360.0
    return -angulo if horario else angulo


def resumen_lidar(estado):
    mapa = estado.get("mapa")
    if not isinstance(mapa, list) or len(mapa) != 72 or any(type(v) is not int or v < 0 or v > 12000 for v in mapa):
        return {"falla": True, "cerca": False, "frente_libre": False}
    frente, radio = estado.get("frente"), estado.get("radio")
    if type(frente) is not int or type(radio) is not int:
        return {"falla": True, "cerca": False, "frente_libre": False}
    horario = bool(estado.get("horario"))
    limite = radio + 80
    cerca = False
    margen = radio + 100
    bloqueado = False
    for i in range(36):
        grados = _relativo(i, frente, horario)
        distancia = _medida(mapa, i)
        if abs(grados) <= 30 and ECO_PROPIO <= distancia < limite:
            cerca = True
        radianes = math.radians(grados)
        x, y = math.cos(radianes), math.sin(radianes)
        if x <= 0 or distancia < ECO_PROPIO:
            continue
        ancho = margen + distancia * 0.08716
        px, py = distancia * x, abs(distancia * y)
        dx = px - ANTICIPACION if px > ANTICIPACION else 0.0
        if dx * dx + py * py <= ancho * ancho:
            bloqueado = True
    return {"falla": not estado.get("lidar"), "cerca": cerca, "frente_libre": not bloqueado}


def robot_util(estado):
    if not isinstance(estado, dict):
        return False
    return (estado.get("version") == "1.6" and estado.get("enlace") == 1
            and estado.get("imu") == 1 and estado.get("lidar") == 1
            and estado.get("configurada") == 1 and estado.get("permiso") == 1)
