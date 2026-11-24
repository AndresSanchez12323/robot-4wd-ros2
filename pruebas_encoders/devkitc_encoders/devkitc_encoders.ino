// Prueba manual de encoders; no mueve motores ni inicia camara/IMU.
// DevKitC V4 WROOM-32D: ESP32 Dev Module. Monitor USB: 115200.
#include <Arduino.h>
#include <string.h>

#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "Este programa es para ESP32 Dev Module / DevKitC V4, no para S3."
#endif

constexpr int STBY = 2;
constexpr int MOTOR_PINS[] = {13,14,25,26,27,32,18,19,23,4,15,33};
constexpr int ENCODER_PINS[] = {34,35,36,39}; // FL, RL, FR(VP), RR(VN)
volatile uint32_t pulsos[4] = {0,0,0,0};
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
HardwareSerial enlace(2);
char linea[32];
size_t longitud = 0;
bool descartar = false;
uint32_t holas = 0;
uint32_t ultimoReporte = 0;
uint32_t anteriores[4] = {0,0,0,0};

void ARDUINO_ISR_ATTR contarFL() {
  portENTER_CRITICAL_ISR(&mux); ++pulsos[0]; portEXIT_CRITICAL_ISR(&mux);
}
void ARDUINO_ISR_ATTR contarRL() {
  portENTER_CRITICAL_ISR(&mux); ++pulsos[1]; portEXIT_CRITICAL_ISR(&mux);
}
void ARDUINO_ISR_ATTR contarFR() {
  portENTER_CRITICAL_ISR(&mux); ++pulsos[2]; portEXIT_CRITICAL_ISR(&mux);
}
void ARDUINO_ISR_ATTR contarRR() {
  portENTER_CRITICAL_ISR(&mux); ++pulsos[3]; portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
  // VM debe estar SIN ALIMENTACION; esto no protege antes de setup().
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW);
  for (int pin : MOTOR_PINS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  Serial.begin(115200);
  enlace.begin(115200, SERIAL_8N1, 16, 17);
  // No INPUT_PULLUP: GPIO34-39 no tienen pull-up interno.
  for (int pin : ENCODER_PINS) pinMode(pin, INPUT);
  attachInterrupt(34, contarFL, FALLING);
  attachInterrupt(35, contarRL, FALLING);
  attachInterrupt(36, contarFR, FALLING);
  attachInterrupt(39, contarRR, FALLING);
  Serial.println("ENCODERS LISTOS | FL34 RL35 FR36(VP) RR39(VN)");
  Serial.println("VM DESCONECTADO | girar UNA rueda a mano | r + Enter: poner a cero");
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

void loop() {
  leerUART();
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == 'r' || c == 'R') {
      portENTER_CRITICAL(&mux);
      for (int i=0; i<4; ++i) pulsos[i] = 0;
      portEXIT_CRITICAL(&mux);
      for (int i=0; i<4; ++i) anteriores[i] = 0;
      Serial.println("Contadores en cero");
    }
  }
  if (millis() - ultimoReporte >= 500) {
    ultimoReporte = millis();
    uint32_t actual[4];
    portENTER_CRITICAL(&mux);
    for (int i=0; i<4; ++i) actual[i] = pulsos[i];
    portEXIT_CRITICAL(&mux);
    Serial.printf("TOTAL FL=%lu RL=%lu FR=%lu RR=%lu | ",
      (unsigned long)actual[0], (unsigned long)actual[1],
      (unsigned long)actual[2], (unsigned long)actual[3]);
    Serial.printf("NUEVOS FL=%lu RL=%lu FR=%lu RR=%lu | ",
      (unsigned long)(actual[0]-anteriores[0]), (unsigned long)(actual[1]-anteriores[1]),
      (unsigned long)(actual[2]-anteriores[2]), (unsigned long)(actual[3]-anteriores[3]));
    Serial.printf("D0=%d,%d,%d,%d | UART_HOLA=%lu\n",
      digitalRead(34), digitalRead(35), digitalRead(36), digitalRead(39),
      (unsigned long)holas);
    for (int i=0; i<4; ++i) anteriores[i] = actual[i];
  }
  delay(1);
}
