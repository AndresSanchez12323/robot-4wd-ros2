#include "../s3_integral/Navegacion.h"
#include "../s3_integral/SupervisorMarcha.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>

void llenarLado(uint16_t *mapa,const ConfigNavegacion &c,int lado,uint16_t d) {
  for(int i=0;i<36;++i) {
    const float a=Navegacion::relativo(i,c)*lado;
    if(a>=20 && a<=100) mapa[2*i]=mapa[2*i+1]=d;
  }
}
void frente(uint16_t *mapa,int grados,uint16_t d) {
  const int i=(grados/10)%36;
  mapa[2*i]=mapa[2*i+1]=d;
}
int main() {
  // El frente no siempre coincide con cero. Ambos montajes angulares.
  for(bool horario : {false,true}) for(int angulo : {0,15,90,180,270,307,359}) {
    ConfigNavegacion c{angulo,150,true,horario};
    for(int salida : {-1,1}) {
      uint16_t mapa[72];std::fill_n(mapa,72,2000);
      llenarLado(mapa,c,-salida,600); frente(mapa,angulo,400);
      Navegacion n;n.arrancar(c,0);n.actualizar(mapa,c,true,1,100);
      assert(n.fase==Navegacion::PAUSA_GIRO && n.sentido==salida);
      n.actualizar(mapa,c,true,2,400);
      assert(n.izquierda==-80*salida && n.derecha==80*salida);
      assert(n.fase==(salida>0?Navegacion::IZQUIERDA:Navegacion::DERECHA));
      // Corregir largo ya no obliga a ARRANCAR otra vez.
      for(unsigned k=3;k<150;++k) {
        n.actualizar(mapa,c,true,k,k*300);
        assert(n.activa());
      }
      std::fill_n(mapa,72,2000);
      // Si esta en PAUSA puede necesitar 250ms antes de los tres barridos libres.
      for(unsigned k=150;k<157;++k)n.actualizar(mapa,c,true,k,k*300);
      assert(n.fase==Navegacion::RECTO && n.izquierda==80 && n.derecha==80);
    }
  }
  ConfigNavegacion c{0,150,true};uint16_t mapa[72];std::fill_n(mapa,72,2000);
  Navegacion n;n.arrancar(c,0);n.actualizar(mapa,c,true,1,100);
  // Sin barrido valido: nunca conservar ultimo PWM. Recuperacion con datos NUEVOS.
  n.actualizar(mapa,c,false,1,700);assert(n.activa() && !n.izquierda && !n.derecha);
  for(unsigned t=800;t<1200;t+=50)n.actualizar(mapa,c,true,1,t);
  assert(!n.izquierda); // no reciclar el ultimo barrido.
  n.actualizar(mapa,c,true,2,1200);n.actualizar(mapa,c,true,3,1300);
  assert(!n.izquierda);n.actualizar(mapa,c,true,4,1400);assert(n.izquierda==80);
  // Reinicio del contador de barridos tras reinicio del sensor.
  n.actualizar(mapa,c,false,1000,1500);
  for(unsigned k=1;k<=3;++k)n.actualizar(mapa,c,true,k,1600+k*100);
  assert(n.izquierda==80);
  // Desconocidos persistentes no son via libre ni obligan a pulsar ARRANCAR.
  std::fill_n(mapa,72,0);n.actualizar(mapa,c,true,5,2000);
  for(unsigned k=6;k<50;++k)n.actualizar(mapa,c,true,k,k*500);
  assert(n.activa() && !n.izquierda && !n.derecha);
  n.parar();std::fill_n(mapa,72,2000);
  for(unsigned k=50;k<60;++k)n.actualizar(mapa,c,true,k,k*500);
  assert(!n.activa() && !n.izquierda); // STOP no se recupera solo.
  n.arrancar(c,0);n.fallar("FALLO_REAL");
  for(unsigned k=1;k<5;++k)n.actualizar(mapa,c,true,k,k*100);
  assert(!n.activa());
  // Escena sintetica que cambia con el giro: obstaculo frontal se desplaza
  // en el marco del sensor. Ambos sentidos encuentran frente libre y continuan.
  for(int lado : {-1,1}) {
    std::fill_n(mapa,72,2000); llenarLado(mapa,c,-lado,600); frente(mapa,0,400);
    Navegacion robot;robot.arrancar(c,0);robot.actualizar(mapa,c,true,1,100);
    robot.actualizar(mapa,c,true,2,400);assert(robot.sentido==lado);
    for(unsigned k=3;k<=18;++k) {
      std::fill_n(mapa,72,2000);
      // Hasta superar el giro el obstaculo sigue por delante; luego queda detras.
      frente(mapa,k<9 ? (lado>0?int((k-3)*10):int(360-(k-3)*10))%360 : 180,400);
      robot.actualizar(mapa,c,true,k,400+(k-2)*100);
    }
    assert(robot.fase==Navegacion::RECTO && robot.izquierda==80 && robot.derecha==80);
  }
  // Supervisor: todas las combinaciones de salud de servidor, operador, UART,
  // MPU y corrida bloqueada. No permite continuar ante un fallo real.
  for(unsigned mask=0;mask<32;++mask) {
    bool servidor=mask&1,operador=mask&2,devkit=mask&4,imu=mask&8,bloqueada=mask&16;
    const char *causa=causaParada(servidor,operador,devkit,imu,bloqueada,"ENCODER_FR_SIN_PULSOS");
    assert((causa==nullptr)==(servidor&&operador&&devkit&&imu&&!bloqueada));
    if(servidor&&operador&&devkit&&bloqueada)assert(!strcmp(causa,"ENCODER_FR_SIN_PULSOS"));
  }
  std::cout<<"OK: 28 casos izquierda/derecha, 2 escenas variables, 32 combinaciones de salud, larga correccion, desconocidos, sensor recuperado, STOP y fallo real\n";
}
