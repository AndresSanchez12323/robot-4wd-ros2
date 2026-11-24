#pragma once
#include <stdint.h>
struct WireDoble {
 void begin(int,int,int){} void setTimeOut(int){}
 void beginTransmission(uint8_t){} void write(uint8_t){}
 int endTransmission(bool=true){return 0;}
 int requestFrom(uint8_t,uint8_t n,bool){return n;}
 int available(){return 0;} uint8_t read(){return 0;}
};
inline WireDoble Wire;
