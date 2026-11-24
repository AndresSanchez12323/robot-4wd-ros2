#pragma once
#include <stdint.h>
#include <math.h>

struct ConfigNavegacion {
  int frente = 0;
  int radio = 0;
  bool confirmada = false;
  bool horario = true; // Verificar con objeto: angulo crece hacia la derecha fisica.
  bool valida() const { return confirmada && frente >= 0 && frente < 360 && radio >= 100 && radio <= 1000; }
};

// Planificador reactivo: conserva la corrida durante esperas recuperables.
class Navegacion {
 public:
  static constexpr float ANTICIPACION_MM = 150.0f;
  enum Fase { PARADO, RECTO, PAUSA_GIRO, IZQUIERDA, DERECHA, ESPERANDO_ESPACIO, ESPERANDO_DATOS, FALLO };
  Fase fase = PARADO;
  int izquierda = 0, derecha = 0;
  int sentido = 1; // +1 izquierda fisica, -1 derecha fisica.
  const char *motivo = "Pulsa ARRANCAR";
  bool activa() const { return fase != PARADO && fase != FALLO; }
  void parar(const char *causa = "Detenido por el operador") { fase = PARADO; cero(); motivo = causa; }
  void fallar(const char *causa) { fase = FALLO; cero(); motivo = causa; }
  void esperar(const char *causa) { fase = ESPERANDO_ESPACIO; cero(); motivo = causa; libres = salida = 0; }
  bool arrancar(const ConfigNavegacion &c, uint32_t ahora) {
    if (!c.valida()) { fallar("Configura frente, radio y sentido del LiDAR"); return false; }
    fase = RECTO; cero(); inicioFase = ahora; ultimoBarrido = UINT32_MAX;
    libres = salida = 0; sentido = 1; motivo = "Evaluando barrido antes de mover"; return true;
  }
  static uint16_t medida(const uint16_t *mapa, int i) {
    const uint16_t a = mapa[2*i], b = mapa[2*i+1];
    return !a ? b : !b ? a : a < b ? a : b;
  }
  static float relativo(int celda, const ConfigNavegacion &c) {
    float a = celda * 10.0f + 5.0f - c.frente;
    while (a > 180) a -= 360;
    while (a <= -180) a += 360;
    return c.horario ? -a : a; // positivo = izquierda fisica.
  }
  const char *evaluar(const uint16_t *mapa, const ConfigNavegacion &c, bool giro = false) const {
    const float radio = c.radio + 100.0f;
    for (int i = 0; i < 36; ++i) {
      const uint16_t d = medida(mapa,i);
      const float a = relativo(i,c) * 0.01745329252f;
      const float x = cosf(a), y = sinf(a);
      if (d && d <= radio) return "Obstaculo dentro del margen del chasis";
      if (giro && !d) return "Falta observar zona de giro";
      if (giro || x <= 0) continue;
      if (!d) return "Falta medida en la zona de avance";
      const float ancho = radio + d * 0.08716f;
      const float px = d*x, py = fabsf(d*y);
      const float dx = px > ANTICIPACION_MM ? px-ANTICIPACION_MM : 0.0f;
      if (dx*dx + py*py <= ancho*ancho) return "Obstaculo en trayectoria";
    }
    return nullptr;
  }
  float espacioLado(const uint16_t *mapa, const ConfigNavegacion &c, int lado) const {
    float espacio = 2000.0f;
    unsigned vistos = 0;
    for (int i=0;i<36;++i) {
      const float a = relativo(i,c)*lado;
      if (a < 20 || a > 100) continue;
      const uint16_t d = medida(mapa,i);
      if (!d) return 0; // desconocido no es una salida.
      if (d < espacio) espacio = d;
      ++vistos;
    }
    return vistos >= 6 ? espacio : 0;
  }
  int elegirLado(const uint16_t *mapa, const ConfigNavegacion &c) const {
    const float l = espacioLado(mapa,c,1), r = espacioLado(mapa,c,-1);
    // Histeresis de 10 cm: en empate conservar decision, sin zigzag por ruido.
    if (l > r + 100) return 1;
    if (r > l + 100) return -1;
    return sentido;
  }
  void prepararGiro(int lado, uint32_t ahora) {
    sentido = lado; fase = PAUSA_GIRO; inicioFase = ahora; libres = 0; cero();
    motivo = lado > 0 ? "Preparando giro izquierdo" : "Preparando giro derecho";
  }
  void actualizar(const uint16_t *mapa, const ConfigNavegacion &c, bool sensor,
                  uint32_t barrido, uint32_t ahora) {
    if (!activa()) return;
    if (!c.valida()) { fallar("Configuracion de geometria invalida"); return; }
    if (!sensor) {
      fase = ESPERANDO_DATOS; cero(); libres = salida = 0;
      ultimoBarrido = barrido; motivo = "Esperando barridos validos; reanudacion automatica"; return;
    }
    if (barrido == ultimoBarrido) return;
    ultimoBarrido = barrido;
    const char *frente = evaluar(mapa,c);
    const char *giro = evaluar(mapa,c,true);
    if (fase == ESPERANDO_DATOS || fase == ESPERANDO_ESPACIO) {
      const uint8_t candidata = !frente ? 1 : !giro ? 2 : 0;
      if (!candidata) { libres = salida = 0; motivo = giro; return; }
      if (salida != candidata) libres = 0;
      salida = candidata;
      if (++libres < 3) { motivo = "Confirmando salida en tres barridos nuevos"; return; }
      if (candidata == 1) avanzar();
      else prepararGiro(elegirLado(mapa,c),ahora);
      return;
    }
    if (fase == RECTO) {
      if (!frente) { avanzar(); return; }
      if (giro) { esperar(giro); return; }
      prepararGiro(elegirLado(mapa,c),ahora); return;
    }
    if (giro) { esperar(giro); return; }
    // Sin salida frontal no cancelar corrida a los 6,5 s: probar el otro lado
    // con envolvente libre, pausa de inversion y mismo identificador de corrida.
    if (ahora - inicioFase > 6500) {
      prepararGiro(-sentido,ahora); motivo = "Reevaluando: probando el otro lado"; return;
    }
    if (fase == PAUSA_GIRO) {
      if (ahora - inicioFase >= 250) pivotar();
      return;
    }
    if (!frente) ++libres; else libres = 0;
    if (libres >= 3) { avanzar(); return; }
    pivotar();
  }
 private:
  uint32_t inicioFase = 0, ultimoBarrido = UINT32_MAX;
  uint8_t libres = 0, salida = 0;
  void cero() { izquierda = derecha = 0; }
  void avanzar() { fase = RECTO; izquierda = derecha = 80; motivo = "Avanzando; correccion automatica activa"; }
  void pivotar() {
    fase = sentido > 0 ? IZQUIERDA : DERECHA;
    izquierda = -80 * sentido; derecha = 80 * sentido;
    motivo = sentido > 0 ? "Corrigiendo a izquierda" : "Corrigiendo a derecha";
  }
};
