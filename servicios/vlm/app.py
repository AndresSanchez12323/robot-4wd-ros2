"""Servicio VLM. Si se le exige la GPU y esa no responde, no mira la camara."""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from comun.servicio import Base, leer_json, responder, servir, token_valido
from vlm.logica import consultar, extraer, gpu_exigida_lista, jpeg_valido, mensaje, pedir_modelo

TOKEN = os.environ.get("TOKEN", "")
MODELO_URL = os.environ.get("MODELO_URL", "").rstrip("/")
GPU_URL = os.environ.get("GPU_URL", "").rstrip("/")
EXIGIR_GPU = os.environ.get("EXIGIR_GPU", "1") == "1"
PUERTO = int(os.environ.get("PUERTO", "8080"))


def _salud():
    gpu_ok, gpu = gpu_exigida_lista(EXIGIR_GPU, GPU_URL)
    modelo_ok = bool(MODELO_URL) and consultar(MODELO_URL, "/health", 2) is not None
    return gpu_ok and modelo_ok, gpu


class VLM(Base):
    def do_GET(self):
        if self.path.split("?", 1)[0] != "/salud":
            self.rechazar(404, "ruta")
            return
        sano, gpu = _salud()
        responder(self, 200 if sano else 503, {
            "servicio": "vlm", "sano": sano, "gpu": gpu,
            "detalle": "camara lista" if sano else "vlm incompleto",
        })

    def do_POST(self):
        if self.path.split("?", 1)[0] != "/ver":
            self.rechazar(404, "ruta")
            return
        if not token_valido(self, TOKEN):
            self.rechazar(401, "token")
            return
        sano, _gpu = _salud()
        if not sano:
            self.rechazar(503, "vlm incompleto")
            return
        try:
            datos = leer_json(self, 2_000_000)
        except ValueError as exc:
            self.rechazar(400, str(exc))
            return
        objeto, imagen = datos.get("objeto"), datos.get("imagen")
        pistas = datos.get("pistas") if isinstance(datos.get("pistas"), list) else []
        if not isinstance(objeto, str) or not 1 <= len(objeto) <= 40 or not jpeg_valido(imagen):
            self.rechazar(400, "objeto o imagen")
            return
        try:
            visto = extraer(pedir_modelo(MODELO_URL, mensaje(objeto.strip(), imagen, pistas)))
        except (OSError, ValueError, KeyError, TypeError, IndexError):
            self.rechazar(503, "el modelo de vision no respondio")
            return
        responder(self, 200, {"servicio": "vlm", "sano": True, **visto})


if __name__ == "__main__":
    if not TOKEN:
        raise SystemExit("Falta TOKEN")
    servir(VLM, PUERTO)
