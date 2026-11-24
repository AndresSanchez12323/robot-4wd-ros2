#pragma once

// Una espera de LiDAR/espacio conserva la corrida. Estos fallos reales la cancelan.
// El video es supervisión visual, no fuente de distancias ni latido del operador.
inline const char *causaParada(bool servidor, bool operador, bool devkit,
                              bool imu, bool bloqueada, const char *causaDevkit) {
  if (!servidor) return "Servidor de control no disponible";
  if (!operador) return "Enlace con operador perdido";
  if (!devkit) return "Telemetria DevKit perdida";
  if (bloqueada) return causaDevkit;
  if (!imu) return "MPU sin datos";
  return nullptr;
}
