"""Servicio LLM. Sin el modelo configurado responde que no esta sano."""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from comun.servicio import Base, leer_json, responder, servir, token_valido
from llm.logica import catalogo_valido, mensaje, modelo_sano, normalizar, pedir_modelo, LIMITE_TEXTO

TOKEN = os.environ.get("TOKEN", "")
MODELO_URL = os.environ.get("MODELO_URL", "").rstrip("/")
PUERTO = int(os.environ.get("PUERTO", "8080"))


class LLM(Base):
    def do_GET(self):
        if self.path.split("?", 1)[0] != "/salud":
            self.rechazar(404, "ruta")
            return
        sano = bool(MODELO_URL) and modelo_sano(MODELO_URL)
        responder(self, 200 if sano else 503, {
            "servicio": "llm", "sano": sano,
            "detalle": "modelo alcanzable" if sano else "modelo apagado",
        })

    def do_POST(self):
        if self.path.split("?", 1)[0] != "/interpretar":
            self.rechazar(404, "ruta")
            return
        if not token_valido(self, TOKEN):
            self.rechazar(401, "token")
            return
        if not MODELO_URL or not modelo_sano(MODELO_URL):
            self.rechazar(503, "modelo apagado")
            return
        try:
            datos = leer_json(self, 16_000)
        except ValueError as exc:
            self.rechazar(400, str(exc))
            return
        texto, catalogo = datos.get("texto"), datos.get("catalogo")
        if not isinstance(texto, str) or not texto.strip() or len(texto) > LIMITE_TEXTO:
            self.rechazar(400, "texto")
            return
        if not catalogo_valido(catalogo):
            self.rechazar(400, "catalogo")
            return
        try:
            crudo = pedir_modelo(MODELO_URL, mensaje(texto.strip(), catalogo))
            responder(self, 200, {"servicio": "llm", "sano": True, **normalizar(crudo, catalogo)})
        except (OSError, ValueError, KeyError, TypeError, IndexError):
            self.rechazar(503, "el modelo no devolvio lugares del catalogo")


if __name__ == "__main__":
    if not TOKEN:
        raise SystemExit("Falta TOKEN")
    servir(LLM, PUERTO)
