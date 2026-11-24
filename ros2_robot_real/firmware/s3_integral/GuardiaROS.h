#pragma once
#include <stdint.h>

struct OrdenROS {
  uint32_t sesion, id, recibida;
  uint8_t inicio;
  int8_t preferencia;
};

class GuardiaROS {
 public:
  static constexpr uint32_t LIMITE_MS = 900;
  uint32_t ultimaOrden = 0;
  int8_t preferencia = 0;

  bool aceptar(const OrdenROS &orden, uint32_t sesion, uint32_t ahora,
               bool activa, bool puedeArrancar) {
    if (!orden.id || orden.sesion != sesion || orden.id <= ultimaOrden ||
        ahora - orden.recibida > 250 ||
        orden.preferencia < -1 || orden.preferencia > 1) return false;
    ultimaOrden = orden.id;
    if (orden.inicio ? activa || !puedeArrancar : !activa || !vigente(ahora)) return false;
    renovado = ahora;
    autorizada = true;
    preferencia = orden.preferencia;
    return true;
  }

  bool vigente(uint32_t ahora) const { return autorizada && ahora - renovado < LIMITE_MS; }
  void parar() { autorizada = false; preferencia = 0; }

 private:
  uint32_t renovado = 0;
  bool autorizada = false;
};
