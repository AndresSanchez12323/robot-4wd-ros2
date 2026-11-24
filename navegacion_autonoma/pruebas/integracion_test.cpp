#include "../devkit_integral/devkit_integral.ino"
#include "../s3_integral/Navegacion.h"
#include <algorithm>
#include <cassert>
#include <iostream>

int main() {
  ConfigNavegacion c{0,150,true}; Navegacion n;
  uint16_t mapa[72]; std::fill_n(mapa,72,2000);
  unsigned seq=0, barrido=0;
  auto enviar=[&](unsigned corrida,int izq,int der) {
    char m[100];snprintf(m,sizeof(m),"V1 123 %u %u %d %d %d %d",++seq,corrida,izq,izq,der,der);
    HardwareSerial tx;enviarRobot(tx,m);
    for(char b:tx.tx)enlace.rx.push_back(b);
    // Simula pulsos, no prueba mecanica real del motor ni consumo/carga.
    for(unsigned i=0;i<4;++i)if(aplicado[i])++pulsos[i];
    direccionMPU=0x68;imuValida=true;ultimaLecturaIMU=reloj;loop();
  };
  auto tick=[&](bool sensor=true) {
    reloj+=100;n.actualizar(mapa,c,sensor,++barrido,reloj);
    enviar(n.activa()?1:0,n.izquierda,n.derecha);
  };
  reloj=100;enviar(0,0,0);n.arrancar(c,reloj);
  for(unsigned k=0;k<10;++k) { tick(); }
  assert(moviendo && guardia.corrida==1);
  // Bloquea izquierda con obstaculos lejanos; elige derecha.
  for(unsigned i=52;i<69;++i)mapa[i]=600;
  mapa[0]=mapa[1]=400;
  for(unsigned k=0;k<15;++k)tick();
  assert(n.fase==Navegacion::DERECHA && objetivo[0]==80 && objetivo[1]==80 && objetivo[2]==-80 && objetivo[3]==-80);
  assert(aplicado[0]==80 && aplicado[1]==80 && aplicado[2]==-80 && aplicado[3]==-80);
  assert(pines[13]==LOW && pines[14]==HIGH && pines[26]==HIGH && pines[27]==LOW);
  assert(pines[18]==HIGH && pines[19]==LOW && pines[4]==LOW && pines[15]==HIGH);
  assert(moviendo && guardia.corrida==1 && guardia.bloqueada==0);
  // Mas de 6,5 s: alterna sin perder identificador ni activar watchdog falso.
  for(unsigned k=0;k<80;++k) { tick(); }
  assert(n.activa() && guardia.corrida==1);
  tick(false);assert(!moviendo && guardia.corrida==1);
  std::fill_n(mapa,72,2000);
  for(unsigned k=0;k<10;++k)tick();
  assert(moviendo && n.fase==Navegacion::RECTO && guardia.corrida==1);
  for(int v:aplicado)assert(v==80);
  // Recuperar sensor no es un nuevo arranque. STOP si cancela la corrida.
  n.parar();tick();assert(!moviendo && guardia.corrida==0);
  for(unsigned k=0;k<10;++k) { tick(); }
  assert(!moviendo);
  std::cout<<"OK: planificador -> CRC/UART -> DevKit real con dobles: derecha, inversion, correccion larga, recuperacion y STOP en una misma corrida\n";
}
