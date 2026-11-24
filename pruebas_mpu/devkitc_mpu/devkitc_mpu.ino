// DevKitC V4 / WROOM-32D. Prueba MPU6050 sin motores.
// SDA=21, SCL=22. Monitor USB 115200. Solo Arduino.h y Wire.h.
#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include <math.h>

#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "Seleccione ESP32 Dev Module para DevKitC V4, no ESP32-S3."
#endif

constexpr int STBY = 2;
constexpr int MOTOR_PINS[] = {13,14,25,26,27,32,18,19,23,4,15,33};
HardwareSerial enlace(2);
uint8_t direccion = 0;
bool listo = false;
uint32_t ultimoReporte = 0;
uint32_t holas = 0;
char linea[32];
size_t longitud = 0;
bool descartar = false;

bool leer(uint8_t dir, uint8_t registro, uint8_t *datos, uint8_t cantidad) {
  Wire.beginTransmission(dir);
  Wire.write(registro);
  if (Wire.endTransmission(false) != 0) return false;
  const size_t recibidos = Wire.requestFrom(dir, cantidad, true);
  if (recibidos != cantidad) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (uint8_t i=0; i<cantidad; ++i) datos[i] = Wire.read();
  return true;
}

bool escribir(uint8_t registro, uint8_t valor) {
  Wire.beginTransmission(direccion);
  Wire.write(registro);
  Wire.write(valor);
  return Wire.endTransmission(true) == 0;
}

bool iniciarMPU() {
  constexpr uint8_t direcciones[] = {0x68, 0x69};
  for (uint8_t dir : direcciones) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission(true) != 0) continue;
    uint8_t id = 0;
    if (!leer(dir, 0x75, &id, 1)) {
      Serial.printf("I2C 0x%02X responde, pero WHO_AM_I no se pudo leer\n", dir);
      continue;
    }
    Serial.printf("Dispositivo en 0x%02X | WHO_AM_I=0x%02X\n", dir, id);
    // MPU6050 responde 0x68 en WHO_AM_I incluso con AD0 alto (dir 0x69).
    if (id != 0x68) {
      Serial.println("Identidad distinta de MPU6050: no configurar a ciegas");
      continue;
    }
    direccion = dir;
    if (!escribir(0x6B, 0x80)) return false; // Reset del sensor.
    delay(100);
    if (!escribir(0x6B, 0x01)) return false; // Despertar, reloj PLL X.
    delay(30);
    if (!escribir(0x6C, 0x00) || // Todos los ejes habilitados.
        !escribir(0x1A, 0x03) || // Filtro digital.
        !escribir(0x19, 0x09) || // Muestras internas 100 Hz.
        !escribir(0x1B, 0x00) || // Giroscopio +-250 grados/s: 131 LSB/(grado/s).
        !escribir(0x1C, 0x00)) return false; // Acelerometro +-2g: 16384 LSB/g.
    uint8_t gyroCfg = 0, accelCfg = 0, power = 0;
    if (!leer(direccion, 0x1B, &gyroCfg, 1) ||
        !leer(direccion, 0x1C, &accelCfg, 1) ||
        !leer(direccion, 0x6B, &power, 1)) return false;
    if ((gyroCfg & 0x18) != 0 || (accelCfg & 0x18) != 0 || (power & 0x40)) {
      Serial.println("ERROR: configuracion del MPU no coincide");
      return false;
    }
    Serial.println("MPU6050 LISTO | aceleracion en g | giro en grados/s");
    return true;
  }
  Serial.println("MPU NO DETECTADO/VALIDADO: revisar VCC, GND, SDA21 y SCL22 con todo apagado");
  return false;
}

void leerUART() {
  while (enlace.available()) {
    const char c = static_cast<char>(enlace.read());
    if (c == '\r') continue;
    if (c == '\n') {
      if (!descartar) {
        linea[longitud] = '\0';
        if (strcmp(linea, "HOLA") == 0) {
          ++holas;
          enlace.println("RECIBIDO");
        }
      }
      longitud = 0;
      descartar = false;
    } else if (!descartar) {
      if (longitud < sizeof(linea)-1) linea[longitud++] = c;
      else { longitud = 0; descartar = true; }
    }
  }
}

int16_t firmado16(const uint8_t *p) {
  const uint16_t valor = (uint16_t(p[0]) << 8) | p[1];
  return static_cast<int16_t>(valor);
}

void setup() {
  // VM SIN ALIMENTACION: el programa no protege antes de setup().
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW);
  for (int pin : MOTOR_PINS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  Serial.begin(115200);
  enlace.begin(115200, SERIAL_8N1, 16, 17);
  Serial.println("PRUEBA MPU | MOTORES DESHABILITADOS | SDA21 SCL22");
  if (!Wire.begin(21, 22, 100000)) {
    Serial.println("ERROR al iniciar I2C; reiniciar tras revisar conexiones");
    return;
  }
  Wire.setTimeOut(50);
  listo = iniciarMPU();
}

void loop() {
  leerUART();
  if (millis() - ultimoReporte >= 500) {
    ultimoReporte = millis();
    if (!listo) {
      Serial.println("MPU no listo. Revisar diagnostico inicial y pulsar EN tras corregir.");
    } else {
      uint8_t datos[14];
      if (!leer(direccion, 0x3B, datos, sizeof(datos))) {
        Serial.println("ERROR DE LECTURA I2C: muestra descartada");
      } else {
        const float ax = firmado16(datos) / 16384.0f;
        const float ay = firmado16(datos+2) / 16384.0f;
        const float az = firmado16(datos+4) / 16384.0f;
        const float temp = firmado16(datos+6) / 340.0f + 36.53f;
        const float gx = firmado16(datos+8) / 131.0f;
        const float gy = firmado16(datos+10) / 131.0f;
        const float gz = firmado16(datos+12) / 131.0f;
        Serial.printf("A[g] X=%.3f Y=%.3f Z=%.3f | modulo=%.3f | ",
          ax, ay, az, sqrtf(ax*ax+ay*ay+az*az));
        Serial.printf("GIRO[grados/s] X=%.2f Y=%.2f Z=%.2f | Tchip=%.1f C | UART_HOLA=%lu\n",
          gx, gy, gz, temp, (unsigned long)holas);
      }
    }
  }
  delay(1);
}
