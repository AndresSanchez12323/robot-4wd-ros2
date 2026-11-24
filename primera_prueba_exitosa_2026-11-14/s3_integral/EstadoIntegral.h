#pragma once
#include <stdint.h>

struct EstadoIntegral {
  bool permiso = false;
  bool camara = false;
  bool lidar = false;
  bool enlace = false;
  bool imu = false;
  bool moviendo = false;
  bool operador = false;
  uint16_t minimo = 0;
  uint8_t sectores = 0;
  uint32_t paquetes = 0;
  uint32_t bytesLidar = 0;
  uint32_t erroresCabecera = 0;
  uint32_t erroresChecksum = 0;
  uint32_t iniciosLidar = 0;
  char diagnosticoLidar[180] = "Esperando bytes del LiDAR";
  uint32_t errores = 0;
  uint32_t barridos = 0;
  float fps = 0;
  float distancia = 0;
  uint16_t mapa[72]{};
  char devkit[300] = "Esperando DevKitC";
  char evento[96] = "Esperando video, LiDAR y DevKitC";
};
