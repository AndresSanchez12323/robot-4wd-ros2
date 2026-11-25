#pragma once
#include <stdint.h>
#include <math.h>

struct ConfigNavegacion {
  int frente = 0;
  int radio = 0;
  bool confirmada = false;
  bool horario = true; // Angulo del LiDAR crece hacia la derecha fisica.
  bool valida() const {
    return confirmada && frente >= 0 && frente < 360 && radio >= 100 && radio <= 1000;
  }
};

// Evasion reactiva. Un sector sin retorno no detiene el carro: solo un eco real
// dentro del corredor delantero. Ante un bloqueo gira, sin cortar el pivote,
// hasta que el frente quede libre. Si esta encima del obstaculo, retrocede y sigue.
class Navegacion {
 public:
  static constexpr float ANTICIPACION_MM = 150.0f;
  static constexpr uint16_t ECO_PROPIO_MM = 80;   // cables y soporte del LiDAR
  static constexpr float ABIERTO_MM = 8000.0f;
  static constexpr int PWM = 140;       // de 255; mas torque en recta y pivote
  static constexpr int PWM_RETRO = 120;
  enum Fase {
    PARADO, RECTO, PAUSA_GIRO, IZQUIERDA, DERECHA, RETROCESO,
    ESPERANDO_ESPACIO, ESPERANDO_DATOS, FALLO
  };
  enum Siguiente { HACER_PIVOTE, HACER_RETRO };

  Fase fase = PARADO;
  int izquierda = 0, derecha = 0;
  int sentido = 1; // +1 izquierda fisica, -1 derecha fisica.
  float yawManiobra = 0;
  bool directa = false; // mision: obedece una orden y no esquiva por su cuenta
  const char *orden = "ninguna";
  const char *motivo = "Pulsa ARRANCAR";

  bool activa() const { return fase != PARADO && fase != FALLO; }
  void parar(const char *causa = "Detenido por el operador") {
    fase = PARADO; cero(); directa = false; orden = "parar"; motivo = causa;
  }
  void fallar(const char *causa) {
    fase = FALLO; cero(); directa = false; orden = "parar"; motivo = causa;
  }

  bool arrancar(const ConfigNavegacion &c, uint32_t ahora) {
    if (!c.valida()) { fallar("Configura frente, radio y sentido del LiDAR"); return false; }
    fase = RECTO; cero(); directa = false; orden = "explorar";
    inicioFase = ahora; ultimoBarrido = UINT32_MAX;
    libres = 0; sentido = 1; retroHecho = false; yawManiobra = 0; siguiente = HACER_PIVOTE;
    motivo = "Evaluando barrido antes de mover";
    return true;
  }

  // 1 explorar, 2 avanzar, 3 girar a la izquierda, 4 girar a la derecha.
  bool aplicarMision(int codigo, const ConfigNavegacion &c, uint32_t ahora) {
    if (codigo == 1) return arrancar(c, ahora);
    if (!c.valida()) { fallar("Configura frente, radio y sentido del LiDAR"); return false; }
    if (codigo < 2 || codigo > 4) return false;
    directa = true;
    libres = 0; retroHecho = false; yawManiobra = 0;
    inicioFase = ahora; ultimoBarrido = UINT32_MAX;
    if (codigo == 2) {
      fase = RECTO; izquierda = derecha = PWM; orden = "avanzar";
      motivo = "Acercandose al objetivo";
    } else if (codigo == 3) {
      sentido = 1; fase = IZQUIERDA; izquierda = -PWM; derecha = PWM;
      orden = "girar_izquierda"; motivo = "Girando a la izquierda hacia el objetivo";
    } else {
      sentido = -1; fase = DERECHA; izquierda = PWM; derecha = -PWM;
      orden = "girar_derecha"; motivo = "Girando a la derecha hacia el objetivo";
    }
    return true;
  }

  static uint16_t medida(const uint16_t *mapa, int i) {
    const uint16_t a = mapa[2 * i], b = mapa[2 * i + 1];
    return !a ? b : !b ? a : a < b ? a : b;
  }

  static float relativo(int celda, const ConfigNavegacion &c) {
    float a = celda * 10.0f + 5.0f - c.frente;
    while (a > 180.0f) a -= 360.0f;
    while (a <= -180.0f) a += 360.0f;
    return c.horario ? -a : a; // positivo = izquierda fisica
  }

  const char *nombreFase() const {
    switch (fase) {
      case PARADO: return "Parado";
      case RECTO: return "Recto";
      case PAUSA_GIRO: return "Pausa";
      case IZQUIERDA: return "Pivote izquierda";
      case DERECHA: return "Pivote derecha";
      case RETROCESO: return "Retroceso";
      case ESPERANDO_ESPACIO: return "Espera espacio";
      case ESPERANDO_DATOS: return "Espera LiDAR";
      case FALLO: return "Fallo";
    }
    return "Parado";
  }

  // nullptr si el corredor delantero esta libre. Los huecos y la parte trasera no bloquean.
  const char *obstaculoFrente(const uint16_t *mapa, const ConfigNavegacion &c) const {
    const float margen = c.radio + 100.0f;
    for (int i = 0; i < 36; ++i) {
      const float grados = relativo(i, c);
      const float rad = grados * 0.01745329252f;
      const float x = cosf(rad), y = sinf(rad);
      if (x <= 0.0f) continue;
      const uint16_t d = medida(mapa, i);
      if (d < ECO_PROPIO_MM) continue;
      const float ancho = margen + d * 0.08716f;
      const float px = d * x, py = fabsf(d * y);
      const float dx = px > ANTICIPACION_MM ? px - ANTICIPACION_MM : 0.0f;
      if (dx * dx + py * py <= ancho * ancho) return "Obstaculo en trayectoria";
    }
    return nullptr;
  }

  bool hayQueRetroceder(const uint16_t *mapa, const ConfigNavegacion &c) const {
    const float limite = c.radio + 80.0f;
    for (int i = 0; i < 36; ++i) {
      if (fabsf(relativo(i, c)) > 30.0f) continue;
      const uint16_t d = medida(mapa, i);
      if (d >= ECO_PROPIO_MM && d < limite) return true;
    }
    return false;
  }

  // Sin eco en el lado cuenta como espacio abierto, no como pared.
  float espacioLado(const uint16_t *mapa, const ConfigNavegacion &c, int lado) const {
    float espacio = ABIERTO_MM;
    for (int i = 0; i < 36; ++i) {
      const float a = relativo(i, c) * lado;
      if (a < 20.0f || a > 100.0f) continue;
      const uint16_t d = medida(mapa, i);
      if (d < ECO_PROPIO_MM) continue;
      if (d < espacio) espacio = static_cast<float>(d);
    }
    return espacio;
  }

  int elegirLado(const uint16_t *mapa, const ConfigNavegacion &c) const {
    const float l = espacioLado(mapa, c, 1), r = espacioLado(mapa, c, -1);
    if (l > r + 100.0f) return 1;
    if (r > l + 100.0f) return -1;
    return sentido;
  }

  void integrar(float gz, uint32_t ahora) {
    if (!ultimoTick) { ultimoTick = ahora; return; }
    const uint32_t dtMs = ahora - ultimoTick;
    ultimoTick = ahora;
    if (!dtMs || dtMs > 200) return;
    if (fase != IZQUIERDA && fase != DERECHA) return;
    if (fabsf(gz) < 2.0f) return;
    yawManiobra += gz * (static_cast<float>(dtMs) / 1000.0f);
  }

  void actualizar(const uint16_t *mapa, const ConfigNavegacion &c, bool sensor,
                  uint32_t barrido, uint32_t ahora) {
    if (!activa()) return;
    if (!c.valida()) { fallar("Configuracion de geometria invalida"); return; }
    if (directa) {
      if (!sensor) { parar("Mision sin LiDAR"); return; }
      if (fase == RECTO && hayQueRetroceder(mapa, c)) { parar("Parado al lado del objetivo"); return; }
      if (fase == RECTO) {
        izquierda = derecha = PWM; motivo = "Acercandose al objetivo";
      } else if (fase == IZQUIERDA || fase == DERECHA) {
        izquierda = -PWM * sentido; derecha = PWM * sentido;
        motivo = sentido > 0 ? "Girando a la izquierda hacia el objetivo"
                             : "Girando a la derecha hacia el objetivo";
      }
      return;
    }
    if (!sensor) {
      if (fase != ESPERANDO_DATOS) {
        fase = ESPERANDO_DATOS; cero(); libres = 0; inicioFase = ahora;
        motivo = "Esperando barridos validos; reanudacion automatica";
      }
      return;
    }
    const bool nuevo = barrido != ultimoBarrido;
    if (nuevo) ultimoBarrido = barrido;
    const bool libre = obstaculoFrente(mapa, c) == nullptr;

    if (nuevo && fase != RECTO) {
      if (libre) {
        if (++libres >= 2) { avanzar(); return; }
      } else {
        libres = 0;
      }
    }

    if (fase == PAUSA_GIRO) {
      if (ahora - inicioFase >= 250) {
        if (siguiente == HACER_RETRO) comenzarRetro(ahora);
        else comenzarPivote(ahora);
      }
      return;
    }
    if (fase == RETROCESO) {
      if (ahora - inicioFase >= 500) prepararGiro(sentido, ahora);
      return;
    }
    if (fase == IZQUIERDA || fase == DERECHA) {
      // El pivote no tiene tope: sigue hasta que dos barridos vean el frente libre.
      izquierda = -PWM * sentido;
      derecha = PWM * sentido;
      motivo = sentido > 0 ? "Girando a la izquierda hasta ver salida"
                           : "Girando a la derecha hasta ver salida";
      if (!libre && hayQueRetroceder(mapa, c) && !retroHecho && fabsf(yawManiobra) >= 40.0f) {
        retroHecho = true;
        pedirRetroceso(ahora);
        return;
      }
      if (fabsf(yawManiobra) >= 320.0f) {
        retroHecho = false;
        yawManiobra = 0;
      }
      return;
    }
    if (fase == ESPERANDO_DATOS || fase == ESPERANDO_ESPACIO) {
      if (!nuevo) return;
      if (libre) { motivo = "Confirmando salida"; return; }
      if (hayQueRetroceder(mapa, c)) pedirRetroceso(ahora);
      else prepararGiro(elegirLado(mapa, c), ahora);
      return;
    }
    if (fase == RECTO) {
      if (!nuevo) return;
      if (libre) { avanzar(); return; }
      sentido = elegirLado(mapa, c);
      retroHecho = false;
      yawManiobra = 0;
      if (hayQueRetroceder(mapa, c)) pedirRetroceso(ahora);
      else prepararGiro(sentido, ahora);
    }
  }

 private:
  uint32_t inicioFase = 0, ultimoBarrido = UINT32_MAX, ultimoTick = 0;
  uint8_t libres = 0;
  bool retroHecho = false;
  Siguiente siguiente = HACER_PIVOTE;

  void cero() { izquierda = derecha = 0; }
  void avanzar() {
    fase = RECTO; izquierda = derecha = PWM; libres = 0; retroHecho = false;
    motivo = "Avanzando; correccion automatica activa";
  }
  void prepararGiro(int lado, uint32_t ahora) {
    if (lado) sentido = lado;
    fase = PAUSA_GIRO; siguiente = HACER_PIVOTE; inicioFase = ahora;
    libres = 0; yawManiobra = 0; cero();
    motivo = sentido > 0 ? "Preparando giro izquierdo" : "Preparando giro derecho";
  }
  void pedirRetroceso(uint32_t ahora) {
    fase = PAUSA_GIRO; siguiente = HACER_RETRO; inicioFase = ahora;
    libres = 0; yawManiobra = 0; cero();
    motivo = "Preparando retroceso";
  }
  void comenzarPivote(uint32_t ahora) {
    fase = sentido > 0 ? IZQUIERDA : DERECHA;
    izquierda = -PWM * sentido; derecha = PWM * sentido;
    inicioFase = ahora; yawManiobra = 0; libres = 0;
    motivo = sentido > 0 ? "Corrigiendo a izquierda" : "Corrigiendo a derecha";
  }
  void comenzarRetro(uint32_t ahora) {
    fase = RETROCESO; izquierda = derecha = -PWM_RETRO;
    inicioFase = ahora; libres = 0;
    motivo = "Retrocediendo para separarme";
  }
};
