"""Integración diferencial de los pulsos etiquetados por el DevKit."""

from math import cos, sin


METROS_POR_PULSO = tuple(0.132 / n for n in (27, 25, 26, 24))
MODULO = 1 << 32


def delta_pulsos(actual, anterior):
    return (actual - anterior + MODULO // 2) % MODULO - MODULO // 2


class Recorrido:
    """Distancia de rueda estimada desde los contadores firmados, incluso con A1 antiguo."""

    def __init__(self):
        self.corrida = 0
        self.anterior = None
        self.metros = 0.0

    def actualizar(self, corrida, muestra):
        identidad, ms, pulsos = muestra
        anterior = self.anterior
        self.anterior = muestra
        if corrida and corrida != self.corrida:
            self.metros = 0.0
        if (anterior and anterior[0] == identidad and (corrida or self.corrida)
                and 0 < (ms - anterior[1]) % MODULO <= 1000):
            self.metros += sum(abs(delta_pulsos(a, b)) * k for a, b, k in
                               zip(pulsos, anterior[2], METROS_POR_PULSO)) / 4
        self.corrida = corrida
        return self.metros * 100


class Odometria:
    def __init__(self):
        self.x = self.y = self.yaw = 0.0
        self.anterior = None
        self.trocha = None
        self.identidad = None

    def actualizar(self, identidad, ms, pulsos, trocha):
        anterior = self.anterior
        self.anterior = (identidad, ms, pulsos)
        if self.identidad != identidad or self.trocha != trocha:
            self.identidad = identidad
            self.trocha = trocha
            self.x = self.y = self.yaw = 0.0
            return None
        if anterior is None:
            return None
        dt = ((ms - anterior[1]) % MODULO) / 1000.0
        if not 0 < dt <= 1.0:
            self.anterior = None
            return None
        deltas = [delta_pulsos(actual, previo) * factor
                  for actual, previo, factor in zip(pulsos, anterior[2], METROS_POR_PULSO)]
        if any(abs(delta) > dt for delta in deltas):
            self.anterior = None
            return None
        izq = (deltas[0] + deltas[1]) / 2
        der = (deltas[2] + deltas[3]) / 2
        tramo, giro = (der + izq) / 2, (der - izq) / trocha
        avance = tramo if abs(giro) < 1e-8 else tramo * 2 * sin(giro / 2) / giro
        self.x += avance * cos(self.yaw + giro / 2)
        self.y += avance * sin(self.yaw + giro / 2)
        self.yaw += giro
        return self.x, self.y, self.yaw, tramo / dt, giro / dt
