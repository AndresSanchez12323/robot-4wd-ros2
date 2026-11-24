#pragma once
#include <stdint.h>

// 2 ms descarta flancos repetidos muy rápidos. Con ~5 mm por pulso
// deja margen por encima de la velocidad del chasis con PWM 80.
inline bool pulsoUtil(int8_t signo, uint32_t ahoraUs, uint32_t anteriorUs) {
  return signo != 0 && ahoraUs - anteriorUs >= 2000;
}
