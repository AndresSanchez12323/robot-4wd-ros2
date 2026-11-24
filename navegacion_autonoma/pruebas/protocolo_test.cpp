#include "../s3_integral/LidarD6.h"
#include "../s3_integral/EnlaceRobot.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

std::vector<uint8_t> paquete(bool inicio, unsigned n, float a, float b, unsigned mm) {
  std::vector<uint8_t> p(10 + 3*n);
  auto poner=[&](unsigned i,unsigned v){p[i]=v&255;p[i+1]=v>>8;};
  p[0]=0xAA;p[1]=0x55;p[2]=inicio?21:20;p[3]=n;
  poner(4,unsigned(a*128)+1);poner(6,unsigned(b*128)+1);
  for(unsigned i=0;i<n;++i){p[10+3*i]=0;p[11+3*i]=(mm&63)<<2;p[12+3*i]=mm>>6;}
  unsigned cs=0;
  // SDK: cabecera 16 bits; cada muestra usa 8 bits y luego 16 bits LE.
  for(unsigned i=0;i<8;i+=2)cs^=p[i]|(p[i+1]<<8);
  for(unsigned i=10;i<p.size();i+=3)cs^=p[i]^(p[i+1]|(p[i+2]<<8));
  poner(8,cs);return p;
}
void dar(LidarD6 &l,const std::vector<uint8_t>&p,unsigned t){for(auto b:p)l.recibir(b,t);}
int main(){
  // Vector fijo independiente del generador: XOR SDK=0x5AFC, distancia=946 mm.
  // El agrupamiento anterior produce 0x9F39 y debe rechazar la trama.
  const std::vector<uint8_t> vectorSDK = {
    0xAA,0x55,0x15,0x01,0x01,0x00,0x01,0x00,0xFC,0x5A,0x88,0xCB,0x0E
  };
  LidarD6 referencia;
  dar(referencia,vectorSDK,0);
  dar(referencia,vectorSDK,100);
  assert(referencia.paquetes==2 && referencia.inicios==2 && referencia.errores==0);
  assert(referencia.distancias[0]==946);
  auto checksumAnterior=vectorSDK;
  checksumAnterior[8]=0x39;checksumAnterior[9]=0x9F;
  dar(referencia,checksumAnterior,120);
  assert(referencia.paquetes==2 && referencia.erroresChecksum==1);
  assert(crcRobot("123456789",9)==0x29B1);
  ReceptorRobot r;
  auto recibir=[&](std::string s){std::string salida;for(char c:s){auto p=r.recibir(c);if(p)salida=p;}return salida;};
  assert(recibir("ruido@123456789*29B1\n")=="123456789");
  assert(recibir("@123456788*29B1\n").empty());
  assert(recibir("@incompleto@123456789*29B1\r\n")=="123456789");
  assert(recibir("@"+std::string(500,'x')+"\n").empty());
  assert(recibir("@123456789*29B1\n")=="123456789");
  LidarD6 l;
  dar(l,paquete(true,1,0,0,800),0);
  dar(l,paquete(false,144,0,357.5,800),20);
  dar(l,paquete(true,1,0,0,800),100);
  assert(l.completo&&l.cobertura==72&&l.minimo==800&&l.vigente(600)&&!l.vigente(601));
  auto malo=paquete(false,144,0,357.5,100);malo[10]^=4;
  dar(l,malo,120);dar(l,paquete(true,1,0,0,800),200);
  assert(!l.completo&&l.errores>0);
  dar(l,paquete(false,144,0,357.5,900),220);dar(l,paquete(true,1,0,0,900),300);
  assert(l.completo&&l.minimo==800); // muestra de inicio del barrido anterior.
  dar(l,paquete(false,144,0,357.5,0),320);dar(l,paquete(true,1,0,0,900),400);
  assert(!l.completo); // ceros no prueban que el entorno esta libre.
  LidarD6 wrap;
  dar(wrap,paquete(true,1,0,0,1000),0);
  dar(wrap,paquete(false,5,350,10,200),10);
  dar(wrap,paquete(true,1,0,0,1000),100);
  assert(!wrap.completo&&wrap.distancias[70]==200&&wrap.distancias[0]==200&&wrap.distancias[2]==200);
  std::cout<<"OK: CRC, tramas corruptas, resincronizacion, barridos, caducidad, ceros y cruce de 360 grados\n";
}
