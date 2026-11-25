"""Informa la tarjeta NVIDIA visible para este contenedor."""

import os
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json

PUERTO = int(os.environ.get("PUERTO", "8080"))
CAPAS = int(os.environ.get("CAPAS_EN_GPU", "99"))


def leer_tarjeta():
    try:
        resultado = subprocess.run(
            ["nvidia-smi", "--query-gpu=name", "--format=csv,noheader"],
            capture_output=True, text=True, timeout=3, check=False,
        )
    except (OSError, subprocess.TimeoutExpired):
        return ""
    if resultado.returncode != 0:
        return ""
    lineas = [linea.strip() for linea in resultado.stdout.splitlines() if linea.strip()]
    return lineas[0] if lineas else ""


class GPU(BaseHTTPRequestHandler):
    def log_message(self, formato, *args):
        return

    def do_GET(self):
        if self.path.split("?", 1)[0] != "/salud":
            self.send_error(404)
            return
        tarjeta = leer_tarjeta()
        sano = bool(tarjeta) and CAPAS >= 1
        cuerpo = {
            "servicio": "gpu",
            "sano": sano,
            "dispositivo": tarjeta,
            "capas_en_gpu": CAPAS if sano else 0,
            "detalle": "gpu en uso" if sano else "gpu apagada",
        }
        datos = json.dumps(cuerpo).encode()
        self.send_response(200 if sano else 503)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(datos)))
        self.end_headers()
        self.wfile.write(datos)


if __name__ == "__main__":
    ThreadingHTTPServer(("0.0.0.0", PUERTO), GPU).serve_forever()
