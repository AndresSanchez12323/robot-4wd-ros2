#include "../s3_integral/Navegacion.h"
#include "../devkit_integral/GuardiaMarcha.h"
#include <cassert>
#include <algorithm>
#include <iostream>

int main() {
  uint16_t mapa[72]; std::fill_n(mapa,72,2000);
  ConfigNavegacion c{0,150,true}; Navegacion n;
  assert(!n.arrancar({0,150,false},0));
  assert(!n.arrancar({360,150,true},0));
  assert(n.arrancar(c,0));
  for (uint32_t i=1;i<2000;++i) {
    n.actualizar(mapa,c,true,i,i*100);
    assert(n.activa() && n.izquierda==80 && n.derecha==80);
  } // mas de 3 minutos: sin limite de tiempo o centimetros.
  mapa[0]=400;
  n.actualizar(mapa,c,true,2000,200000);
  assert(n.fase==Navegacion::PAUSA_GIRO && !n.izquierda && !n.derecha);
  n.actualizar(mapa,c,true,2001,200200); assert(!n.izquierda);
  n.actualizar(mapa,c,true,2002,200300);
  assert(n.fase==Navegacion::IZQUIERDA && n.izquierda==-80 && n.derecha==80);
  mapa[0]=2000;
  n.actualizar(mapa,c,true,2003,200400);
  n.actualizar(mapa,c,true,2003,200500); // mismo barrido no vale como confirmacion nueva.
  assert(n.fase==Navegacion::IZQUIERDA);
  n.actualizar(mapa,c,true,2004,200600);
  n.actualizar(mapa,c,true,2005,200700);
  assert(n.fase==Navegacion::RECTO && n.izquierda==80 && n.derecha==80);
  n.actualizar(mapa,c,true,2006,200800); assert(n.izquierda==80);
  n.parar(); n.actualizar(mapa,c,true,2007,200900); assert(!n.activa() && !n.izquierda);
  n.arrancar(c,0); n.actualizar(mapa,c,false,1,1); assert(n.fase==Navegacion::FALLO);
  n.actualizar(mapa,c,true,2,100); assert(!n.activa()); // no rearme automatico.
  mapa[40]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // ecos cercanos nunca se ocultan.
  mapa[40]=2000; mapa[0]=mapa[1]=0; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // cero frontal no significa libre.
  mapa[0]=400; mapa[1]=2000; mapa[40]=mapa[41]=0; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // zona de giro sin observar.
  mapa[40]=mapa[41]=2000; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  n.actualizar(mapa,c,true,2,400); n.actualizar(mapa,c,true,3,6701);
  assert(!n.activa()); // atrapado, no girar indefinidamente.
  mapa[0]=2000; mapa[18]=400; c.frente=90;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.fase==Navegacion::PAUSA_GIRO);
  // Falta trasera no impide avanzar, pero nunca se considera libre para pivotar.
  c.frente=0; mapa[18]=2000; mapa[40]=mapa[41]=0;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.izquierda==80);
  // Cuatro huecos aislados: cada celda conserva una medida real y el margen angular.
  std::fill_n(mapa,72,2000); mapa[0]=400;
  mapa[11]=mapa[21]=mapa[41]=mapa[51]=0;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.fase==Navegacion::PAUSA_GIRO);
  mapa[10]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda);
  // Recuperar espacio reanuda dentro de la misma corrida y sin nuevo ARRANCAR.
  std::fill_n(mapa,72,2000);
  n.actualizar(mapa,c,true,2,200); n.actualizar(mapa,c,true,2,210);
  assert(!n.izquierda);
  n.actualizar(mapa,c,true,3,300); assert(!n.izquierda);
  n.actualizar(mapa,c,true,4,400); assert(n.izquierda==80 && n.derecha==80);
  // Falta de zona durante el giro: espera y retoma giro con tres barridos.
  mapa[0]=400; n.actualizar(mapa,c,true,5,500);
  n.actualizar(mapa,c,true,6,800); assert(n.fase==Navegacion::IZQUIERDA);
  mapa[40]=mapa[41]=0; n.actualizar(mapa,c,true,7,900);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO);
  mapa[40]=mapa[41]=2000;
  n.actualizar(mapa,c,true,8,1000); n.actualizar(mapa,c,true,9,1100);
  n.actualizar(mapa,c,true,10,1200); assert(n.fase==Navegacion::PAUSA_GIRO);
  n.actualizar(mapa,c,true,11,1500); assert(n.izquierda==-80 && n.derecha==80);
  // STOP durante espera SIEMPRE cancela el rearme automatico.
  mapa[40]=54; n.actualizar(mapa,c,true,12,1600); n.parar();
  std::fill_n(mapa,72,2000);
  for(unsigned i=13;i<20;++i) n.actualizar(mapa,c,true,i,i*200);
  assert(!n.activa() && !n.izquierda && !n.derecha);
  // Fallo de LiDAR durante espera sigue siendo un fallo real enclavado.
  mapa[40]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  n.actualizar(mapa,c,false,2,200); assert(n.fase==Navegacion::FALLO);
  // Anticipacion reducida: 600 mm ya no bloquea el avance; 400 mm aun exige evasión.
  std::fill_n(mapa,72,2000); mapa[0]=600;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.izquierda==80 && n.derecha==80);
  mapa[0]=400; n.actualizar(mapa,c,true,2,200); assert(n.fase==Navegacion::PAUSA_GIRO);
  GuardiaMarcha g;
  assert(!g.aceptar(123,1,1,0)); // nueva sesion primero en reposo.
  assert(g.aceptar(123,1,0,0)); assert(g.aceptar(123,2,1,100));
  assert(g.vigente(550) && !g.vigente(551));
  g.bloquear(); assert(!g.aceptar(123,3,1,600));
  assert(g.aceptar(123,4,0,700)); assert(!g.aceptar(123,5,1,800));
  assert(g.aceptar(123,6,2,900)); assert(!g.aceptar(123,6,2,1000));
  assert(!g.aceptar(124,1,3,1100)); assert(g.aceptar(124,1,0,1100));
  assert(g.aceptar(124,2,1,1200));
  std::cout << "OK: marcha continua, giro, salida estable, STOP, geometria, ceros, ecos cercanos, caducidad y rearme\n";
}
