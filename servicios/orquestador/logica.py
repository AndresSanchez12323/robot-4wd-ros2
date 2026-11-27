"""Resume el LiDAR como el firmware y decide si la mision puede seguir."""

import math

ECO_PROPIO = 80
ANTICIPACION = 150


def _coincide(visto, alias):
    alias = alias.strip().lower()
    if not alias or visto == alias:
        return bool(alias)
    return len(alias) >= 4 and alias in visto


def nombre_visto(destino, objeto, catalogo):
    """Acepta el nombre del catalogo, una pista o una frase que lo contenga."""
    if not isinstance(objeto, str) or not isinstance(destino, str) or not isinstance(catalogo, list):
        return objeto
    visto = objeto.strip().lower()
    pedido = destino.strip().lower()
    for lugar in catalogo:
        if not isinstance(lugar, dict):
            continue
        nombre = lugar.get("nombre")
        if not isinstance(nombre, str) or nombre.strip().lower() != pedido:
            continue
        alias = [nombre]
        pistas = lugar.get("pistas")
        if isinstance(pistas, list):
            alias.extend(pista for pista in pistas if isinstance(pista, str))
        if any(_coincide(visto, item) for item in alias):
            return nombre
    return objeto


def pistas_de(destino, catalogo):
    if not isinstance(destino, str) or not isinstance(catalogo, list):
        return []
    pedido = destino.strip().lower()
    for lugar in catalogo:
        if not isinstance(lugar, dict):
            continue
        nombre = lugar.get("nombre")
        if not isinstance(nombre, str) or nombre.strip().lower() != pedido:
            continue
        pistas = lugar.get("pistas")
        if not isinstance(pistas, list):
            return []
        return [pista.strip() for pista in pistas if isinstance(pista, str) and pista.strip()][:8]
    return []


def seguir_busqueda(accion, sentido, frente_libre, pasos_sin_ver):
    """Si el objeto se ve, va hacia el. Si no, avanza con el frente libre y solo gira cuando hay pared."""
    if accion == "parar":
        return "parar", sentido, 0
    if accion == "avanzar":
        return "avanzar", sentido, 0
    if accion == "girar_izquierda":
        return "girar_izquierda", 1, 0
    if accion == "girar_derecha":
        return "girar_derecha", -1, 0
    sentido = -1 if sentido < 0 else 1
    pasos = pasos_sin_ver + 1
    if frente_libre and pasos % 4 != 0:
        return "avanzar", sentido, pasos
    giro = "girar_derecha" if sentido < 0 else "girar_izquierda"
    return giro, sentido, pasos


def anotar(bitacora, actual, limite=60):
    """Guarda un paso solo cuando cambia la decision, para poder copiar la lista."""
    if not isinstance(bitacora, list) or not isinstance(actual, dict):
        return bitacora
    paso = {campo: str(actual.get(campo) or "")[:200] for campo in ("estado", "detalle", "lugar", "accion")}
    if bitacora and all(bitacora[-1].get(campo) == paso[campo] for campo in paso):
        return bitacora
    bitacora.append(paso)
    if len(bitacora) > limite:
        del bitacora[:-limite]
    return bitacora


def evaluar(servicios):
    faltan = [nombre for nombre, estado in servicios.items() if not estado or not estado.get("sano")]
    return {"listo": not faltan, "faltan": faltan}


def llegada(accion, estado, parada_inicial, ordenes, confirmado=False):
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
        return "llegue" if confirmado else "seguir"
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
