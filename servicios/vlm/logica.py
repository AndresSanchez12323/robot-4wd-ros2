"""Pide al modelo de vision si el objeto buscado se ve, y de que lado."""

import base64
import json
from urllib.request import Request, urlopen

LADOS = ("frente", "izquierda", "derecha", "indeterminada")
CAMPOS = ("objeto", "lado", "visible")
LADO_ALIAS = {
    "frente": "frente", "adelante": "frente", "centro": "frente", "center": "frente",
    "front": "frente", "enfrente": "frente",
    "izquierda": "izquierda", "izq": "izquierda", "left": "izquierda",
    "derecha": "derecha", "der": "derecha", "right": "derecha",
    "indeterminada": "indeterminada", "indeterminado": "indeterminada",
    "ninguno": "indeterminada", "desconocido": "indeterminada",
}


def jpeg_valido(codificado):
    if not isinstance(codificado, str) or not 100 <= len(codificado) <= 1_800_000:
        return False
    try:
        datos = base64.b64decode(codificado, validate=True)
    except (ValueError, TypeError):
        return False
    return datos.startswith(b"\xff\xd8") and datos.endswith(b"\xff\xd9") and len(datos) <= 1_200_000


def _lado(valor):
    if not isinstance(valor, str):
        raise ValueError("vision")
    lado = LADO_ALIAS.get(valor.strip().lower())
    if not lado:
        raise ValueError("vision")
    return lado


def _visible(valor):
    if isinstance(valor, bool):
        return valor
    if valor in (0, 1):
        return bool(valor)
    if isinstance(valor, str) and valor.strip().lower() in ("true", "si", "sí", "yes", "1", "verdadero"):
        return True
    if isinstance(valor, str) and valor.strip().lower() in ("false", "no", "0", "falso"):
        return False
    raise ValueError("vision")


def extraer(texto):
    limpio = texto.strip()
    inicio, fin = limpio.find("{"), limpio.rfind("}")
    if inicio < 0 or fin < inicio:
        raise ValueError("sin objeto")
    datos = json.loads(limpio[inicio:fin + 1])
    if not isinstance(datos, dict) or any(campo not in datos for campo in CAMPOS):
        raise ValueError("campos")
    if not isinstance(datos["objeto"], str) or not datos["objeto"].strip():
        raise ValueError("objeto")
    return {"objeto": datos["objeto"].strip(), "lado": _lado(datos["lado"]), "visible": _visible(datos["visible"])}


def mensaje(objeto, imagen, pistas=None):
    nombres = [objeto]
    if isinstance(pistas, list):
        nombres.extend(pista.strip() for pista in pistas if isinstance(pista, str) and pista.strip())
    alias = ", ".join(nombres[:8])
    return {
        "temperature": 0,
        "max_tokens": 160,
        "messages": [
            {"role": "system", "content": (
                "Miras una foto de la camara del robot y confirmas si el objeto pedido se ve. "
                "Si aparece, aunque esté cerca, pequeño o con otro nombre parecido, "
                "visible es true y objeto es exactamente el nombre pedido. "
                "lado es frente si está en el centro o adelante, izquierda, derecha o indeterminada. "
                "Solo usa visible false si el objeto no está en la foto. "
                "Responde solo JSON."
            )},
            {"role": "user", "content": [
                {"type": "text", "text": (
                    "Confirma si se ve: " + alias + ". "
                    "Si sí está al frente responde "
                    "{\"objeto\":\"" + objeto + "\",\"lado\":\"frente\",\"visible\":true}."
                )},
                {"type": "image_url", "image_url": {"url": "data:image/jpeg;base64," + imagen}},
            ]},
        ],
    }


def pedir_modelo(url, cuerpo, timeout=75):
    solicitud = Request(
        url.rstrip("/") + "/v1/chat/completions",
        data=json.dumps(cuerpo).encode(),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urlopen(solicitud, timeout=timeout) as respuesta:
        datos = json.loads(respuesta.read(1_000_000).decode())
    return datos["choices"][0]["message"]["content"]


def consultar(url, ruta, timeout):
    try:
        with urlopen(url.rstrip("/") + ruta, timeout=timeout) as respuesta:
            if not 200 <= respuesta.status < 300:
                return None
            bruto = respuesta.read(8000)
            if not bruto:
                return {"sano": True}
            datos = json.loads(bruto.decode())
            return datos if isinstance(datos, dict) else None
    except (OSError, ValueError, json.JSONDecodeError):
        return None


def gpu_exigida_lista(exigir, gpu_url):
    if not exigir:
        return True, {"dispositivo": "cpu", "capas_en_gpu": 0, "sano": True}
    estado = consultar(gpu_url, "/salud", 2) if gpu_url else None
    if not estado or not estado.get("sano") or not estado.get("dispositivo"):
        return False, estado or {"sano": False, "detalle": "gpu apagada"}
    if not isinstance(estado.get("capas_en_gpu"), int) or estado["capas_en_gpu"] < 1:
        return False, estado
    return True, estado
