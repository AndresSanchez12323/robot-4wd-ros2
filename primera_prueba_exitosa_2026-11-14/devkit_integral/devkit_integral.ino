#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "EnlaceRobot.h"

#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "Selecciona ESP32 Dev Module para el DevKitC."
#endif

// Orden de las ruedas: FL, RL, FR, RR. Mapa conservado del montaje probado.
struct Motor {
  int in1, in2, pwm, encoder;
  bool invertir;
};
const Motor motores[] = {
  {13, 14, 25, 34, true},
  {26, 27, 32, 35, false},
  {18, 19, 23, 36, true},
  {4, 15, 33, 39, false}
};
constexpr int STBY = 2;
constexpr int PWM_PRUEBA = 80;
constexpr float OBJETIVO_CM = 50.0f;
// Factores compatibles con los ultimos registros aceptados de 30 cm.
constexpr float CM_POR_PULSO[] = {13.2f / 27, 13.2f / 25, 13.2f / 26, 13.2f / 24};
constexpr uint32_t LIMITE_MOVIMIENTO_MS = 8000;
constexpr uint32_t LIMITE_ENLACE_MS = 450;
constexpr uint32_t LIMITE_ENCODER_MS = 1500;
constexpr uint16_t OBSTACULO_MM = 250;

HardwareSerial enlace(2);
ReceptorRobot receptor;
portMUX_TYPE muxPulsos = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t pulsos[4]{};
uint32_t iniciales[4]{}, anteriores[4]{}, ultimoPulso[4]{};
uint32_t sesionS3 = 0, secuenciaS3 = 0, ultimoInicio = 0;
uint32_t ultimoLatido = 0, inicioMovimiento = 0, ultimoReporte = 0;
uint32_t ultimaLecturaIMU = 0, ultimoIntentoIMU = 0;
bool latidoRecibido = false, permisoS3 = false, moviendo = false;
uint16_t minimoLidar = 0;
uint8_t direccionMPU = 0;
bool imuValida = false;
float ax = 0, ay = 0, az = 0, gz = 0;
const char *motivo = "ARRANQUE";
char lineaUSB[32]{};
size_t longitudUSB = 0;
bool descartarUSB = false;

void ARDUINO_ISR_ATTR contarFL() {
  portENTER_CRITICAL_ISR(&muxPulsos); ++pulsos[0]; portEXIT_CRITICAL_ISR(&muxPulsos);
}
void ARDUINO_ISR_ATTR contarRL() {
  portENTER_CRITICAL_ISR(&muxPulsos); ++pulsos[1]; portEXIT_CRITICAL_ISR(&muxPulsos);
}
void ARDUINO_ISR_ATTR contarFR() {
  portENTER_CRITICAL_ISR(&muxPulsos); ++pulsos[2]; portEXIT_CRITICAL_ISR(&muxPulsos);
}
void ARDUINO_ISR_ATTR contarRR() {
  portENTER_CRITICAL_ISR(&muxPulsos); ++pulsos[3]; portEXIT_CRITICAL_ISR(&muxPulsos);
}

void copiarPulsos(uint32_t *destino) {
  portENTER_CRITICAL(&muxPulsos);
  for (size_t i = 0; i < 4; ++i) destino[i] = pulsos[i];
  portEXIT_CRITICAL(&muxPulsos);
}

float distanciaCM(const uint32_t *cuentas) {
  float suma = 0;
  for (size_t i = 0; i < 4; ++i) suma += (cuentas[i] - iniciales[i]) * CM_POR_PULSO[i];
  return suma / 4;
}

void detener(const char *causa) {
  digitalWrite(STBY, LOW);
  for (const Motor &m : motores) {
    analogWrite(m.pwm, 0);
    digitalWrite(m.in1, LOW);
    digitalWrite(m.in2, LOW);
  }
  const bool estabaMoviendo = moviendo;
  moviendo = false;
  motivo = causa;
  if (estabaMoviendo) {
    uint32_t cuentas[4];
    copiarPulsos(cuentas);
    Serial.printf("FIN | %s | estimado %.1f cm\n", causa, distanciaCM(cuentas));
    Serial.printf("PULSOS | FL=%lu RL=%lu FR=%lu RR=%lu\n",
      (unsigned long)(cuentas[0] - iniciales[0]), (unsigned long)(cuentas[1] - iniciales[1]),
      (unsigned long)(cuentas[2] - iniciales[2]), (unsigned long)(cuentas[3] - iniciales[3]));
  }
}

bool leerRegistros(uint8_t dir, uint8_t registro, uint8_t *datos, uint8_t cantidad) {
  Wire.beginTransmission(dir);
  Wire.write(registro);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(dir, cantidad, true) != cantidad) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (uint8_t i = 0; i < cantidad; ++i) datos[i] = Wire.read();
  return true;
}

bool escribirRegistro(uint8_t reg, uint8_t valor) {
  Wire.beginTransmission(direccionMPU);
  Wire.write(reg);
  Wire.write(valor);
  return Wire.endTransmission(true) == 0;
}

bool iniciarMPU() {
  for (uint8_t dir : {0x68, 0x69}) {
    uint8_t id = 0;
    if (leerRegistros(dir, 0x75, &id, 1) && id == 0x68) {
      direccionMPU = dir;
      return escribirRegistro(0x6B, 0x01) && escribirRegistro(0x1A, 0x03) &&
             escribirRegistro(0x1B, 0x00) && escribirRegistro(0x1C, 0x00);
    }
  }
  direccionMPU = 0;
  return false;
}

int16_t firmado16(const uint8_t *p) {
  return static_cast<int16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

void actualizarIMU(uint32_t ahora) {
  if (ahora - ultimoIntentoIMU < 50) return;
  ultimoIntentoIMU = ahora;
  uint8_t datos[14];
  imuValida = direccionMPU && leerRegistros(direccionMPU, 0x3B, datos, sizeof(datos));
  if (!imuValida) return;
  ax = firmado16(datos) / 16384.0f;
  ay = firmado16(datos + 2) / 16384.0f;
  az = firmado16(datos + 4) / 16384.0f;
  gz = firmado16(datos + 12) / 131.0f;
  ultimaLecturaIMU = ahora;
}

const char *bloqueo(uint32_t ahora) {
  if (!latidoRecibido || ahora - ultimoLatido > LIMITE_ENLACE_MS) return "SIN_ENLACE_S3";
  if (!permisoS3) return "S3_SIN_PERMISO";
  if (!minimoLidar || minimoLidar < OBSTACULO_MM) return "OBSTACULO_LIDAR";
  if (!imuValida || ahora - ultimaLecturaIMU > 250) return "MPU_SIN_DATOS";
  return nullptr;
}

void iniciarAvance() {
  if (moviendo) return;
  const char *problema = bloqueo(millis());
  if (problema) {
    motivo = problema;
    Serial.printf("RECHAZADO | %s\n", problema);
    return;
  }
  copiarPulsos(iniciales);
  inicioMovimiento = millis();
  for (size_t i = 0; i < 4; ++i) {
    anteriores[i] = iniciales[i];
    ultimoPulso[i] = inicioMovimiento;
    digitalWrite(motores[i].in1, motores[i].invertir ? LOW : HIGH);
    digitalWrite(motores[i].in2, motores[i].invertir ? HIGH : LOW);
    analogWrite(motores[i].pwm, PWM_PRUEBA);
  }
  motivo = "AVANZANDO_50CM";
  moviendo = true;
  digitalWrite(STBY, HIGH);
  Serial.println("INICIO | objetivo 50 cm | PWM 80 | LIDAR+MPU+UART activos");
}

void procesarEnlace(const char *texto) {
  unsigned long sesion, secuencia;
  unsigned int permiso, distancia;
  char extra;
  if (sscanf(texto, "S %lu %lu %u %u %c", &sesion, &secuencia, &permiso, &distancia, &extra) == 4) {
    if (!sesion || permiso > 1 || distancia > 12000) return;
    if (sesion != sesionS3) {
      detener("CAMBIO_SESION_S3");
      sesionS3 = sesion;
      secuenciaS3 = 0;
      ultimoInicio = 0;
    }
    if (static_cast<int32_t>(secuencia - secuenciaS3) <= 0) return;
    secuenciaS3 = secuencia;
    permisoS3 = permiso;
    minimoLidar = distancia;
    ultimoLatido = millis();
    latidoRecibido = true;
    return;
  }
  if (sscanf(texto, "GO %lu %lu %c", &sesion, &secuencia, &extra) == 2) {
    if (sesion != sesionS3 || static_cast<int32_t>(secuencia - ultimoInicio) <= 0) return;
    ultimoInicio = secuencia;
    iniciarAvance();
    return;
  }
  if (strcmp(texto, "STOP") == 0) detener("STOP_OPERADOR");
}

void leerUSB() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineaUSB[longitudUSB] = '\0';
      if (!descartarUSB) {
        if (strcmp(lineaUSB, "STOP") == 0) detener("STOP_USB");
        else if (strcmp(lineaUSB, "AVANZA 50") == 0) iniciarAvance();
        else Serial.println("Comandos: AVANZA 50 | STOP. Requiere permiso del S3.");
      }
      longitudUSB = 0;
      descartarUSB = false;
    } else if (longitudUSB < sizeof(lineaUSB) - 1) {
      lineaUSB[longitudUSB++] = c;
    } else {
      descartarUSB = true;
    }
  }
}

void setup() {
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW);
  for (const Motor &m : motores) {
    pinMode(m.in1, OUTPUT);
    pinMode(m.in2, OUTPUT);
    pinMode(m.pwm, OUTPUT);
    digitalWrite(m.in1, LOW);
    digitalWrite(m.in2, LOW);
    analogWrite(m.pwm, 0);
    pinMode(m.encoder, INPUT);
  }
  Serial.begin(115200);
  enlace.setRxBufferSize(1024);
  enlace.begin(115200, SERIAL_8N1, 16, 17);
  attachInterrupt(34, contarFL, FALLING);
  attachInterrupt(35, contarRL, FALLING);
  attachInterrupt(36, contarFR, FALLING);
  attachInterrupt(39, contarRR, FALLING);
  Wire.begin(21, 22, 100000);
  Wire.setTimeOut(15);
  const bool mpu = iniciarMPU();
  motivo = mpu ? "LISTO" : "MPU_NO_DETECTADO";
  Serial.printf("INTEGRAL DEVKIT V1 | MPU=%s | motores deshabilitados\n", mpu ? "SI" : "NO");
}

void loop() {
  for (size_t i = 0; i < 512 && enlace.available(); ++i) {
    const char *texto = receptor.recibir(enlace.read());
    if (texto) procesarEnlace(texto);
  }
  leerUSB();
  actualizarIMU(millis());
  const uint32_t ahora = millis();
  uint32_t cuentas[4];
  copiarPulsos(cuentas);
  if (moviendo) {
    const char *problema = bloqueo(ahora);
    if (problema) detener(problema);
    else if (ahora - inicioMovimiento >= LIMITE_MOVIMIENTO_MS) detener("LIMITE_TIEMPO");
    else if (distanciaCM(cuentas) >= OBJETIVO_CM) detener("FIN_50CM");
    else {
      for (size_t i = 0; i < 4; ++i) {
        if (cuentas[i] != anteriores[i]) ultimoPulso[i] = ahora;
        anteriores[i] = cuentas[i];
        if (ahora - ultimoPulso[i] > LIMITE_ENCODER_MS) {
          detener("ENCODER_SIN_PULSOS");
          break;
        }
      }
    }
  }
  if (ahora - ultimoReporte >= 200) {
    ultimoReporte = ahora;
    char mensaje[300];
    snprintf(mensaje, sizeof(mensaje),
      "D %lu %u %u %.1f %lu %lu %lu %lu %.3f %.3f %.3f %.2f %s",
      (unsigned long)sesionS3, moviendo, imuValida, distanciaCM(cuentas),
      (unsigned long)(cuentas[0] - iniciales[0]), (unsigned long)(cuentas[1] - iniciales[1]),
      (unsigned long)(cuentas[2] - iniciales[2]), (unsigned long)(cuentas[3] - iniciales[3]),
      ax, ay, az, gz, motivo);
    enviarRobot(enlace, mensaje);
    Serial.println(mensaje);
  }
  delay(1);
}
