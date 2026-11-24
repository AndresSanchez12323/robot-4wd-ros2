#include <cassert>
#include <initializer_list>
#include "../firmware/s3_integral/GuardiaROS.h"
#include "../firmware/s3_integral/LidarD6.h"
#include "../firmware/s3_integral/Navegacion.h"
#include "../firmware/devkit_integral/EncoderUtil.h"

int main() {
  assert(!pulsoUtil(0, 3000, 0));
  assert(!pulsoUtil(1, 1999, 0));
  assert(pulsoUtil(1, 2000, 0));
  assert(pulsoUtil(-1, 1500, UINT32_MAX - 700));
  GuardiaROS guardia;
  assert(!guardia.vigente(0));
  assert(!guardia.aceptar({2, 1, 10, 1, 0}, 1, 10, false, true));
  assert(!guardia.aceptar({1, 1, 10, 1, 0}, 1, 261, false, true));
  assert(!guardia.aceptar({1, 1, 10, 1, 0}, 1, 15, false, false));
  assert(!guardia.aceptar({1, 1, 10, 1, 0}, 1, 15, false, true));
  assert(guardia.aceptar({1, 2, 15, 1, -1}, 1, 16, false, true));
  assert(guardia.preferencia == -1);
  assert(!guardia.aceptar({1, 3, 40, 1, 0}, 1, 41, true, true));
  assert(guardia.aceptar({1, 4, 100, 0, 1}, 1, 100, true, true));
  assert(guardia.vigente(999) && !guardia.vigente(1000));
  assert(!guardia.aceptar({1, 5, 1000, 0, 1}, 1, 1000, true, true));
  guardia.parar();
  assert(!guardia.aceptar({1, 6, 1100, 0, 1}, 1, 1100, false, true));
  assert(guardia.aceptar({1, 7, 1100, 1, 0}, 1, 1100, false, true));
  guardia.parar();
  assert(!guardia.vigente(1100));

  Navegacion nav;
  const ConfigNavegacion config{0, 150, true, false};
  uint16_t mapa[72];
  for (auto &d : mapa) d = 1500;
  nav.preferenciaROS = -1;
  assert(nav.elegirLado(mapa, config) == -1);
  for (int i = 4; i < 20; ++i) mapa[i] = 1800;
  assert(nav.elegirLado(mapa, config) == 1);
  mapa[0] = mapa[1] = 0;
  assert(nav.evaluar(mapa, config, true));
  mapa[0] = 120;
  assert(nav.evaluar(mapa, config));

  uint16_t previo[72], avance[72];
  for (auto &d : mapa) d = 1500;
  for (auto &d : previo) d = 1500;
  mapa[14] = mapa[15] = 0;
  LidarD6::combinarAvance(mapa, previo, avance, true);
  assert(nav.evaluar(mapa, config) && !nav.evaluar(avance, config));
  assert(nav.arrancar(config, 0));
  nav.actualizar(mapa, config, true, 1, 100, avance);
  assert(nav.fase == Navegacion::RECTO && nav.izquierda > 0 && nav.derecha > 0);
  nav.actualizar(mapa, config, true, 2, 200, mapa);
  assert(nav.fase == Navegacion::ESPERANDO_ESPACIO && nav.izquierda == 0 && nav.derecha == 0);
  LidarD6::combinarAvance(mapa, previo, avance, false);
  assert(nav.evaluar(avance, config));
  previo[14] = 300;
  LidarD6::combinarAvance(mapa, previo, avance, true);
  assert(nav.evaluar(avance, config));
  mapa[14] = 200;
  previo[14] = 1500;
  LidarD6::combinarAvance(mapa, previo, avance, true);
  assert(nav.evaluar(avance, config));

  for (auto &d : mapa) d = 1500;
  mapa[0] = 320;
  assert(nav.evaluar(mapa, config) && !nav.evaluar(mapa, config, false, true));
  mapa[36] = 320;
  assert(nav.evaluar(mapa, config, false, true));
  mapa[36] = 1500;
  for (bool perderCobertura : {false, true}) {
    assert(nav.arrancar(config, 0));
    nav.actualizar(mapa, config, true, 1, 100);
    nav.actualizar(mapa, config, true, 2, 400);
    nav.actualizar(mapa, config, true, 3, 6701);
    assert(nav.fase == Navegacion::PAUSA_RETROCESO && nav.izquierda == 0);
    nav.actualizar(mapa, config, true, 4, 6801);
    nav.actualizar(mapa, config, true, 5, 6901);
    nav.actualizar(mapa, config, true, 6, 7001);
    assert(nav.fase == Navegacion::RETROCESO && nav.izquierda < 0 && nav.derecha < 0);
    if (perderCobertura) {
      mapa[36] = mapa[37] = 0;
      nav.actualizar(mapa, config, true, 7, 7101);
      mapa[36] = mapa[37] = 1500;
    } else {
      nav.actualizar(mapa, config, true, 6, 7351);
    }
    assert(nav.fase == Navegacion::ESPERANDO_ESPACIO && nav.izquierda == 0 && nav.derecha == 0);
  }
}
