// Prueba de UART independiente de la camara.
// Placa: ESP32S3 Dev Module. Usar puerto USB-UART y USB CDC On Boot: Disabled.
// GPIO41/42 propuestos para esta placa: validar antes de integrar camara.
#include <Arduino.h>
#include <string.h>

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Seleccione ESP32S3 Dev Module para este programa."
#endif

constexpr int RX_ROBOT = 42;
constexpr int TX_ROBOT = 41;
HardwareSerial enlace(1);
char linea[32];
size_t longitud = 0;
bool descartar = false;
uint32_t ultimoEnvio = 0;
bool esperando = false;
bool huboEnvio = false;

void setup() {
  Serial.begin(115200);
  enlace.begin(115200, SERIAL_8N1, RX_ROBOT, TX_ROBOT);
  Serial.println("S3 LISTO | TX41 RX42 | prueba sin camara");
}

void loop() {
  while (enlace.available()) {
    const char c = static_cast<char>(enlace.read());
    if (c == '\r') continue;
    if (c == '\n') {
      if (!descartar) {
        linea[longitud] = '\0';
        if (strcmp(linea, "RECIBIDO") == 0 && esperando) {
          Serial.println("DevKitC -> RECIBIDO | ida y vuelta OK");
          esperando = false;
        }
      }
      longitud = 0;
      descartar = false;
    } else if (!descartar) {
      if (longitud < sizeof(linea) - 1) linea[longitud++] = c;
      else { descartar = true; longitud = 0; }
    }
  }
  const uint32_t ahora = millis();
  if (esperando && ahora - ultimoEnvio >= 1000) {
    Serial.println("SIN RESPUESTA: revisar alimentacion, GND y TX/RX");
    esperando = false;
  }
  if (!huboEnvio || ahora - ultimoEnvio >= 2000) {
    ultimoEnvio = ahora;
    huboEnvio = true;
    esperando = true;
    enlace.println("HOLA");
    Serial.println("S3 -> HOLA");
  }
  delay(1);
}
