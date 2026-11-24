#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// COIN-D6, protocolo V1.0. AA 55, M&T, N, FSA, LSA, XOR, N muestras de 3 B.
// Los prefijos A5 5A y ACK del equipo se ignoran mientras se busca AA 55.
class LidarD6 {
 public:
  static constexpr size_t SECTORES = 72;
  uint32_t paquetes = 0;
  uint32_t bytesRecibidos = 0;
  uint32_t erroresCabecera = 0;
  uint32_t erroresChecksum = 0;
  uint32_t inicios = 0;
  char diagnostico[180] = "Esperando bytes del LiDAR";
  uint32_t errores = 0;
  uint32_t barridos = 0;
  uint32_t ultimoBarrido = 0;
  uint16_t minimo = 0;
  uint16_t puntos = 0;
  uint8_t cobertura = 0;
  uint16_t distancias[SECTORES]{};
  bool completo = false;

  void recibir(uint8_t valor, uint32_t ahora) {
    ++bytesRecibidos;
    if (usados && ahora - ultimoByte > 100) {
      usados = 0;
      corrupto = true;
    }
    ultimoByte = ahora;
    if (usados == sizeof(buffer)) descartar(1);
    buffer[usados++] = valor;

    while (usados >= 2) {
      if (buffer[0] != 0xAA || buffer[1] != 0x55) {
        descartar(1);
        continue;
      }
      if (usados < 10) return;
      const uint8_t cantidad = buffer[3];
      const uint16_t primero = palabra(buffer + 4);
      const uint16_t ultimo = palabra(buffer + 6);
      const bool inicio = buffer[2] & 1;
      if (cantidad == 0 || !(primero & 1) || !(ultimo & 1) ||
          (primero >> 1) >= 360 * 64 || (ultimo >> 1) >= 360 * 64 ||
          (inicio && cantidad != 1)) {
        ++erroresCabecera;
        registrarError("CABECERA", 0, ahora);
        ++errores;
        corrupto = true;
        descartar(1);
        continue;
      }
      const size_t longitud = 10 + 3 * static_cast<size_t>(cantidad);
      if (usados < longitud) return;
      uint16_t checksum = 0x55AA ^ palabra(buffer + 2) ^ primero ^ ultimo;
      for (size_t i = 10; i < longitud; i += 3) {
        // SDK del fabricante: primer byte aislado y los dos siguientes en LE.
        // El diagrama del manual V1.0 agrupa estos bytes de otra manera.
        checksum ^= buffer[i];
        checksum ^= palabra(buffer + i + 1);
      }
      if (checksum != palabra(buffer + 8)) {
        ++erroresChecksum;
        registrarError("CHECKSUM", checksum, ahora);
        ++errores;
        corrupto = true;
        descartar(1);
        continue;
      }
      ++paquetes;
      if (inicio) ++inicios;
      procesar(cantidad, primero, ultimo, inicio, ahora);
      descartar(longitud);
    }
  }

  bool vigente(uint32_t ahora) const {
    return completo && ahora - ultimoBarrido <= 500;
  }

 private:
  uint8_t buffer[10 + 3 * 255]{};
  size_t usados = 0;
  uint32_t ultimoByte = 0;
  uint32_t inicioBarrido = 0;
  bool iniciado = false;
  bool corrupto = false;
  uint16_t acumuladas[SECTORES]{};
  uint16_t cantidadPuntos = 0;
  uint32_t ultimoDiagnostico = 0;
  bool diagnosticoRegistrado = false;

  void registrarError(const char *tipo, uint16_t calculado, uint32_t ahora) {
    // Limitar el formateo para no retrasar la lectura UART con datos corruptos.
    if (diagnosticoRegistrado && ahora - ultimoDiagnostico < 1000) return;
    diagnosticoRegistrado = true;
    ultimoDiagnostico = ahora;
    char hex[61];
    const size_t cantidad = usados < 20 ? usados : 20;
    for (size_t i = 0; i < cantidad; ++i) {
      snprintf(hex + 3 * i, sizeof(hex) - 3 * i, "%02X ", buffer[i]);
    }
    hex[3 * cantidad] = '\0';
    snprintf(diagnostico, sizeof(diagnostico),
      "%s tipo=%02X N=%u FSA=%04X LSA=%04X CS=%04X calculado=%04X | %s",
      tipo, buffer[2], buffer[3], palabra(buffer + 4), palabra(buffer + 6),
      palabra(buffer + 8), calculado, hex);
  }

  static uint16_t palabra(const uint8_t *p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
  }

  void descartar(size_t cantidad) {
    usados -= cantidad;
    memmove(buffer, buffer + cantidad, usados);
  }

  void procesar(uint8_t cantidad, uint16_t primero, uint16_t ultimo,
                bool inicio, uint32_t ahora) {
    if (inicio) {
      if (iniciado) {
        ++barridos;
        ultimoBarrido = ahora;
        cobertura = 0;
        minimo = 0;
        puntos = cantidadPuntos;
        memcpy(distancias, acumuladas, sizeof(distancias));
        for (uint16_t d : distancias) {
          if (d) {
            ++cobertura;
            if (!minimo || d < minimo) minimo = d;
          }
        }
        const uint32_t duracion = ahora - inicioBarrido;
        // 36 de 72 alcanza: en la sala real el D6 deja huecos y aun asi se puede conducir.
        completo = !corrupto && cobertura >= 36 && puntos >= 100 &&
                   duracion >= 40 && duracion <= 300;
      }
      memset(acumuladas, 0, sizeof(acumuladas));
      cantidadPuntos = 0;
      corrupto = false;
      iniciado = true;
      inicioBarrido = ahora;
    }
    if (!iniciado) return;
    const float anguloInicial = (primero >> 1) / 64.0f;
    float arco = (ultimo >> 1) / 64.0f - anguloInicial;
    if (arco < 0) arco += 360.0f;
    for (size_t i = 0; i < cantidad; ++i) {
      const uint8_t *muestra = buffer + 10 + 3 * i;
      const uint16_t distancia = static_cast<uint16_t>(muestra[2]) * 64 +
                                 (muestra[1] >> 2);
      if (!distancia || distancia > 12000) continue;
      float angulo = anguloInicial;
      if (cantidad > 1) angulo += arco * i / (cantidad - 1);
      if (angulo >= 360) angulo -= 360;
      const size_t sector = static_cast<size_t>(angulo / 5.0f) % SECTORES;
      if (!acumuladas[sector] || distancia < acumuladas[sector]) {
        acumuladas[sector] = distancia;
      }
      if (cantidadPuntos < 65535) ++cantidadPuntos;
    }
  }
};
