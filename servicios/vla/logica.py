"""Una sola accion a partir de la vision y del LiDAR."""

ACCIONES = ("explorar", "avanzar", "girar_izquierda", "girar_derecha", "parar")
LADOS = ("frente", "izquierda", "derecha", "indeterminada")


def _mismo(visto, destino):
    return isinstance(visto, str) and isinstance(destino, str) and visto.strip().lower() == destino.strip().lower()


def decidir(destino, vision, lidar):
    if not isinstance(vision, dict) or not isinstance(lidar, dict):
        raise ValueError("entrada")
    if lidar.get("falla"):
        return {"accion": "parar", "motivo": "Sin datos del carro"}
    lado = vision.get("lado")
    if lado not in LADOS or not isinstance(vision.get("visible"), bool):
        raise ValueError("vision")
    visible = vision["visible"] and _mismo(vision.get("objeto"), destino)
    if visible and lado == "frente" and lidar.get("cerca"):
        return {"accion": "parar", "motivo": "Objetivo al frente y cerca"}
    if visible and lado == "frente":
        return {"accion": "avanzar", "motivo": "Objetivo al frente"}
    if visible and lado == "izquierda":
        return {"accion": "girar_izquierda", "motivo": "Objetivo a la izquierda"}
    if visible and lado == "derecha":
        return {"accion": "girar_derecha", "motivo": "Objetivo a la derecha"}
    return {"accion": "explorar", "motivo": "Buscando el objetivo"}
