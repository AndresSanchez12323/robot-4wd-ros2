#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <deque>
#define CONFIG_IDF_TARGET_ESP32 1
#define ARDUINO_ISR_ATTR
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define FALLING 2
#define SERIAL_8N1 0
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE = int;
inline void portENTER_CRITICAL(int*) {} inline void portEXIT_CRITICAL(int*) {}
inline void portENTER_CRITICAL_ISR(int*) {} inline void portEXIT_CRITICAL_ISR(int*) {}
inline uint32_t reloj = 0;
inline int pines[64]{}, potencia[64]{};
inline uint32_t millis(){return reloj;}
inline void delay(unsigned ms){reloj+=ms;}
inline void pinMode(int,int){}
inline void digitalWrite(int pin,int v){pines[pin]=v;}
inline void analogWrite(int pin,int v){potencia[pin]=v;}
inline void attachInterrupt(int,void(*)(),int){}
class HardwareSerial {
 public:
  explicit HardwareSerial(int=0){}
  std::deque<char> rx; std::string tx;
  void setRxBufferSize(int){} void begin(int,int=0,int=0,int=0){}
  int available(){return rx.size();} char read(){char c=rx.front();rx.pop_front();return c;}
  void print(char c){tx+=c;} void print(const char*s){tx+=s;}
  void println(const char*s){tx+=s;tx+='\n';}
  template<typename... T> void printf(const char* f,T...args){char b[512];snprintf(b,sizeof(b),f,args...);tx+=b;}
};
inline HardwareSerial Serial;
