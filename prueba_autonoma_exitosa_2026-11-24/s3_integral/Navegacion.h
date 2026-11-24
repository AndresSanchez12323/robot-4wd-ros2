#pragma once
#include <stdint.h>
#include <math.h>

// Planificador reactivo conservador, independiente del hardware. Nunca retrocede.
// El radio debe incluir TODO el chasis desde el eje del LiDAR.
struct ConfigNavegacion {
  int frente = 0;
  int radio = 0;
  bool confirmada = false;
  bool valida() const { return confirmada && frente >= 0 && frente < 360 && radio >= 100 && radio <= 1000; }
};

class Navegacion {
 public:
  static constexpr float ANTICIPACION_MM = 150.0f;
  enum Fase { PARADO, RECTO, PAUSA_GIRO, IZQUIERDA, ESPERANDO_ESPACIO, FALLO };
  Fase fase = PARADO;
  int izquierda = 0, derecha = 0;
  const char *motivo = "Pulsa ARRANCAR";
  bool activa() const { return fase == RECTO || fase == PAUSA_GIRO || fase == IZQUIERDA || fase == ESPERANDO_ESPACIO; }
  void parar(const char *causa = "Detenido por el operador") {
    fase = PARADO; izquierda = derecha = 0; motivo = causa;
  }
  void fallar(const char *causa) { fase = FALLO; izquierda = derecha = 0; motivo = causa; }
  void esperar(const char *causa) {
    fase = ESPERANDO_ESPACIO; izquierda = derecha = 0; motivo = causa; libres = 0; salida = 0;
  }
  bool arrancar(const ConfigNavegacion &c, uint32_t ahora) {
    if (!c.valida()) { fallar("Configura frente y radio del chasis"); return false; }
    fase = RECTO; izquierda = derecha = 0; inicioFase = ahora; ultimoBarrido = UINT32_MAX; libres = 0;
    motivo = "Evaluando barrido antes de mover"; return true;
  }
  // 0 significa sin medida, NUNCA espacio libre. No se filtran ecos cercanos.
  const char *evaluar(const uint16_t *mapa, const ConfigNavegacion &c, bool giro = false) const {
    const float radio = c.radio + 100.0f; // margen inicial de 10 cm, por validar fisicamente.
    // Planificar en 36 celdas de 10 grados: conservar el eco menor de cada par
    // de sectores de 5 grados. Una celda sin ninguna medida sigue desconocida.
    // No se interpolan distancias ni se elimina un eco cercano.
    for (int i = 0; i < 36; ++i) {
      const uint16_t a0 = mapa[2*i], a1 = mapa[2*i+1];
      const uint16_t medida = !a0 ? a1 : !a1 ? a0 : a0 < a1 ? a0 : a1;
      float a = (i * 10.0f + 5.0f - c.frente) * 0.01745329252f;
      float x = cosf(a), y = sinf(a);
      if (medida && medida <= radio) return "Obstaculo dentro del margen del chasis";
      if (giro && !medida) return "Giro bloqueado: falta observar una zona del LiDAR";
      if (giro) continue;
      if (x <= 0) continue;
      if (!medida) return "Falta medida en la zona de avance";
      // Capsula barrida: circulo del chasis desplazado 15 cm hacia adelante.
      // Agrandar por media apertura del sector para no tratar el centro como un rayo exacto.
      const float d = medida;
      const float ancho = radio + d * 0.08716f;
      const float px = d * x, py = fabsf(d * y);
      const float dx = px > ANTICIPACION_MM ? px - ANTICIPACION_MM : 0.0f;
      if (dx * dx + py * py <= ancho * ancho) return "Obstaculo en trayectoria: buscando giro";
    }
    return nullptr;
  }
  void actualizar(const uint16_t *mapa, const ConfigNavegacion &c, bool sensor,
                  uint32_t barrido, uint32_t ahora) {
    if (!activa()) return;
    if (!sensor) { fallar("LiDAR sin barrido valido reciente"); return; }
    if (!c.valida()) { fallar("Configuracion de geometria invalida"); return; }
    if ((fase == IZQUIERDA || fase == PAUSA_GIRO) && ahora - inicioFase > 6500) {
      fallar("No se encontro salida durante el giro"); return;
    }
    if (barrido == ultimoBarrido) return;
    ultimoBarrido = barrido;
    const char *frente = evaluar(mapa, c);
    const char *giro = evaluar(mapa, c, true);
    if (fase == ESPERANDO_ESPACIO) {
      // Una obstruccion con LiDAR sano no cancela la corrida. Tres barridos
      // nuevos confirman la salida; un STOP o fallo real siguen cancelandola.
      if (!frente) {
        if (salida != 1) libres = 0;
        salida = 1;
        if (++libres >= 3) {
          fase = RECTO; izquierda = derecha = 80;
          motivo = "Espacio libre: avance reanudado automaticamente";
        }
      } else if (!giro) {
        if (salida != 2) libres = 0;
        salida = 2;
        if (++libres >= 3) {
          fase = PAUSA_GIRO; inicioFase = ahora; libres = 0;
          motivo = "Espacio para giro recuperado: esquivando";
        }
      } else { libres = 0; salida = 0; motivo = giro; }
      return;
    }
    if (fase == RECTO) {
      if (!frente) { izquierda = derecha = 80; motivo = "Avanzando sin limite de distancia"; return; }
      izquierda = derecha = 0;
      if (giro) { esperar(giro); return; }
      fase = PAUSA_GIRO; inicioFase = ahora; libres = 0;
      motivo = "Pausa antes de girar a la izquierda"; return;
    }
    if (giro) { esperar(giro); return; }
    if (fase == PAUSA_GIRO) {
      if (ahora - inicioFase >= 250) { fase = IZQUIERDA; izquierda = -80; derecha = 80; motivo = "Girando a la izquierda para esquivar"; }
      return;
    }
    if (!frente) ++libres; else libres = 0;
    if (libres >= 3) {
      fase = RECTO; izquierda = derecha = 80; motivo = "Salida libre: retomando avance";
      return;
    }
    izquierda = -80; derecha = 80;
  }
 private:
  uint32_t inicioFase = 0, ultimoBarrido = UINT32_MAX;
  uint8_t libres = 0, salida = 0;
};
