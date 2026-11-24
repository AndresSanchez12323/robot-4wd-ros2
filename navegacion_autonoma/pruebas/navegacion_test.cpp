#include "../s3_integral/Navegacion.h"
#include "../devkit_integral/GuardiaMarcha.h"
#include <cassert>
#include <algorithm>
#include <iostream>
#include <cmath>

int main() {
  uint16_t mapa[72]; std::fill_n(mapa, 72, 2000);
  ConfigNavegacion c{0, 150, true}; Navegacion n;
  assert(!n.arrancar({0, 150, false}, 0));
  assert(!n.arrancar({360, 150, true}, 0));
  assert(n.arrancar(c, 0));
  for (uint32_t i = 1; i < 2000; ++i) {
    n.actualizar(mapa, c, true, i, i * 100);
    assert(n.activa() && n.izquierda == Navegacion::PWM && n.derecha == Navegacion::PWM);
  }
  // Huecos detras no frenan la recta.
  mapa[40] = mapa[41] = mapa[50] = mapa[51] = 0;
  n.actualizar(mapa, c, true, 2000, 200000);
  assert(n.fase == Navegacion::RECTO && n.izquierda == Navegacion::PWM);

  // Eco del propio soporte, mas cerca de 80 mm, no es un obstaculo.
  std::fill_n(mapa, 72, 2000); mapa[10] = 54;
  n.actualizar(mapa, c, true, 2001, 200100);
  assert(n.fase == Navegacion::RECTO && n.izquierda == Navegacion::PWM);

  // Obstaculo a 40 cm: pivota. Un hueco trasero durante el giro no lo para.
  mapa[10] = 2000; mapa[0] = 400; mapa[40] = mapa[41] = 0;
  n.actualizar(mapa, c, true, 2002, 200200);
  assert(n.fase == Navegacion::PAUSA_GIRO && !n.izquierda && !n.derecha);
  n.actualizar(mapa, c, true, 2003, 200400);
  assert(!n.izquierda);
  n.actualizar(mapa, c, true, 2004, 200500);
  assert(n.fase == Navegacion::IZQUIERDA && n.izquierda == -Navegacion::PWM && n.derecha == Navegacion::PWM);
  mapa[40] = mapa[41] = 0;
  n.actualizar(mapa, c, true, 2005, 200600);
  assert(n.fase == Navegacion::IZQUIERDA && n.izquierda == -Navegacion::PWM);

  // Frente libre dos barridos: vuelve a avanzar, aunque el de atras siga vacio.
  std::fill_n(mapa, 72, 2000); mapa[40] = mapa[41] = 0;
  n.actualizar(mapa, c, true, 2006, 200700);
  assert(n.fase == Navegacion::IZQUIERDA);
  n.actualizar(mapa, c, true, 2007, 200800);
  assert(n.fase == Navegacion::RECTO && n.izquierda == Navegacion::PWM && n.derecha == Navegacion::PWM);

  // Lado izquierdo abierto (sin ecos) y pared a la derecha: gira a la izquierda.
  std::fill_n(mapa, 72, 0); mapa[0] = mapa[1] = 400;
  for (int i = 0; i < 36; ++i) {
    const float a = Navegacion::relativo(i, c);
    if (a <= -20.0f && a >= -100.0f) mapa[2 * i] = mapa[2 * i + 1] = 500;
  }
  Navegacion lado; lado.arrancar(c, 0);
  lado.actualizar(mapa, c, true, 1, 100);
  assert(lado.fase == Navegacion::PAUSA_GIRO && lado.sentido == 1);
  lado.actualizar(mapa, c, true, 2, 400);
  assert(lado.fase == Navegacion::IZQUIERDA && lado.izquierda == -Navegacion::PWM && lado.derecha == Navegacion::PWM);

  // Obstaculo a 18 cm: primero retrocede.
  std::fill_n(mapa, 72, 2000); mapa[0] = mapa[1] = 180;
  Navegacion cerca; cerca.arrancar(c, 0);
  cerca.actualizar(mapa, c, true, 1, 100);
  assert(cerca.fase == Navegacion::PAUSA_GIRO && !cerca.izquierda);
  cerca.actualizar(mapa, c, true, 2, 400);
  assert(cerca.fase == Navegacion::RETROCESO && cerca.izquierda == -Navegacion::PWM_RETRO && cerca.derecha == -Navegacion::PWM_RETRO);
  cerca.actualizar(mapa, c, true, 3, 950);
  assert(cerca.activa() && cerca.fase == Navegacion::PAUSA_GIRO);

  // El giro no se corta a los 55 grados ni a los varios segundos: sigue hasta ver salida.
  std::fill_n(mapa, 72, 2000); mapa[0] = 400;
  Navegacion giro; giro.arrancar(c, 0);
  giro.actualizar(mapa, c, true, 1, 100);
  giro.actualizar(mapa, c, true, 2, 400);
  assert(giro.fase == Navegacion::IZQUIERDA);
  giro.integrar(0, 400);
  for (uint32_t t = 450; t <= 900; t += 50) giro.integrar(120.0f, t);
  assert(std::fabs(giro.yawManiobra) >= 55.0f);
  giro.actualizar(mapa, c, true, 3, 900);
  assert(giro.activa() && giro.fase == Navegacion::IZQUIERDA && giro.sentido == 1);
  assert(giro.izquierda == -Navegacion::PWM && giro.derecha == Navegacion::PWM);
  for (uint32_t t = 950; t <= 5000; t += 100) {
    giro.integrar(90.0f, t);
    giro.actualizar(mapa, c, true, 4 + (t - 950) / 100, t);
    assert(giro.activa() && giro.izquierda == -Navegacion::PWM && giro.derecha == Navegacion::PWM);
  }
  std::fill_n(mapa, 72, 2000);
  giro.actualizar(mapa, c, true, 80, 5200);
  assert(giro.fase == Navegacion::IZQUIERDA && giro.izquierda == -Navegacion::PWM);
  giro.actualizar(mapa, c, true, 81, 5400);
  assert(giro.fase == Navegacion::RECTO && giro.izquierda == Navegacion::PWM && giro.derecha == Navegacion::PWM);

  // Sensor caido: velocidades a cero y la misma corrida. Dos barridos nuevos reanudan.
  std::fill_n(mapa, 72, 2000);
  n.parar(); n.arrancar(c, 0); n.actualizar(mapa, c, true, 1, 100); assert(n.izquierda == Navegacion::PWM);
  n.actualizar(mapa, c, false, 1, 200); assert(n.fase == Navegacion::ESPERANDO_DATOS && !n.izquierda);
  n.actualizar(mapa, c, true, 1, 300); assert(!n.izquierda);
  n.actualizar(mapa, c, true, 2, 400); assert(!n.izquierda);
  n.actualizar(mapa, c, true, 3, 500); assert(n.izquierda == Navegacion::PWM && n.derecha == Navegacion::PWM);

  // 60 cm por delante no obliga a esquivar. 40 cm si.
  mapa[0] = 600; n.actualizar(mapa, c, true, 4, 600); assert(n.izquierda == Navegacion::PWM && n.derecha == Navegacion::PWM);
  mapa[0] = 400; n.actualizar(mapa, c, true, 5, 700); assert(n.fase == Navegacion::PAUSA_GIRO);

  n.parar(); n.actualizar(mapa, c, true, 6, 800); assert(!n.activa() && !n.izquierda);
  GuardiaMarcha g;
  assert(!g.aceptar(123, 1, 1, 0));
  assert(g.aceptar(123, 1, 0, 0)); assert(g.aceptar(123, 2, 1, 100));
  assert(g.vigente(550) && !g.vigente(551));
  g.bloquear(); assert(!g.aceptar(123, 3, 1, 600));
  assert(g.aceptar(123, 4, 0, 700)); assert(!g.aceptar(123, 5, 1, 800));
  assert(g.aceptar(123, 6, 2, 900)); assert(!g.aceptar(123, 6, 2, 1000));
  assert(!g.aceptar(124, 1, 3, 1100)); assert(g.aceptar(124, 1, 0, 1100));
  assert(g.aceptar(124, 2, 1, 1200));
  // Barrido real de la sala: huecos, eco del costado y obstaculo a 383 mm.
  // Tiene que girar y seguir girando, no quedarse en cero.
  const uint16_t sala[72] = {
    1254,1267,602,456,383,1968,1717,1512,968,896,0,0,126,131,120,114,114,115,115,114,112,114,
    0,0,0,0,0,0,0,0,0,2430,2570,3367,4725,0,0,6472,6494,0,2563,2633,2732,11178,9984,9251,
    4989,8020,7672,7354,7298,8798,0,0,6652,6647,5396,3946,4212,4562,4318,0,0,0,1652,1599,1652,0,0,2497,1455,1358
  };
  Navegacion real; real.arrancar(c, 0);
  for (uint32_t i = 1; i <= 40; ++i) real.actualizar(sala, c, true, i, i * 200);
  assert(real.activa());
  assert(real.fase == Navegacion::IZQUIERDA || real.fase == Navegacion::DERECHA);
  assert(real.izquierda != 0 && real.derecha != 0);
  std::cout << "OK: recta con huecos, pivote continuo, retroceso, yaw, sala real y STOP\n";
}
