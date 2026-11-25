"""Convierte una frase en la lista ordenada de lugares del catalogo."""

import json
import re
from urllib.request import Request, urlopen

LIMITE_TEXTO = 500
LIMITE_LUGARES = 8


def catalogo_valido(catalogo):
    if not isinstance(catalogo, list) or not 1 <= len(catalogo) <= 20:
        return False
    vistos = set()
    for lugar in catalogo:
        if not isinstance(lugar, dict):
            return False
        ident, nombre, pistas = lugar.get("id"), lugar.get("nombre"), lugar.get("pistas", [])
        if not isinstance(ident, int) or not 1 <= ident <= 99 or ident in vistos:
            return False
        if not isinstance(nombre, str) or not 1 <= len(nombre) <= 40:
            return False
        if not isinstance(pistas, list) or len(pistas) > 12:
            return False
        if any(not isinstance(pista, str) or not pista or len(pista) > 80 for pista in pistas):
            return False
        vistos.add(ident)
    return True


def extraer_json(texto):
    limpio = texto.strip()
    if limpio.startswith("```"):
        limpio = re.sub(r"^```(?:json)?", "", limpio).strip()
        limpio = re.sub(r"```$", "", limpio).strip()
    inicio, fin = limpio.find("{"), limpio.rfind("}")
    if inicio < 0 or fin < inicio:
        raise ValueError("sin objeto")
    datos = json.loads(limpio[inicio:fin + 1])
    if not isinstance(datos, dict):
        raise ValueError("objeto")
    return datos


def normalizar(respuesta, catalogo):
    lugares = extraer_json(respuesta).get("lugares")
    if not isinstance(lugares, list) or not lugares or len(lugares) > LIMITE_LUGARES:
        raise ValueError("lugares")
    permitidos = {lugar["id"]: lugar["nombre"] for lugar in catalogo}
    salida = []
    for item in lugares:
        if isinstance(item, str) and item.isdigit():
            item = int(item)
        if not isinstance(item, int) or item not in permitidos:
            raise ValueError("lugar")
        salida.append(item)
    return {"lugares": salida, "nombres": [permitidos[ident] for ident in salida]}


def mensaje(texto, catalogo):
    return {
        "temperature": 0,
        "max_tokens": 120,
        "messages": [
            {"role": "system", "content": (
                "Eres el servicio de lenguaje del robot. "
                "Del catalogo elige los lugares que la frase pide, en ese orden. "
                "Acepta cualquier idioma y cualquier forma de pedirlo. "
                "Responde solo JSON {\"lugares\":[1,2]} con ids del catalogo. "
                "Si no corresponde ningun lugar, responde {\"lugares\":[]}."
            )},
            {"role": "user", "content": json.dumps(
                {"frase": texto, "catalogo": catalogo}, ensure_ascii=False)},
        ],
    }


def pedir_modelo(url, cuerpo, timeout=60):
    solicitud = Request(
        url.rstrip("/") + "/v1/chat/completions",
        data=json.dumps(cuerpo).encode(),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urlopen(solicitud, timeout=timeout) as respuesta:
        datos = json.loads(respuesta.read(1_000_000).decode())
    return datos["choices"][0]["message"]["content"]


def modelo_sano(url, timeout=2):
    try:
        with urlopen(url.rstrip("/") + "/health", timeout=timeout) as respuesta:
            return 200 <= respuesta.status < 300
    except (OSError, ValueError):
        return False
