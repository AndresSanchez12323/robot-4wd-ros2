"""Servidor HTTP compartido por los microservicios."""

import hmac
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


def leer_json(handler, limite):
    try:
        largo = int(handler.headers.get("Content-Length", "0"))
    except ValueError as exc:
        raise ValueError("cabecera") from exc
    if largo < 0 or largo > limite:
        raise ValueError("tamano")
    bruto = handler.rfile.read(largo) if largo else b""
    try:
        datos = json.loads(bruto.decode("utf-8")) if bruto else {}
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise ValueError("json") from exc
    if not isinstance(datos, dict):
        raise ValueError("objeto")
    return datos


def responder(handler, codigo, cuerpo):
    datos = json.dumps(cuerpo, ensure_ascii=False).encode("utf-8")
    handler.send_response(codigo)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Cache-Control", "no-store")
    handler.send_header("Content-Length", str(len(datos)))
    handler.end_headers()
    handler.wfile.write(datos)


def token_valido(handler, esperado):
    recibido = handler.headers.get("Authorization", "")
    if not esperado or not recibido.startswith("Bearer "):
        return False
    return hmac.compare_digest(recibido[7:].encode(), esperado.encode())


def servir(manejador, puerto):
    ThreadingHTTPServer(("0.0.0.0", puerto), manejador).serve_forever()


class Base(BaseHTTPRequestHandler):
    def log_message(self, formato, *args):
        return

    def rechazar(self, codigo, detalle):
        responder(self, codigo, {"sano": False, "detalle": detalle})
