"""Servicio VLA. Solo decide la siguiente accion; no habla con los motores."""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from comun.servicio import Base, leer_json, responder, servir, token_valido
from vla.logica import decidir

TOKEN = os.environ.get("TOKEN", "")
PUERTO = int(os.environ.get("PUERTO", "8080"))


class VLA(Base):
    def do_GET(self):
        if self.path.split("?", 1)[0] != "/salud":
            self.rechazar(404, "ruta")
            return
        responder(self, 200, {"servicio": "vla", "sano": True, "detalle": "listo"})

    def do_POST(self):
        if self.path.split("?", 1)[0] != "/decidir":
            self.rechazar(404, "ruta")
            return
        if not token_valido(self, TOKEN):
            self.rechazar(401, "token")
            return
        try:
            datos = leer_json(self, 16_000)
            decision = decidir(datos.get("destino"), datos.get("vision"), datos.get("lidar"))
        except ValueError as exc:
            self.rechazar(400, str(exc))
            return
        responder(self, 200, {"servicio": "vla", "sano": True, **decision})


if __name__ == "__main__":
    if not TOKEN:
        raise SystemExit("Falta TOKEN")
    servir(VLA, PUERTO)
