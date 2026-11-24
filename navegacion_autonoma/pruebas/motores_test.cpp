#include "../devkit_integral/devkit_integral.ino"
#include <cassert>
#include <iostream>

void tiempo(uint32_t t) { reloj=t; imuValida=true; direccionMPU=0x68; ultimaLecturaIMU=t; }
void orden(const char *s) { procesarEnlace(s); aplicarObjetivo(millis()); }
int main() {
  tiempo(100); orden("V1 123 1 0 0 0 0 0");
  orden("V1 123 2 1 80 80 80 80");
  assert(moviendo && pines[STBY]==HIGH);
  assert(pines[13]==LOW && pines[14]==HIGH); // inversion FL conservada.
  assert(pines[26]==HIGH && pines[27]==LOW);
  for(const Motor&m:motores) assert(potencia[m.pwm]==80);
  tiempo(200); orden("V1 123 3 1 -80 -80 80 80");
  assert(!moviendo && pines[STBY]==LOW);
  tiempo(349); aplicarObjetivo(millis()); assert(!moviendo);
  tiempo(350); aplicarObjetivo(millis());
  assert(moviendo && pines[13]==HIGH && pines[14]==LOW && pines[26]==LOW && pines[27]==HIGH);
  assert(pines[18]==LOW && pines[19]==HIGH && pines[4]==HIGH && pines[15]==LOW);
  tiempo(400); orden("V1 123 4 1 0 0 0 0"); assert(!moviendo);
  tiempo(500); orden("V1 123 5 1 80 80 80 80"); assert(!moviendo);
  tiempo(650); aplicarObjetivo(millis()); assert(moviendo);
  detener("STOP_USB"); orden("V1 123 6 1 80 80 80 80"); assert(!moviendo);
  tiempo(700); orden("V1 123 7 2 80 80 80 80"); assert(moviendo);
  // Una trama tardia ya en el bufer no puede reanimar la corrida caducada.
  tiempo(1200); char linea[80];
  snprintf(linea,sizeof(linea),"V1 123 8 2 80 80 80 80");
  HardwareSerial salida; enviarRobot(salida,linea);
  for(char ch:salida.tx) enlace.rx.push_back(ch);
  loop(); assert(!moviendo && guardia.bloqueada==2);
  tiempo(1300); orden("V1 123 9 3 80 80 80 80"); assert(moviendo);
  tiempo(2810); procesarEnlace("V1 123 10 3 80 80 80 80"); loop();
  assert(!moviendo && std::string(motivo)=="ENCODER_FL_SIN_PULSOS");
  procesarEnlace("STOP"); assert(std::string(motivo)=="ENCODER_FL_SIN_PULSOS");
  tiempo(3000); orden("V1 123 11 4 80 80 80 80"); assert(moviendo);
  imuValida=false; procesarEnlace("V1 123 12 4 80 80 80 80"); assert(!moviendo);
  // Detectar cada encoder por nombre. Los otros tres tienen pulsos actuales.
  unsigned seq=12;
  for(unsigned i=0;i<4;++i) {
    tiempo(4000+i*3000);
    char m[100];snprintf(m,sizeof(m),"V1 123 %u %u 80 80 80 80",++seq,5+i);
    orden(m);if(pausaInversion){tiempo(reloj+150);aplicarObjetivo(millis());}
    assert(moviendo);
    for(unsigned j=0;j<4;++j){anteriores[j]=pulsos[j];ultimoPulso[j]=reloj;}
    ultimoPulso[i]=reloj-1600;
    loop();assert(!moviendo && std::string(motivo)==FALLO_ENCODER[i]);
  }
  std::cout<<"OK: salidas de cuatro motores, inversion, pausa no bloqueante, STOP, UART tardia, encoders y MPU\n";
}
