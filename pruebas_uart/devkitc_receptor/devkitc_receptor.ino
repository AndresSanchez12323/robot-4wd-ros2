// Prueba UART, sin movimiento. Placa: ESP32 Dev Module (WROOM-32D).
// USB y UART de robot son puertos diferentes. VM debe quedar desconectado.
#include <Arduino.h>
#include <string.h>

#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "Seleccione ESP32 Dev Module para el DevKitC V4, no ESP32-S3."
#endif

constexpr int RX_ROBOT = 16;
constexpr int TX_ROBOT = 17;
constexpr int STBY = 2;
constexpr int MOTOR_PINS[] = {13, 14, 25, 26, 27, 32, 18, 19, 23, 4, 15, 33};
HardwareSerial enlace(2);
char linea[32];
size_t longitud = 0;
bool descartar = false;
uint32_t recibidos = 0;
uint32_t ultimoAviso = 0;

void setup() {
  digitalWrite(STBY, LOW);
  pinMode(STBY, OUTPUT);
  for (int pin : MOTOR_PINS) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  Serial.begin(115200);
  enlace.begin(115200, SERIAL_8N1, RX_ROBOT, TX_ROBOT);
  Serial.println("DEVKITC LISTO | RX16 TX17 | MOTORES DESHABILITADOS");
}

void loop() {
  // Este programa nunca habilita STBY ni genera PWM.
  while (enlace.available()) {
    const char c = static_cast<char>(enlace.read());
    if (c == '\r') continue;
    if (c == '\n') {
      if (!descartar) {
        linea[longitud] = '\0';
        if (strcmp(linea, "HOLA") == 0) {
          ++recibidos;
          enlace.println("RECIBIDO");
          Serial.print("S3 -> HOLA | respuesta -> RECIBIDO | total: ");
          Serial.println(recibidos);
        } else if (longitud > 0) {
          Serial.println("Mensaje UART desconocido: descartado");
        }
      }
      longitud = 0;
      descartar = false;
    } else if (!descartar) {
      if (longitud < sizeof(linea) - 1) linea[longitud++] = c;
      else { descartar = true; longitud = 0; }
    }
  }
  if (millis() - ultimoAviso >= 3000) {
    ultimoAviso = millis();
    Serial.print("DEVKITC ACTIVO | HOLA recibidos: ");
    Serial.print(recibidos);
    Serial.println(" | motores deshabilitados");
  }
  delay(1);
}
