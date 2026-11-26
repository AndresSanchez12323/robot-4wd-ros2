#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <math.h>
#include "EnlaceRobot.h"
#include "GuardiaMarcha.h"

#if __has_include("RedLocal.h")
#include "RedLocal.h"
#else
constexpr char CLAVE_RED[] = "CAMBIA_CLAVE_LAMBORGINI";
#endif
constexpr char NOMBRE_RED[] = "Lamborgini";

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
constexpr float CM_POR_PULSO[] = {13.2f / 27, 13.2f / 25, 13.2f / 26, 13.2f / 24};
constexpr uint32_t LIMITE_ENCODER_MS = 1500;
const char *const FALLO_ENCODER[] = {"ENCODER_FL_SIN_PULSOS", "ENCODER_RL_SIN_PULSOS",
                                   "ENCODER_FR_SIN_PULSOS", "ENCODER_RR_SIN_PULSOS"};
GuardiaMarcha guardia;
int objetivo[4]{}, aplicado[4]{};
int ultimoSentido[4]{};
uint32_t sinPotenciaDesde[4]{};
uint32_t inicioPausa = 0;
bool pausaInversion = false;

HardwareSerial enlace(2);
ReceptorRobot receptor;
portMUX_TYPE muxPulsos = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t pulsos[4]{};
uint32_t iniciales[4]{}, anteriores[4]{}, ultimoPulso[4]{};
uint8_t avisosEncoder[4]{};
uint32_t ultimoReporte = 0;
uint32_t ultimaLecturaIMU = 0, ultimoIntentoIMU = 0;
bool moviendo = false;
uint8_t direccionMPU = 0;
bool imuValida = false;
float ax = 0, ay = 0, az = 0, gz = 0;
const char *motivo = "ARRANQUE";
bool otaLista = false;
uint32_t ultimoIntentoWifi = 0;
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

void apagarSalidas() {
  digitalWrite(STBY, LOW);
  for (size_t i = 0; i < 4; ++i) {
    analogWrite(motores[i].pwm, 0);
    digitalWrite(motores[i].in1, LOW);
    digitalWrite(motores[i].in2, LOW);
    aplicado[i] = 0;
    sinPotenciaDesde[i] = millis();
  }
  moviendo = false;
}

void detener(const char *causa) {
  guardia.bloquear();
  for (int &v : objetivo) v = 0;
  apagarSalidas();
  pausaInversion = false;
  motivo = causa;
}

void aplicarObjetivo(uint32_t ahora) {
  if (pausaInversion && ahora - inicioPausa < 150) return;
  pausaInversion = false;
  bool inversion = false;
  for (size_t i = 0; i < 4; ++i) {
    if (ultimoSentido[i] * objetivo[i] < 0 &&
        (aplicado[i] || ahora - sinPotenciaDesde[i] < 150)) inversion = true;
  }
  if (inversion) { apagarSalidas(); inicioPausa = ahora; pausaInversion = true; return; }
  moviendo = false;
  for (size_t i = 0; i < 4; ++i) {
    int v = objetivo[i];
    const bool adelante = (v > 0) != motores[i].invertir;
    if (v != aplicado[i]) {
      analogWrite(motores[i].pwm, 0);
      digitalWrite(motores[i].in1, v ? (adelante ? HIGH : LOW) : LOW);
      digitalWrite(motores[i].in2, v ? (adelante ? LOW : HIGH) : LOW);
      analogWrite(motores[i].pwm, abs(v));
      if (v) ultimoSentido[i] = v > 0 ? 1 : -1;
      else sinPotenciaDesde[i] = ahora;
      if (!aplicado[i]) ultimoPulso[i] = ahora;
      aplicado[i] = v;
    }
    moviendo |= v != 0;
  }
  digitalWrite(STBY, moviendo ? HIGH : LOW);
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

void procesarEnlace(const char *texto) {
  if (strcmp(texto, "STOP") == 0) {
    // Conservar la causa de un fallo ya enclavado para verla en la pagina.
    if (guardia.corrida || !guardia.bloqueada) detener("STOP_OPERADOR");
    return;
  }
  unsigned long sesion, secuencia, corrida;
  int v[4]; char extra;
  if (sscanf(texto, "V1 %lu %lu %lu %d %d %d %d %c", &sesion, &secuencia, &corrida,
      &v[0], &v[1], &v[2], &v[3], &extra) != 7) return;
  for (int valor : v) if (valor < -140 || valor > 140) return;
  if (!corrida) for (int valor : v) if (valor) return;
  const uint32_t anterior = guardia.corrida;
  if (!guardia.aceptar(sesion, secuencia, corrida, millis())) return;
  if (!corrida) { for (int &valor : objetivo) valor = 0; apagarSalidas(); return; }
  if (!imuValida || millis() - ultimaLecturaIMU > 250) { detener("MPU_SIN_DATOS"); return; }
  if (corrida != anterior) {
    copiarPulsos(iniciales);
    copiarPulsos(anteriores);
    for (uint32_t &t : ultimoPulso) t = millis();
    for (uint8_t &aviso : avisosEncoder) aviso = 0;
  }
  for (size_t i = 0; i < 4; ++i) objetivo[i] = v[i];
  motivo = "ORDEN_AUTONOMA";
}

void leerUSB() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineaUSB[longitudUSB] = '\0';
      if (!descartarUSB) {
        if (strcmp(lineaUSB, "STOP") == 0) detener("STOP_USB");
        
        else Serial.println("Comando USB: STOP. ARRANCAR se hace en la pagina del S3.");
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
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(NOMBRE_RED, CLAVE_RED);
  ultimoIntentoWifi = millis();
  Serial.printf("AUTONOMO DEVKIT V1.6 | MPU=%s | motores deshabilitados | OTA robot-devkit\n", mpu ? "SI" : "NO");
}

void prepararOTA() {
  ArduinoOTA.setHostname("robot-devkit");
  ArduinoOTA.setPassword(CLAVE_RED);
  ArduinoOTA.onStart([]() { detener("OTA"); });
  ArduinoOTA.begin();
  otaLista = true;
  WiFi.setSleep(false);
  Serial.println("OTA lista: robot-devkit en Lamborgini");
}

void mantenerWifi(uint32_t ahora) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!otaLista) prepararOTA();
    ArduinoOTA.handle();
    return;
  }
  otaLista = false;
  if (ahora - ultimoIntentoWifi < 5000) return;
  ultimoIntentoWifi = ahora;
  WiFi.disconnect();
  WiFi.begin(NOMBRE_RED, CLAVE_RED);
}

void loop() {
  // Comprobar ANTES de leer: una trama tardia no debe renovar una corrida caducada.
  if (guardia.corrida && !guardia.vigente(millis())) detener("SIN_ENLACE_S3");
  for (size_t i = 0; i < 512 && enlace.available(); ++i) {
    const char *texto = receptor.recibir(enlace.read());
    if (texto) procesarEnlace(texto);
  }
  leerUSB();
  actualizarIMU(millis());
  const uint32_t ahora = millis();
  uint32_t cuentas[4];
  copiarPulsos(cuentas);
  if (guardia.corrida) {
    if (!guardia.vigente(ahora)) detener("SIN_ENLACE_S3");
    else if (!imuValida || ahora - ultimaLecturaIMU > 250) detener("MPU_SIN_DATOS");
    else aplicarObjetivo(ahora);
  }
  for (size_t i = 0; i < 4; ++i) {
    if (cuentas[i] != anteriores[i]) {
      ultimoPulso[i] = ahora;
      avisosEncoder[i] = 0;
    } else if (!aplicado[i]) {
      ultimoPulso[i] = ahora;
    }
    anteriores[i] = cuentas[i];
    if (aplicado[i] && ahora - ultimoPulso[i] > LIMITE_ENCODER_MS) {
      if (!avisosEncoder[i]) {
        avisosEncoder[i] = 1;
        ultimoPulso[i] = ahora;
        motivo = "ENCODER_REINTENTO";
        break;
      }
      detener(FALLO_ENCODER[i]);
      break;
    }
  }
  if (ahora - ultimoReporte >= 200) {
    ultimoReporte = ahora;
    char mensaje[300];
    snprintf(mensaje, sizeof(mensaje),
      "A1 %lu %lu %lu %u %u %.1f %s | %lu %lu %lu %lu %.3f %.3f %.3f %.2f",
      (unsigned long)guardia.sesion, (unsigned long)guardia.corrida, (unsigned long)guardia.bloqueada,
      moviendo, imuValida, distanciaCM(cuentas), motivo,
      (unsigned long)(cuentas[0] - iniciales[0]), (unsigned long)(cuentas[1] - iniciales[1]),
      (unsigned long)(cuentas[2] - iniciales[2]), (unsigned long)(cuentas[3] - iniciales[3]),
      ax, ay, az, gz);
    enviarRobot(enlace, mensaje);
    Serial.println(mensaje);
  }
  mantenerWifi(ahora);
  delay(1);
}
