"""Aprendizaje de giro: compara resultados observados en espacios equivalentes."""

import json
from math import sqrt
from pathlib import Path
from odometria import delta_pulsos


class Giros:
    def __init__(self, ruta):
        self.ruta = Path(ruta)
        try:
            anterior = json.loads(self.ruta.read_text(encoding="utf-8"))
            self.historial = {
                contexto: {lado: [max(0, int(n)) for n in anterior.get(contexto, {}).get(lado, [0, 0])]
                           for lado in ("1", "-1")}
                for contexto in self.contextos()
            }
            if any(len(v) != 2 for grupo in self.historial.values() for v in grupo.values()):
                raise ValueError("Formato de aprendizaje inválido")
        except (OSError, ValueError, KeyError, TypeError):
            self.historial = self.contextos()
        self.corrida = None
        self.intento = None
        self.reanudacion = None

    @staticmethod
    def contextos():
        return {c: {"1": [0, 0], "-1": [0, 0]} for c in
                ("estrecho", "amplio", "mas_espacio_izquierda", "mas_espacio_derecha")}

    @staticmethod
    def contexto(d):
        izq, der = d["espacioIzq"], d["espacioDer"]
        if not d["lidar"] or min(izq, der) < 1:
            return None
        if abs(izq - der) > 100:
            return "mas_espacio_izquierda" if izq > der else "mas_espacio_derecha"
        return "estrecho" if min(izq, der) < 700 else "amplio"

    def preferencia(self, d):
        contexto = self.contexto(d)
        if contexto not in ("estrecho", "amplio"):
            return 0
        historial = self.historial[contexto]
        # Tasa suavizada y un margen de exploración entre salidas equivalentes.
        if not sum(historial["1"]):
            return 1
        if not sum(historial["-1"]):
            return -1
        return max((1, -1), key=lambda lado: (
            (historial[str(lado)][0] + 1) / (sum(historial[str(lado)]) + 2)
            + 0.2 / sqrt(sum(historial[str(lado)]))))

    def observar(self, d, muestra=None):
        corrida = (d["sesion"], d["corrida"])
        fase, lado = d["fase"], d["giro"]
        cancelar = fase in (0, 7, 8, 9)
        if corrida != self.corrida or not d["corrida"] or cancelar or not muestra:
            self.corrida = corrida
            self.intento = None
            self.reanudacion = None
        if self.intento and self.intento[2][0] != muestra[0]:
            self.intento = None
            self.reanudacion = None
        if not d["corrida"] or cancelar or not muestra:
            return None
        if fase in (5, 6) or not d["lidar"]:
            self.reanudacion = None
            if self.intento and not self.intento[3]:
                self.intento = None
            return None

        resultado = None
        if self.intento and self.intento[3]:
            contexto, anterior, _, _ = self.intento
            if fase == 1 and self.reanudacion is None:
                self.reanudacion = muestra
            deltas = ([delta_pulsos(a, b) for a, b in zip(muestra[2], self.reanudacion[2])]
                      if fase == 1 else [])
            exito = fase == 1 and d["moviendo"] and sum(deltas[:2]) >= 4 and sum(deltas[2:]) >= 4
            if exito or fase == 2:
                if exito or lado != anterior:
                    self.historial[contexto][str(anterior)][0 if exito else 1] += 1
                    resultado = {"contexto": contexto, "lado": anterior, "salida": exito,
                                 "historial": self.historial}
                self.intento = None
                self.reanudacion = None
        if fase == 2 and self.intento is None:
            contexto = self.contexto(d)
            if contexto and lado in (-1, 1):
                self.intento = (contexto, lado, muestra, False)
        elif fase in (3, 4) and self.intento and self.intento[1] == lado and d["moviendo"]:
            base = self.intento[2]
            if muestra[1] != base[1]:
                deltas = [delta_pulsos(a, b) for a, b in zip(muestra[2], base[2])]
                if -lado * sum(deltas[:2]) >= 4 and lado * sum(deltas[2:]) >= 4:
                    self.intento = (self.intento[0], lado, base, True)
        elif fase == 1 and self.intento and not self.intento[3]:
            self.intento = None
        if resultado:
            temporal = self.ruta.with_suffix(".tmp")
            temporal.write_text(json.dumps(self.historial) + "\n", encoding="utf-8")
            temporal.replace(self.ruta)
        return resultado
