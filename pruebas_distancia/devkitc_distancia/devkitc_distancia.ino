#include <Arduino.h>
#include <atomic>
#include <string.h>

// Valores provisionales para la primera prueba. Confirmar diametro y conteos.
constexpr float DIAMETRO_CM = 6.5f;  // Diametro supuesto, en cm.

constexpr float PULSOS_CINCO_VUELTAS[] = {
  148.0f,  // FL: delantera izquierda
  36.0f,  // RL: trasera izquierda
  130.0f,  // FR: delantera derecha
  202.0f   // RR: total provisional, suponiendo que inicio en cero.
};

struct Motor {
  int in1;
  int in2;
  int pwm;
  int encoder;
  bool invertir;
};

constexpr Motor motores[] = {
  {13, 14, 25, 34, true},   // FL
  {26, 27, 32, 35, false},  // RL
  {18, 19, 23, 36, true},   // FR
  { 4, 15, 33, 39, false}   // RR
};

constexpr int STBY = 2;
constexpr int PWM_PRUEBA = 80;
constexpr float OBJETIVO_CM = 30.0f;
constexpr uint32_t PULSO_MS = 350;
constexpr uint32_t LIMITE_MS = 3000;

enum class Modo { Parado, Pulso, Distancia };
Modo modo = Modo::Parado;

std::atomic<uint32_t> pulsos[4]{};
uint32_t pulsosIniciales[4]{};
uint32_t inicio = 0;

char comando[24];
size_t longitud = 0;
bool descartarLinea = false;

void IRAM_ATTR contarFL() { pulsos[0].fetch_add(1, std::memory_order_relaxed); }
void IRAM_ATTR contarRL() { pulsos[1].fetch_add(1, std::memory_order_relaxed); }
void IRAM_ATTR contarFR() { pulsos[2].fetch_add(1, std::memory_order_relaxed); }
void IRAM_ATTR contarRR() { pulsos[3].fetch_add(1, std::memory_order_relaxed); }

bool tieneCalibracion() {
  if (DIAMETRO_CM <= 0) {
    return false;
  }
  for (float total : PULSOS_CINCO_VUELTAS) {
    if (total <= 0) {
      return false;
    }
  }
  return true;
}

void leerDistancias(float& izquierda, float& derecha) {
  float distancia[4];
  const float circunferencia = 3.14159265f * DIAMETRO_CM;

  for (size_t i = 0; i < 4; ++i) {
    const uint32_t cuenta =
        pulsos[i].load(std::memory_order_relaxed) - pulsosIniciales[i];
    distancia[i] = cuenta * circunferencia * 5.0f / PULSOS_CINCO_VUELTAS[i];
  }

  izquierda = (distancia[0] + distancia[1]) / 2.0f;
  derecha = (distancia[2] + distancia[3]) / 2.0f;
}

void detener(const char* motivo) {
  digitalWrite(STBY, LOW);
  const Modo anterior = modo;
  modo = Modo::Parado;

  for (const Motor& motor : motores) {
    analogWrite(motor.pwm, 0);
    digitalWrite(motor.in1, LOW);
    digitalWrite(motor.in2, LOW);
  }

  if (anterior == Modo::Distancia) {
    float izquierda, derecha;
    leerDistancias(izquierda, derecha);
    Serial.printf("%s | estimado %.1f cm | izq %.1f cm | der %.1f cm\n",
                  motivo, (izquierda + derecha) / 2.0f, izquierda, derecha);
  } else {
    Serial.printf("%s | motores deshabilitados\n", motivo);
  }
}

void iniciar(Modo nuevoModo) {
  for (const Motor& motor : motores) {
    digitalWrite(motor.in1, motor.invertir ? LOW : HIGH);
    digitalWrite(motor.in2, motor.invertir ? HIGH : LOW);
    analogWrite(motor.pwm, PWM_PRUEBA);
  }
  for (size_t i = 0; i < 4; ++i) {
    pulsosIniciales[i] = pulsos[i].load(std::memory_order_relaxed);
  }

  inicio = millis();
  modo = nuevoModo;
  digitalWrite(STBY, HIGH);
}

void ejecutar(const char* texto) {
  if (strcmp(texto, "STOP") == 0) {
    detener("STOP");
    return;
  }
  if (modo != Modo::Parado) {
    Serial.println("En movimiento. Espera FIN o envia STOP.");
    return;
  }

  if (strcmp(texto, "AVANZA 30") == 0) {
    if (!tieneCalibracion()) {
      Serial.println("Completa el diametro y los cuatro conteos de cinco vueltas.");
      return;
    }
    iniciar(Modo::Distancia);
  } else if (strcmp(texto, "AVANZA") == 0) {
    iniciar(Modo::Pulso);
  } else {
    Serial.println("Comandos: AVANZA 30 | AVANZA | STOP");
  }
}

void setup() {
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW);
  Serial.begin(115200);

  for (const Motor& motor : motores) {
    pinMode(motor.in1, OUTPUT);
    pinMode(motor.in2, OUTPUT);
    pinMode(motor.pwm, OUTPUT);
    pinMode(motor.encoder, INPUT);
  }

  attachInterrupt(motores[0].encoder, contarFL, FALLING);
  attachInterrupt(motores[1].encoder, contarRL, FALLING);
  attachInterrupt(motores[2].encoder, contarFR, FALLING);
  attachInterrupt(motores[3].encoder, contarRR, FALLING);

  detener("LISTO");
  Serial.println("Comandos: AVANZA 30 | AVANZA | STOP");
}

void loop() {
  const uint32_t transcurrido = millis() - inicio;

  if (modo == Modo::Pulso && transcurrido >= PULSO_MS) {
    detener("FIN");
  } else if (modo == Modo::Distancia) {
    float izquierda, derecha;
    leerDistancias(izquierda, derecha);

    if (transcurrido >= LIMITE_MS) {
      detener("FIN POR TIEMPO");
    } else if ((izquierda + derecha) / 2.0f >= OBJETIVO_CM) {
      detener("FIN POR DISTANCIA");
    }
  }

  if (!Serial.available()) {
    return;
  }

  const char c = static_cast<char>(Serial.read());
  if (c == '\r') {
    return;
  }
  if (c == '\n') {
    if (!descartarLinea && longitud > 0) {
      comando[longitud] = '\0';
      ejecutar(comando);
    }
    longitud = 0;
    descartarLinea = false;
  } else if (longitud < sizeof(comando) - 1) {
    comando[longitud++] = c;
  } else {
    descartarLinea = true;
  }
}
