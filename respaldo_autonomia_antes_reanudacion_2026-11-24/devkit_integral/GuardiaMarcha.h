#pragma once
#include <stdint.h>

// Cada ARRANCAR crea una corrida nueva. Un fallo/STOP invalida esa corrida:
// recibir otra vez la misma orden no vuelve a poner los motores en marcha.
class GuardiaMarcha {
 public:
  uint32_t sesion = 0, secuencia = 0, corrida = 0, ultimaCorrida = 0, bloqueada = 0, ultimo = 0;
  bool recibido = false;
  void bloquear() { if (corrida) bloqueada = corrida; corrida = 0; }
  bool vigente(uint32_t ahora) const { return recibido && ahora - ultimo <= 450; }
  bool aceptar(uint32_t s, uint32_t q, uint32_t r, uint32_t ahora) {
    if (!s) return false;
    if (s != sesion) {
      if (r != 0) return false; // reinicio requiere primero sincronizacion en reposo.
      sesion = s; secuencia = 0; corrida = ultimaCorrida = bloqueada = 0; recibido = false;
    }
    if (static_cast<int32_t>(q - secuencia) <= 0) return false;
    if (r && (r == bloqueada || static_cast<int32_t>(r - ultimaCorrida) < 0)) return false;
    secuencia = q; ultimo = ahora; recibido = true;
    if (!r) bloquear();
    else { corrida = r; ultimaCorrida = r; }
    return true;
  }
};
