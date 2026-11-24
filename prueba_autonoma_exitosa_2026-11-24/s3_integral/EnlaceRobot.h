#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
