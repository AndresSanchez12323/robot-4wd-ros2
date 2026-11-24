# Relevo completo del robot 4WD

Corte del proyecto: 24 de noviembre de 2026. Proyecto de Programación Distribuida y Paralela. Este archivo contiene el estado de continuidad, el firmware completo de ambas placas, las pruebas, los respaldos y la memoria histórica. Empieza por esta sección. Cuando un registro antiguo indique un paso ya realizado o una configuración diferente, usa este resumen y el código V1.2 para verificar qué está vigente.

## Qué se busca

El robot de cuatro ruedas debe iniciar con ARRANCAR, avanzar y esquivar obstáculos a ambos lados. La marcha carece de un límite de recorrido. Si la salida todavía no está confirmada o faltan barridos temporalmente, el carro espera con la misma orden de marcha y reanuda al recuperar una salida válida. STOP cancela esa orden. La meta académica posterior es integrar percepción, localización y navegación con ROS 2 para ejecutar instrucciones como «ve a la izquierda de las gafas».

## Estado confirmado y avances

| Etapa | Evidencia y alcance |
|---|---|
| Montaje y calibración | Se montaron dos ESP32, cuatro motores, dos TB6612FNG, encoders, MPU6050, cámara OV5640 y LiDAR D6. Se hicieron pruebas parciales UART, de encoders y de distancia. La memoria histórica conserva sus mediciones y límites. |
| Primera integración, 14 de noviembre | Cámara, comunicación UART, MPU y D6 operaron simultáneamente. La corrección del checksum basada en el SDK del D6 quedó validada en capturas con más de 31 000 paquetes y cero errores. La carpeta de la prueba de 50 cm quedó respaldada. El recorrido físico exacto de 50 cm quedó sin certificar. |
| Navegación V1 | ARRANCAR/STOP, marcha continua, pivote izquierdo y configuración de frente y radio. Introdujo el protocolo entre placas V1/A1. |
| Navegación V1.1 | Se preservó la corrida durante esperas recuperables de espacio y se configuró la anticipación adicional de 150 mm. El usuario confirmó éxito con obstáculos cercanos y reportó giro solo a izquierda, interrupciones que exigían otro ARRANCAR y dificultades en ambientes abiertos. Esta versión probada se conserva en su carpeta y ZIP de respaldo. |
| Navegación V1.2 | Selección izquierda/derecha con las cuatro ruedas, reanudación tras tres barridos coherentes, reevaluación de giros largos, espera recuperable por espacio o LiDAR y diagnóstico del encoder que pierde pulsos. Se compiló para ambas placas con Arduino-ESP32 3.3.12. Pasaron las pruebas de software. Sigue pendiente confirmar la versión que corre en ambas placas y reproducir el problema físico con el diagnóstico visible. |
| ROS 2 | Se preparó la imagen base Docker de ROS 2 Humble y se verificó la importación. Quedan pendientes el puente con el carro, la publicación de sensores y la validación de navegación de alto nivel. El firmware autónomo actual implementa evasión reactiva. |

## Configuración vigente

- DevKitC V4, módulo ESP32-WROOM-32D. Maneja motores, encoders y MPU6050. En Arduino IDE: ESP32 Dev Module, Flash Mode DIO.
- ESP32-S3 con cámara OV5640. Maneja cámara, LiDAR y página de control. En Arduino IDE: ESP32S3 Dev Module, Flash 16 MB, PSRAM OPI, partición APP 3 MB / FATFS 9 MB y USB CDC On Boot Disabled para el USB-UART usado.
- Arduino IDE abre cada archivo principal desde su carpeta, con nombre idéntico: devkit_integral/devkit_integral.ino y s3_integral/s3_integral.ino. Las pestañas .h permanecen en la misma carpeta. Evitar abrir copias descargadas con sufijos como s3_integral[1].ino. Para el diagnóstico completo de V1.2, cargar ambos sketches de la misma entrega, con alimentación de motores apagada.
- D6: amarillo TX → S3 RX GPIO1, verde RX → S3 TX GPIO2, rojo → 5 V regulados, negro → GND común. UART a 230400. S3 TX41 → DevKit RX16, DevKit TX17 → S3 RX42, GND común, UART a 115200. MPU6050 en DevKit: VCC 3,3 V, SDA21, SCL22 y GND.
- La alimentación acordada usa el XL4016 ajustado a 5 V. Las señales GPIO son de 3,3 V. El mapa de motores, encoders y alimentación está en los anexos.
- Red local RobotCam, clave de fábrica del sketch CAMBIA_CLAVE_ROBOTCAM, panel en http://192.168.4.1/. Confirmar que la página muestre «Version 1.2». Mantener la página abierta durante la marcha.
- LiDAR centrado y radio declarado de 150 mm hasta la punta del chasis. El ángulo frontal se calibra con un objeto delante. Moverlo físicamente a la derecha permite comprobar si el ángulo crece en ese sentido. Confirmar en la página los valores aplicados. La configuración está en RAM y se vuelve a confirmar tras reiniciar el S3.
- El DevKit presentó un puerto COM6 intermitente y «Dispositivo USB desconocido (Error de restablecimiento de puerto)». Se respaldaron y desinstalaron dos paquetes CP210x antiguos y se instaló Silicon Labs 11.6.0.420. Posteriormente el usuario continuó con cargas y pruebas. El USB-C del S3 es una conexión distinta. Si reaparece, identificar el puerto real de cada placa y distinguir enumeración USB de compilación Arduino.

## Problema abierto y punto de reanudación

El usuario informa que el error persiste y pregunta si el D6 es demasiado sensible. Aún falta una captura tomada durante la interrupción que identifique la causa. Confirmar primero la versión del panel y anotar sectores, distancia mínima, mensaje tras «barridos», «Ultima parada» y la línea A1 del DevKit, antes de pulsar STOP. En este firmware, la consulta de /estado renueva el permiso del operador. Usar la información del panel existente en la prueba y evitar consultas automáticas que mantengan ese permiso sin el operador.

| Mensaje o condición | Significado en V1.2 |
|---|---|
| Obstaculo dentro del margen del chasis | Alguna lectura está dentro del radio más 100 mm. Con radio 150 mm, el umbral es 250 mm desde el centro del D6. Puede corresponder a un objeto real o a una parte del montaje. Comparar el punto con el entorno y el chasis. |
| Falta observar zona de giro | El planificador requiere mediciones en todas las celdas de la envolvente de giro. El lector puede declarar LiDAR OK con menos sectores válidos. En una zona abierta, celdas sin retorno pueden dejar el vehículo esperando. |
| Datos caducados o inválidos | Se ordenan velocidades cero hasta disponer de barridos nuevos y una salida confirmada. |
| Fallo de encoder, MPU, UART u operador en Ultima parada | Revisar el componente indicado. Estas condiciones cancelan la marcha y requieren ARRANCAR explícito tras resolver el fallo. |

El algoritmo toma el mínimo de dos sectores de 5 grados para cada celda de 10 grados. La anticipación de 150 mm se añade a la envolvente frontal. La distancia efectiva de frenado y la evasión de V1.2 en suelo siguen pendientes de medición. Ante una interrupción, conservar las protecciones mientras se identifica el motivo y evaluar la solución con pruebas reproducibles. El punto de entrada para investigar es Navegacion.h, SupervisorMarcha.h y el diagnóstico web de s3_integral.ino. Ejecutar las pruebas de escritorio con python navegacion_autonoma/pruebas/verificar.py desde la raíz del proyecto. El detalle de compilación y los resultados archivados aparecen abajo.

## Cómo reutilizar los anexos

Cada bloque de código está rotulado con su ruta relativa a la raíz del proyecto. Restaurar los archivos de la versión deseada conservando los nombres y la estructura de carpetas. Para programación directa también existe NAVEGACION_AUTONOMA_V1_2_COMPLETA.zip, con la estructura de Arduino lista para extraer. Las versiones y respaldos previos se incluyen como referencia para comparar comportamientos. La memoria histórica contiene también diagramas textuales, calibración, errores de carga, enlaces y muestras de código anteriores.

## A. Firmware vigente V1.2 y pruebas

### `navegacion_autonoma/LEEME.md`

```markdown
# Navegación autónoma V1.2 — izquierda/derecha y continuidad

24 de noviembre de 2026. Antes de cambiar se guardó `prueba_autonoma_exitosa_2026-11-24`: 15 archivos de V1.1 verificados por SHA256. El usuario confirmó éxito de esa prueba con objetos cercanos, pero reportó giro solo a izquierda y paradas que requerían otro ARRANCAR. El respaldo original del 14 de noviembre también permanece intacto.

## Qué cambió

- Elige izquierda o derecha comparando el espacio medido entre 20 y 100 grados de cada lado del frente. Conserva la decisión durante el giro y usa 100mm de diferencia para evitar cambiar de lado por ruido.
- Izquierda: FL/RL negativos y FR/RR positivos. Derecha: FL/RL positivos y FR/RR negativos. Se usan las cuatro ruedas, con PWM máximo80 y el mapa de GPIO probado.
- Tras tres barridos nuevos con trayectoria frontal libre, envía avance a las cuatro ruedas en la misma corrida. No hay límite de centímetros ni duración del avance.
- Un giro de 6,5 segundos sin salida deja de cancelar la corrida: pausa, prueba el otro lado si la envolvente de giro está observada y libre, y sigue reevaluando. Las pausas al invertir los motores permanecen.
- Si falta espacio o una zona del radar es desconocida, espera con motores parados y conserva ARRANCAR. Retoma después de tres barridos nuevos que confirmen el mismo tipo de salida. No trata cero como distancia infinita ni elimina ecos próximos.
- Si un barrido es inválido o caduca, pasa a ESPERANDO_DATOS con velocidades cero, sin perder la corrida. Tras recuperar medidas válidas y tres barridos de salida, continúa. Si el sensor no vuelve, permanece detenido. Reenvía el comando documentado de inicio del D6 cada4s sin barrido válido; no se mueve con datos viejos.
- Una caída de video ya no cancela una marcha con LiDAR y control sanos. Se conserva el latido del navegador. Se puede reconectar el video durante la misma corrida.
- Página muestra Última parada, que se conserva aunque cambie el mensaje de estado. Un encoder bloqueado se identifica como FL, RL, FR o RR en el DevKit.

## Cargar los códigos completos

Abrir en Arduino IDE desde ESTA carpeta, sin pegar fragmentos:

| Placa | Archivo | Pestañas necesarias |
|---|---|---|
| DevKitC | devkit_integral/devkit_integral.ino | EnlaceRobot.h, GuardiaMarcha.h |
| S3 con cámara | s3_integral/s3_integral.ino | EnlaceRobot.h, EstadoIntegral.h, LidarD6.h, Navegacion.h, SupervisorMarcha.h |

Para recibir también el diagnóstico específico de ruedas cargar AMBAS placas. Protocolo V1/A1 permanece compatible con el DevKit autónomo anterior; no mezclar con la antigua prueba de50cm. No se cargó firmware automáticamente. No hay OTA.

DevKit: ESP32 Dev Module, Flash Mode DIO. S3: ESP32S3 Dev Module; Flash16MB; PSRAM OPI; partición APP3MB/FATFS9MB; USB CDC On Boot Disabled para el USB-UART usado. Paquete compilado con Arduino-ESP32 instalado. Programar con motores sin alimentación y manteniendo el aislamiento USB/alimentación externa adoptado.

No cambia cableado: D6 amarilloTX→S3GPIO1RX, verdeRX→S3GPIO2TX, rojo5V y negroGND; UART230400. S3TX41→DevKitRX16, DevKitTX17→S3RX42, GND común; UART115200. MPU VCC3V3, GND, SDA21, SCL22 en DevKit. XL4016 a5V según decisión previa; GPIO nunca a5V.

## Configuración de dirección física

Wi-Fi RobotCam, clave CAMBIA_CLAVE_ROBOTCAM, página http://192.168.4.1/. La página debe identificar versión1.2. Mantener la página abierta. Sin internet es normal en esa red; perder RobotCam no es normal para controlar.

1. Con el robot parado, confirmar frente y radio150mm. Centrar un objeto delante y seleccionar su ángulo en el radar; no convertir15cm en grados. Comparar la línea de valores aplicados con los campos escritos.
2. Nuevo selector obligatorio: sentido angular del LiDAR. Mover el objeto del frente un poco hacia la derecha física del carro y observar si su ángulo aumenta o disminuye. Elegir crece hacia derecha si aumenta y crece hacia izquierda si disminuye. En el cruce359→0 aumenta; 0→359 disminuye. No asumir un sentido sin comprobarlo. El dibujo del radar por sí solo no demuestra la orientación física del sensor.
3. Pulsar Confirmar frente y radio y comprobar que la línea aplicada incluye el sentido elegido. La configuración permanece en RAM y debe confirmarse al reiniciarS3. Cambiarla cancela la corrida para impedir modificar geometría en marcha.
4. Retirar el objeto de referencia y comprobar LiDAR, UART, MPU y operador. La cámara puede estar pendiente sin cancelar el permiso de navegación.
5. Para validar los dos sentidos, primero comprobarlos con ruedas elevadas. Luego en suelo libre ensayar un obstáculo delante con más espacio a derecha y después con más espacio a izquierda. Mantener STOP y el interruptor accesibles. Registrar capturas con motivo y Última parada si algo falla.

## Distancias y expectativas

Anticipación adicional150mm, radio informado150mm y margen100mm. El límite global de proximidad sigue250mm desde el eje del LiDAR. La envolvente frontal incluye150mm extra y ampliación angular. No significa permitir contacto a150mm desde el sensor; el chasis ya ocupa ese radio. No se ha medido la distancia física real de frenado con V1.2.

Se usan36 celdas de10grados, mínimo real de cada par de sectores5grados. Una celda sin medidas sigue desconocida. El lector D6 mantiene checksum, edades y requisitos de cobertura; una habitación con pocos retornos puede impedir certificar una maniobra. No se supone que distancias sin retorno sean espacio libre.

El algoritmo es reactivo, no aprendizaje por refuerzo ni un mapa. No puede prometer seguir avanzando si está encerrado, faltan datos o se avería una rueda. Mantiene la intención de marcha durante esperas recuperables; STOP la cancela. No se borran protecciones para ocultar fallos.

## Casos de parada y recuperación

| Caso | Motores | ¿Conserva la corrida? | Recuperación |
|---|---|---|---|
| Obstáculo frontal con giro libre | Pausa y pivote elegido | Sí | Avance tras tres barridos libres |
| Corrección prolongada | Pausa y cambia lado | Sí | Reevalúa sin ARRANCAR nuevo |
| Obstáculo dentro del margen | Cero | Sí | Cuando haya espacio observado |
| Zona necesaria sin medida | Cero | Sí | Cuando se observe una salida |
| Barrido inválido/caducado o sensor sin datos | Cero | Sí | Datos válidos y tres barridos de salida |
| Video interrumpido | Navegación permitida con LiDAR sano | Sí | Reconectar video |
| STOP en página o USB | Cero | No | ARRANCAR explícito |
| Cambio de frente/radio/sentido | Cero | No | Configurar y ARRANCAR |
| Navegador ausente1,2s | Cero | No | Reconectar y ARRANCAR |
| Sin órdenes UART450ms en DevKit | Cero | No | Resolver enlace y ARRANCAR |
| Sin telemetría DevKit600ms en S3 | Cero | No | Resolver enlace y ARRANCAR |
| MPU sin datos | Cero | No | Resolver cableado, reiniciar si faltó al inicio y ARRANCAR |
| FL/RL/FR/RR activada sin pulsos1,5s | Cero | No | Revisar esa rueda/encoder y ARRANCAR |
| Reinicio de una placa | Cero | No | Sincronización, configurar y ARRANCAR |
| CRC incorrecto, orden fuera de rango, atrasada o duplicada | No se aplica esa orden | No renueva watchdog | Corregir enlace; sin rearme por orden vieja |

Las pausas por reversión del motor no se eliminan: protegen el cambio de sentido. Los fallos reales de UART/MPU/encoder no se recuperan con un bucle que fuerce un nuevo identificador de corrida.

## Pruebas antes de entregar

Ejecutar `python pruebas/verificar.py` desde esta carpeta para pruebas en PC (WSL cong++ y Node), más compilación Arduino separada. Los tests no controlan el vehículo físico.

- Protocolo: CRC y checksum D6, corrupción, resincronización, barrido incompleto, caducidad y desconocidos.
- Planificador: avance largo, pausa, salida estable, recuperación sin ARRANCAR, STOP y fallo enclavado.
- Casos base:28 combinaciones de frente y sentido físico con elección de ambos lados;2 escenas con obstáculo cambiante;32 combinaciones de salud; correcciones largas, sensor recuperado, ceros, STOP y fallo real.
- Motores: sketch real con dobles GPIO/UART/I2C; cuatro inversiones, pausa de reversión, UART tardía, watchdog y detección individual de cuatro encoders.
- Integración: planificador→tramaCRC→receptorUART→sketchDevKit con dobles; giroderecho, corrección larga, esperaLiDAR, retorno al avance y STOP conservando una sola corrida durante las maniobras.
- Interfaz: JavaScript real con DOM simulado; habilitación, STOP, ángulo0, configuración y desconexión.

Estado: verificaciones de software y compilación; validación física V1.2 pendiente. El usuario confirmó la pruebaV1.1 respaldada, no estas modificaciones nuevas.
```

### `navegacion_autonoma/VERIFICACION_V1_2.txt`

```text
Verificacion de compilacion Arduino — 24-nov-2026
DevKit: esp32:esp32:esp32:FlashMode=dio
Salida: Sketch uses 327991 bytes (25%); Global variables use 24896 bytes (7%). Exit code 0.
S3: esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=default
Salida: Sketch uses 1001782 bytes (31%); Global variables use 59544 bytes (18%). Exit code 0.
Pruebas C++ ejecutadas con -std=c++17 -Wall -Wextra -Werror -pedantic.
Interfaz real verificada con Node/DOM simulado. Resultados: pruebas/resultado_pruebas.txt.
Ningun firmware se cargo al vehiculo automaticamente. Falta prueba fisica de V1.2.

SHA256 de los fuentes de firmware verificados:
3a28cbfff2f85a71d96ca4dd3eba8c9eb265b0b26623fa47426aaaa1cfc4f7ff  devkit_integral/devkit_integral.ino
c90b940b9e9032b232d8a4212f3786c72321a66cd426a6a954ce8c87ac178a24  devkit_integral/EnlaceRobot.h
387bedd6033b4ff52f3aa7c7877dc47ede9c203041ee3e23c4da13d7d4cea3a0  devkit_integral/GuardiaMarcha.h
c90b940b9e9032b232d8a4212f3786c72321a66cd426a6a954ce8c87ac178a24  s3_integral/EnlaceRobot.h
90bc57953b84f481a7d64eb559ed80ba4ea2fd6c9de50cec76938c2b96ed76da  s3_integral/EstadoIntegral.h
f173d33189feeecf20f6527ce96a81cb8b31768bbd2f57f5708101021b51b541  s3_integral/LidarD6.h
4474ac3d4e8d22bd0337f9733a151c2a078a372c96117d21853e996e65ba66c5  s3_integral/Navegacion.h
90f6eca478f196e918c3454fbeeae824f1fe0bd46126fdf17ca9ef5414752abc  s3_integral/s3_integral.ino
873781a8ced8b3c716f1a36966c13463986b882e1de3f0e78a7783b38b949a7e  s3_integral/SupervisorMarcha.h
```

### `navegacion_autonoma/devkit_integral/devkit_integral.ino`

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "EnlaceRobot.h"
#include "GuardiaMarcha.h"

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
uint32_t ultimoReporte = 0;
uint32_t ultimaLecturaIMU = 0, ultimoIntentoIMU = 0;
bool moviendo = false;
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
  for (int valor : v) if (valor < -80 || valor > 80) return;
  if (!corrida) for (int valor : v) if (valor) return;
  const uint32_t anterior = guardia.corrida;
  if (!guardia.aceptar(sesion, secuencia, corrida, millis())) return;
  if (!corrida) { for (int &valor : objetivo) valor = 0; apagarSalidas(); return; }
  if (!imuValida || millis() - ultimaLecturaIMU > 250) { detener("MPU_SIN_DATOS"); return; }
  if (corrida != anterior) {
    copiarPulsos(iniciales);
    copiarPulsos(anteriores);
    for (uint32_t &t : ultimoPulso) t = millis();
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
  Serial.printf("AUTONOMO DEVKIT V1 | MPU=%s | motores deshabilitados\n", mpu ? "SI" : "NO");
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
    if (!aplicado[i] || cuentas[i] != anteriores[i]) ultimoPulso[i] = ahora;
    anteriores[i] = cuentas[i];
    if (aplicado[i] && ahora - ultimoPulso[i] > LIMITE_ENCODER_MS) {
      detener(FALLO_ENCODER[i]); break;
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
  delay(1);
}
```

### `navegacion_autonoma/devkit_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `navegacion_autonoma/devkit_integral/GuardiaMarcha.h`

```cpp
#pragma once
#include <stdint.h>

// Cada ARRANCAR crea una corrida nueva. Un fallo/STOP invalida esa corrida:
// recibir otra vez la misma orden no vuelve a poner los motores en marcha.
class GuardiaMarcha {
 public:
  uint32_t sesion = 0, secuencia = 0, corrida = 0, ultimaCorrida = 0, bloqueada = 0, ultimo = 0;
  bool recibido = false;
  void bloquear() { if (corrida) bloqueada = corrida; corrida = 0; }
  bool vigente(uint32_t ahora) const { return recibido && ahora - ultimo <= 450; }
  bool aceptar(uint32_t s, uint32_t q, uint32_t r, uint32_t ahora) {
    if (!s) return false;
    if (s != sesion) {
      if (r != 0) return false; // reinicio requiere primero sincronizacion en reposo.
      sesion = s; secuencia = 0; corrida = ultimaCorrida = bloqueada = 0; recibido = false;
    }
    if (static_cast<int32_t>(q - secuencia) <= 0) return false;
    if (r && (r == bloqueada || static_cast<int32_t>(r - ultimaCorrida) < 0)) return false;
    secuencia = q; ultimo = ahora; recibido = true;
    if (!r) bloquear();
    else { corrida = r; ultimaCorrida = r; }
    return true;
  }
};
```

### `navegacion_autonoma/s3_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `navegacion_autonoma/s3_integral/EstadoIntegral.h`

```cpp
#pragma once
#include <stdint.h>

struct EstadoIntegral {
  bool activa = false, configurada = false;
  bool horario = true;
  int frente = 0, radio = 150;
  bool permiso = false;
  bool camara = false;
  bool lidar = false;
  bool enlace = false;
  bool imu = false;
  bool moviendo = false;
  bool operador = false;
  uint16_t minimo = 0;
  uint8_t sectores = 0;
  uint32_t paquetes = 0;
  uint32_t bytesLidar = 0;
  uint32_t erroresCabecera = 0;
  uint32_t erroresChecksum = 0;
  uint32_t iniciosLidar = 0;
  char diagnosticoLidar[180] = "Esperando bytes del LiDAR";
  uint32_t errores = 0;
  uint32_t barridos = 0;
  float fps = 0;
  float distancia = 0;
  uint16_t mapa[72]{};
  char devkit[300] = "Esperando DevKitC";
  char evento[96] = "Esperando video, LiDAR y DevKitC";
  char ultimaParada[96] = "Ninguna";
};
```

### `navegacion_autonoma/s3_integral/LidarD6.h`

```cpp
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
        completo = !corrupto && cobertura >= 60 && puntos >= 100 &&
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
```

### `navegacion_autonoma/s3_integral/Navegacion.h`

```cpp
#pragma once
#include <stdint.h>
#include <math.h>

struct ConfigNavegacion {
  int frente = 0;
  int radio = 0;
  bool confirmada = false;
  bool horario = true; // Verificar con objeto: angulo crece hacia la derecha fisica.
  bool valida() const { return confirmada && frente >= 0 && frente < 360 && radio >= 100 && radio <= 1000; }
};

// Planificador reactivo: conserva la corrida durante esperas recuperables.
class Navegacion {
 public:
  static constexpr float ANTICIPACION_MM = 150.0f;
  enum Fase { PARADO, RECTO, PAUSA_GIRO, IZQUIERDA, DERECHA, ESPERANDO_ESPACIO, ESPERANDO_DATOS, FALLO };
  Fase fase = PARADO;
  int izquierda = 0, derecha = 0;
  int sentido = 1; // +1 izquierda fisica, -1 derecha fisica.
  const char *motivo = "Pulsa ARRANCAR";
  bool activa() const { return fase != PARADO && fase != FALLO; }
  void parar(const char *causa = "Detenido por el operador") { fase = PARADO; cero(); motivo = causa; }
  void fallar(const char *causa) { fase = FALLO; cero(); motivo = causa; }
  void esperar(const char *causa) { fase = ESPERANDO_ESPACIO; cero(); motivo = causa; libres = salida = 0; }
  bool arrancar(const ConfigNavegacion &c, uint32_t ahora) {
    if (!c.valida()) { fallar("Configura frente, radio y sentido del LiDAR"); return false; }
    fase = RECTO; cero(); inicioFase = ahora; ultimoBarrido = UINT32_MAX;
    libres = salida = 0; sentido = 1; motivo = "Evaluando barrido antes de mover"; return true;
  }
  static uint16_t medida(const uint16_t *mapa, int i) {
    const uint16_t a = mapa[2*i], b = mapa[2*i+1];
    return !a ? b : !b ? a : a < b ? a : b;
  }
  static float relativo(int celda, const ConfigNavegacion &c) {
    float a = celda * 10.0f + 5.0f - c.frente;
    while (a > 180) a -= 360;
    while (a <= -180) a += 360;
    return c.horario ? -a : a; // positivo = izquierda fisica.
  }
  const char *evaluar(const uint16_t *mapa, const ConfigNavegacion &c, bool giro = false) const {
    const float radio = c.radio + 100.0f;
    for (int i = 0; i < 36; ++i) {
      const uint16_t d = medida(mapa,i);
      const float a = relativo(i,c) * 0.01745329252f;
      const float x = cosf(a), y = sinf(a);
      if (d && d <= radio) return "Obstaculo dentro del margen del chasis";
      if (giro && !d) return "Falta observar zona de giro";
      if (giro || x <= 0) continue;
      if (!d) return "Falta medida en la zona de avance";
      const float ancho = radio + d * 0.08716f;
      const float px = d*x, py = fabsf(d*y);
      const float dx = px > ANTICIPACION_MM ? px-ANTICIPACION_MM : 0.0f;
      if (dx*dx + py*py <= ancho*ancho) return "Obstaculo en trayectoria";
    }
    return nullptr;
  }
  float espacioLado(const uint16_t *mapa, const ConfigNavegacion &c, int lado) const {
    float espacio = 2000.0f;
    unsigned vistos = 0;
    for (int i=0;i<36;++i) {
      const float a = relativo(i,c)*lado;
      if (a < 20 || a > 100) continue;
      const uint16_t d = medida(mapa,i);
      if (!d) return 0; // desconocido no es una salida.
      if (d < espacio) espacio = d;
      ++vistos;
    }
    return vistos >= 6 ? espacio : 0;
  }
  int elegirLado(const uint16_t *mapa, const ConfigNavegacion &c) const {
    const float l = espacioLado(mapa,c,1), r = espacioLado(mapa,c,-1);
    // Histeresis de 10 cm: en empate conservar decision, sin zigzag por ruido.
    if (l > r + 100) return 1;
    if (r > l + 100) return -1;
    return sentido;
  }
  void prepararGiro(int lado, uint32_t ahora) {
    sentido = lado; fase = PAUSA_GIRO; inicioFase = ahora; libres = 0; cero();
    motivo = lado > 0 ? "Preparando giro izquierdo" : "Preparando giro derecho";
  }
  void actualizar(const uint16_t *mapa, const ConfigNavegacion &c, bool sensor,
                  uint32_t barrido, uint32_t ahora) {
    if (!activa()) return;
    if (!c.valida()) { fallar("Configuracion de geometria invalida"); return; }
    if (!sensor) {
      fase = ESPERANDO_DATOS; cero(); libres = salida = 0;
      ultimoBarrido = barrido; motivo = "Esperando barridos validos; reanudacion automatica"; return;
    }
    if (barrido == ultimoBarrido) return;
    ultimoBarrido = barrido;
    const char *frente = evaluar(mapa,c);
    const char *giro = evaluar(mapa,c,true);
    if (fase == ESPERANDO_DATOS || fase == ESPERANDO_ESPACIO) {
      const uint8_t candidata = !frente ? 1 : !giro ? 2 : 0;
      if (!candidata) { libres = salida = 0; motivo = giro; return; }
      if (salida != candidata) libres = 0;
      salida = candidata;
      if (++libres < 3) { motivo = "Confirmando salida en tres barridos nuevos"; return; }
      if (candidata == 1) avanzar();
      else prepararGiro(elegirLado(mapa,c),ahora);
      return;
    }
    if (fase == RECTO) {
      if (!frente) { avanzar(); return; }
      if (giro) { esperar(giro); return; }
      prepararGiro(elegirLado(mapa,c),ahora); return;
    }
    if (giro) { esperar(giro); return; }
    // Sin salida frontal no cancelar corrida a los 6,5 s: probar el otro lado
    // con envolvente libre, pausa de inversion y mismo identificador de corrida.
    if (ahora - inicioFase > 6500) {
      prepararGiro(-sentido,ahora); motivo = "Reevaluando: probando el otro lado"; return;
    }
    if (fase == PAUSA_GIRO) {
      if (ahora - inicioFase >= 250) pivotar();
      return;
    }
    if (!frente) ++libres; else libres = 0;
    if (libres >= 3) { avanzar(); return; }
    pivotar();
  }
 private:
  uint32_t inicioFase = 0, ultimoBarrido = UINT32_MAX;
  uint8_t libres = 0, salida = 0;
  void cero() { izquierda = derecha = 0; }
  void avanzar() { fase = RECTO; izquierda = derecha = 80; motivo = "Avanzando; correccion automatica activa"; }
  void pivotar() {
    fase = sentido > 0 ? IZQUIERDA : DERECHA;
    izquierda = -80 * sentido; derecha = 80 * sentido;
    motivo = sentido > 0 ? "Corrigiendo a izquierda" : "Corrigiendo a derecha";
  }
};
```

### `navegacion_autonoma/s3_integral/s3_integral.ino`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "EnlaceRobot.h"
#include "LidarD6.h"
#include "EstadoIntegral.h"
#include "Navegacion.h"
#include "SupervisorMarcha.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Selecciona ESP32S3 Dev Module."
#endif

Navegacion navegacion;
std::atomic<uint32_t> geometria{0}; // RAM: vuelve a confirmar el frente tras reiniciar.
uint32_t corrida = 0, bloqueadaDevkit = 0;
ConfigNavegacion configuracion() {
  const uint32_t g = geometria.load();
  return {int(g & 511), int((g >> 9) & 2047), bool(g & 0x80000000u), !(g & 0x40000000u)};
}
HardwareSerial enlace(1);
HardwareSerial puertoLidar(2);
ReceptorRobot receptor;
LidarD6 lidar;
httpd_handle_t servidorWeb = nullptr, servidorVideo = nullptr;
QueueHandle_t solicitudesInicio = nullptr;
portMUX_TYPE muxEstado = portMUX_INITIALIZER_UNLOCKED;
EstadoIntegral estado;
std::atomic<bool> solicitudStop{false};
std::atomic<bool> videoActivo{false};
std::atomic<uint32_t> ultimoFrame{0}, ultimoNavegador{0};
std::atomic<uint32_t> fpsDecimas{0};
bool camaraIniciada = false, servidoresListos = false;
bool devkitIMU = false, devkitMoviendo = false, devkitRecibido = false;
float distanciaDevkit = 0;
uint32_t sesion = 0, secuencia = 0, orden = 0;
uint32_t ultimoDevkit = 0, ultimoLatido = 0, ultimoLog = 0;
uint32_t ultimoArranqueLidar = 0;
char mensajeDevkit[300] = "Esperando DevKitC";
char ultimaParada[96] = "Ninguna";
char motivoDevkit[48] = "Sin telemetria";
char evento[96] = "Abre la pagina y espera los indicadores";
char lineaUSB[32]{};
size_t longitudUSB = 0;
bool descartarUSB = false;

const char PAGINA[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot 4WD · navegación continua</title>
<style>
body{font:16px Arial;background:#142b43;color:#fff;margin:20px auto;max-width:1100px;padding:0 16px}
h1{font-size:25px}.paneles{display:flex;gap:16px;flex-wrap:wrap}.panel{flex:1;min-width:280px}
img,canvas{width:100%;max-width:520px;background:#203c55;border-radius:12px}
button{font-size:19px;padding:14px 22px;border:0;border-radius:8px;margin:8px 10px 8px 0;cursor:pointer}
#avanzar{background:#77dec4}#parar{background:#ff6666;font-weight:bold}button:disabled{opacity:.4}
pre{white-space:pre-wrap;background:#203c55;padding:14px;border-radius:8px;overflow-wrap:anywhere}
</style></head><body>
<h1>Robot 4WD · navegación continua</h1>
<p>Version 1.2: izquierda/derecha; reanudacion automatica | anticipacion de evasion: 150 mm mas el margen del chasis.</p>
<p>ARRANCAR inicia el avance sin límite de centímetros. Ante un obstáculo elige izquierda o derecha según el espacio observado. Si falta espacio, espera y reanuda al recuperarlo. PARAR / STOP cancela la marcha. La espera por LiDAR o espacio se recupera sola. STOP, fallos de enlace, MPU o rueda cancelan la marcha. El video puede reconectarse sin cancelar una marcha con LiDAR sano.</p>
<div class="paneles"><div class="panel"><h2>Camara OV5640</h2>
<img id="video" alt="Video de la camara"><br>
<button onclick="reconectar()">Reconectar video</button>
<a style="color:#8deadb" href="/captura" target="_blank">Captura</a></div>
<div class="panel"><h2>LiDAR · vista de 72 sectores</h2>
<canvas id="radar" width="420" height="420"></canvas>
<p>0 grados es la referencia del LiDAR. El dibujo no supone el frente del carro. Escala 3 m.</p></div></div>
<p>Configuración antes de arrancar: coloca un objeto delante del carro y pulsa su punto en el radar para elegir el ángulo frontal. Retira el objeto antes de arrancar.</p>
<label>Frente del carro (grados): <input id="frente" type="number" min="0" max="359" step="1" placeholder="Selecciona en radar"></label>
<label>Radio hasta la esquina (mm): <input id="radio" type="number" min="100" max="1000" value="150"></label>
<label>Sentido angular del LiDAR: <select id="sentido"><option value="">Selecciona tras comprobar</option><option value="1">Crece hacia la derecha física</option><option value="-1">Crece hacia la izquierda física</option></select></label>
<p>Con el carro detenido, mueve el objeto del frente a la derecha: confirma si el ángulo aumenta o disminuye. Este ajuste orienta la elección de lado.</p>
<button onclick="configurar()">Confirmar frente y radio</button><p id="configActual">Frente sin confirmar. Radio indicado por ti: 150 mm.</p>
<button id="avanzar" disabled onclick="mandar('avanzar')">ARRANCAR</button>
<button id="parar" onclick="mandar('stop')">PARAR / STOP</button>
<pre id="estado">Esperando datos...</pre><pre id="diagnostico"></pre><pre id="devkit"></pre>
<p>Mantener esta pagina abierta. Al perder el enlace con la pagina se cancela el movimiento.</p>
<script>
const boton=document.getElementById('avanzar');
const radar=document.getElementById('radar');const ctx=radar.getContext('2d');
radar.addEventListener('click',e=>{const b=radar.getBoundingClientRect();const x=(e.clientX-b.left)/b.width*420-210,y=(e.clientY-b.top)/b.height*420-210;
 if(Math.hypot(x,y)<8)return;document.getElementById('frente').value=Math.round((Math.atan2(x,-y)*180/Math.PI+360)%360)%360});
async function configurar(){
 const f=document.getElementById('frente'),r=document.getElementById('radio');
 const sentido=document.getElementById('sentido').value;
 if(sentido!=='1'&&sentido!=='-1'){alert('Confirma hacia que lado aumentan los angulos del LiDAR.');return}
 if(!f.value||!r.value||!f.checkValidity()||!r.checkValidity()){alert('Indica el ángulo frontal y el radio real del chasis.');return}
 try{const res=await fetch('/configurar?frente='+f.value+'&radio='+r.value+'&confirmar=1&sentido='+document.getElementById('sentido').value,{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('configActual').textContent=await res.text()}catch(e){}
}
function reconectar(){document.getElementById('video').src='http://'+location.hostname+':81/stream?t='+Date.now()}
async function mandar(accion){
 boton.disabled=true;
 try{const r=await fetch('/'+accion,{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('estado').textContent=await r.text()}catch(e){}
}
function dibujar(mapa,frente,configurada){
 ctx.clearRect(0,0,420,420);ctx.strokeStyle='#56738b';ctx.fillStyle='#8deadb';
 for(let r=42;r<=168;r+=42){ctx.beginPath();ctx.arc(210,210,r,0,2*Math.PI);ctx.stroke()}
 ctx.beginPath();ctx.moveTo(210,20);ctx.lineTo(210,400);ctx.moveTo(20,210);ctx.lineTo(400,210);ctx.stroke();
 ctx.font='14px Arial';ctx.fillText('0 grados',220,20);
 mapa.forEach((d,i)=>{if(d>0&&d<=3000){const a=i*5*Math.PI/180;
 const r=d/3000*180;ctx.beginPath();ctx.arc(210+Math.sin(a)*r,210-Math.cos(a)*r,3,0,2*Math.PI);ctx.fill()}});
 ctx.fillStyle='#ffcc55';ctx.fillRect(205,205,10,10);
 if(configurada){const a=frente*Math.PI/180;ctx.strokeStyle='#ffcc55';ctx.beginPath();ctx.moveTo(210,210);ctx.lineTo(210+Math.sin(a)*110,210-Math.cos(a)*110);ctx.stroke()}
}
async function actualizar(){
 try{
  const r=await fetch('/estado',{cache:'no-store'});if(!r.ok)throw Error();const d=await r.json();
  const si=v=>v?'OK':'PENDIENTE';
  document.getElementById('estado').textContent=
   'Camara: '+si(d.camara)+' | FPS enviados: '+d.fps+'\n'+
   'LiDAR: '+si(d.lidar)+' | sectores: '+d.sectores+'/72 | minimo: '+d.minimo+' mm\n'+
   'UART: '+si(d.enlace)+' | MPU: '+si(d.imu)+' | operador: '+si(d.operador)+'\n'+
   'Recorrido de ruedas (incluye giros): '+d.distancia+' cm | marcha autónoma: '+(d.activa?'ACTIVA':'PARADA')+' | motores: '+(d.moviendo?'SI':'NO')+'\n'+
   'LiDAR paquetes: '+d.paquetes+' | errores: '+d.errores+' | barridos: '+d.barridos+'\n'+d.evento+'\nUltima parada: '+d.ultimaParada;
  document.getElementById('devkit').textContent='DevKitC\n'+d.devkit;
  document.getElementById('diagnostico').textContent=
   'DIAGNOSTICO D6 | bytes: '+d.bytesLidar+' | cabeceras invalidas: '+d.erroresCabecera+
   ' | checksum invalidos: '+d.erroresChecksum+' | inicios validos: '+d.iniciosLidar+'\n'+(d.lidar?'Barridos válidos recibidos':d.diagnosticoLidar);
  boton.disabled=!(d.permiso&&!d.activa);
  document.getElementById('configActual').textContent=d.configurada?'Frente confirmado: '+d.frente+' grados | radio: '+d.radio+' mm | angulos crecen hacia '+(d.horario?'derecha':'izquierda'):'Confirma el frente del carro para habilitar la marcha';
  dibujar(d.mapa,d.frente,d.configurada);
 }catch(e){boton.disabled=true;document.getElementById('estado').textContent='Sin datos actuales del S3. Comprueba RobotCam; los valores anteriores pueden estar desactualizados.';
 document.getElementById('devkit').textContent='DevKitC: sin telemetria actual; esperando reconexion';
 document.getElementById('diagnostico').textContent='Sin diagnostico actual; esperando reconexion';}
 setTimeout(actualizar,300);
}
reconectar();actualizar();
</script></body></html>
)HTML";

bool peticionLocal(httpd_req_t *req) {
  char valor[4];
  return httpd_req_get_hdr_value_str(req, "X-Robot", valor, sizeof(valor)) == ESP_OK &&
         strcmp(valor, "1") == 0;
}

esp_err_t paginaWeb(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, PAGINA, HTTPD_RESP_USE_STRLEN);
}

esp_err_t estadoWeb(httpd_req_t *req) {
  ultimoNavegador.store(millis());
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  char mapa[512];
  size_t usados = 0;
  for (size_t i = 0; i < 72; ++i) {
    usados += snprintf(mapa + usados, sizeof(mapa) - usados, "%s%u", i ? "," : "", copia.mapa[i]);
  }
  char json[2800];
  snprintf(json, sizeof(json),
    "{\"version\":\"1.2\",\"horario\":%u,\"ultimaParada\":\"%s\",\"activa\":%u,\"configurada\":%u,\"frente\":%d,\"radio\":%d,\"permiso\":%u,\"camara\":%u,\"lidar\":%u,\"enlace\":%u,\"imu\":%u,"
    "\"moviendo\":%u,\"operador\":%u,\"minimo\":%u,\"sectores\":%u,\"paquetes\":%lu,"
    "\"errores\":%lu,\"barridos\":%lu,\"fps\":%.1f,\"distancia\":%.1f,"
    "\"bytesLidar\":%lu,\"erroresCabecera\":%lu,\"erroresChecksum\":%lu,\"iniciosLidar\":%lu,"
    "\"diagnosticoLidar\":\"%s\","
    "\"devkit\":\"%s\",\"evento\":\"%s\",\"mapa\":[%s]}",
    copia.horario, copia.ultimaParada, copia.activa, copia.configurada, copia.frente, copia.radio, copia.permiso, copia.camara, copia.lidar, copia.enlace, copia.imu, copia.moviendo,
    copia.operador, copia.minimo, copia.sectores, (unsigned long)copia.paquetes,
    (unsigned long)copia.errores, (unsigned long)copia.barridos, copia.fps, copia.distancia,
    (unsigned long)copia.bytesLidar, (unsigned long)copia.erroresCabecera,
    (unsigned long)copia.erroresChecksum, (unsigned long)copia.iniciosLidar, copia.diagnosticoLidar,
    copia.devkit, copia.evento, mapa);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

esp_err_t avanzarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  if (!copia.permiso || !copia.enlace || !copia.imu || copia.activa) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan condiciones. Revisa los indicadores.");
  }
  const uint32_t ahora = millis();
  if (xQueueSend(solicitudesInicio, &ahora, 0) != pdPASS) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ya hay una orden pendiente.");
  }
  return httpd_resp_send(req, "Orden recibida. Esperando respuesta del DevKitC.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t configurarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado); copia = estado; portEXIT_CRITICAL(&muxEstado);
  if (copia.activa || copia.moviendo) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Pulsa STOP antes de configurar");
  char query[120], f[12], r[12], c[4], sentido[4];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "frente", f, sizeof(f)) != ESP_OK ||
      httpd_query_key_value(query, "radio", r, sizeof(r)) != ESP_OK ||
      httpd_query_key_value(query, "confirmar", c, sizeof(c)) != ESP_OK || strcmp(c, "1") != 0 ||
      httpd_query_key_value(query, "sentido", sentido, sizeof(sentido)) != ESP_OK ||
      (strcmp(sentido, "1") != 0 && strcmp(sentido, "-1") != 0))
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan frente, radio y confirmacion");
  char *finF, *finR;
  long frente = strtol(f, &finF, 10), radio = strtol(r, &finR, 10);
  if (!*f || !*r || *finF || *finR || frente < 0 || frente >= 360 || radio < 100 || radio > 1000)
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Geometria fuera de rango");
  geometria.store(0x80000000u | (strcmp(sentido, "-1") == 0 ? 0x40000000u : 0u) | (uint32_t(radio) << 9) | uint32_t(frente));
  solicitudStop.store(true); // evita carreras con un ARRANCAR simultaneo.
  return httpd_resp_send(req, "Frente y radio confirmados. Retira el objeto de referencia.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t pararWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  solicitudStop.store(true);
  return httpd_resp_send(req, "STOP solicitado", HTTPD_RESP_USE_STRLEN);
}

esp_err_t capturaWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  camera_fb_t *imagen = esp_camera_fb_get();
  if (!imagen) return httpd_resp_send_500(req);
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  const esp_err_t resultado = httpd_resp_send(req, (const char *)imagen->buf, imagen->len);
  esp_camera_fb_return(imagen);
  return resultado;
}

esp_err_t videoWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  esp_err_t resultado = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=robotframe");
  if (resultado != ESP_OK) return resultado;
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  videoActivo.store(true);
  uint32_t cuadros = 0, inicioFPS = millis();
  while (true) {
    camera_fb_t *imagen = esp_camera_fb_get();
    if (!imagen) { resultado = ESP_FAIL; break; }
    char cabecera[100];
    const int n = snprintf(cabecera, sizeof(cabecera),
      "\r\n--robotframe\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)imagen->len);
    resultado = httpd_resp_send_chunk(req, cabecera, n);
    if (resultado == ESP_OK) resultado = httpd_resp_send_chunk(req, (const char *)imagen->buf, imagen->len);
    esp_camera_fb_return(imagen);
    if (resultado != ESP_OK) break;
    ultimoFrame.store(millis());
    ++cuadros;
    const uint32_t transcurrido = millis() - inicioFPS;
    if (transcurrido >= 3000) {
      fpsDecimas.store(cuadros * 10000UL / transcurrido);
      inicioFPS = millis();
      cuadros = 0;
    }
    delay(1);
  }
  videoActivo.store(false);
  fpsDecimas.store(0);
  return resultado;
}

bool registrarRuta(httpd_handle_t servidor, const char *uri, httpd_method_t metodo,
                   esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t ruta = {};
  ruta.uri = uri;
  ruta.method = metodo;
  ruta.handler = handler;
  return httpd_register_uri_handler(servidor, &ruta) == ESP_OK;
}

bool iniciarServidores() {
  httpd_config_t web = HTTPD_DEFAULT_CONFIG();
  web.server_port = 80;
  web.stack_size = 8192;
  web.send_wait_timeout = 2;
  if (httpd_start(&servidorWeb, &web) != ESP_OK) return false;
  bool bien = registrarRuta(servidorWeb, "/", HTTP_GET, paginaWeb) &&
    registrarRuta(servidorWeb, "/estado", HTTP_GET, estadoWeb) &&
    registrarRuta(servidorWeb, "/avanzar", HTTP_POST, avanzarWeb) &&
    registrarRuta(servidorWeb, "/configurar", HTTP_POST, configurarWeb) &&
    registrarRuta(servidorWeb, "/stop", HTTP_POST, pararWeb) &&
    registrarRuta(servidorWeb, "/captura", HTTP_GET, capturaWeb);
  httpd_config_t video = HTTPD_DEFAULT_CONFIG();
  video.server_port = 81;
  video.ctrl_port = web.ctrl_port + 1;
  video.stack_size = 8192;
  video.send_wait_timeout = 2;
  bien = bien && httpd_start(&servidorVideo, &video) == ESP_OK &&
         registrarRuta(servidorVideo, "/stream", HTTP_GET, videoWeb);
  if (!bien) {
    if (servidorVideo) httpd_stop(servidorVideo);
    httpd_stop(servidorWeb);
  }
  return bien;
}

bool iniciarCamara() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11; config.pin_d1 = 9; config.pin_d2 = 8; config.pin_d3 = 10;
  config.pin_d4 = 12; config.pin_d5 = 18; config.pin_d6 = 17; config.pin_d7 = 16;
  config.pin_xclk = 15; config.pin_pclk = 13; config.pin_vsync = 6; config.pin_href = 7;
  config.pin_sccb_sda = 4; config.pin_sccb_scl = 5;
  config.pin_pwdn = -1; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 16;
  const bool psram = psramFound();
  config.fb_count = psram ? 2 : 1;
  config.fb_location = psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = psram ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  Serial.printf("PSRAM=%s | buferes=%d\n", psram ? "SI" : "NO", config.fb_count);
  const esp_err_t resultado = esp_camera_init(&config);
  Serial.printf("CAMARA init=0x%X\n", (unsigned)resultado);
  return resultado == ESP_OK;
}

void procesarDevkit(const char *texto) {
  unsigned long recibida, activa, bloqueada;
  unsigned int moviendo, imu;
  float distancia; char causa[48];
  if (sscanf(texto, "A1 %lu %lu %lu %u %u %f %47s", &recibida, &activa, &bloqueada,
      &moviendo, &imu, &distancia, causa) != 7 || recibida != sesion ||
      moviendo > 1 || imu > 1 || !isfinite(distancia) || distancia < 0) return;
  for (const char *p = texto; *p; ++p) {
    if (*p == '"' || *p == '\\' || static_cast<unsigned char>(*p) < 32) return;
  }
  devkitRecibido = true; ultimoDevkit = millis(); devkitMoviendo = moviendo;
  devkitIMU = imu; distanciaDevkit = distancia; bloqueadaDevkit = bloqueada;
  snprintf(motivoDevkit, sizeof(motivoDevkit), "%s", causa);
  snprintf(mensajeDevkit, sizeof(mensajeDevkit), "%s", texto);
}

void leerUSB() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineaUSB[longitudUSB] = '\0';
      if (!descartarUSB) {
        if (strcmp(lineaUSB, "STOP") == 0) solicitudStop.store(true);
        else if (strcmp(lineaUSB, "ARRANCAR") == 0) {
          const uint32_t ahora = millis();
          xQueueSend(solicitudesInicio, &ahora, 0);
        } else Serial.println("Comandos: ARRANCAR | STOP. Mantener pagina con video abierta.");
      }
      longitudUSB = 0;
      descartarUSB = false;
    } else if (longitudUSB < sizeof(lineaUSB) - 1) lineaUSB[longitudUSB++] = c;
    else descartarUSB = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  sesion = esp_random();
  if (!sesion) sesion = 1;
  solicitudesInicio = xQueueCreate(1, sizeof(uint32_t));
  if (!solicitudesInicio) { Serial.println("ERROR memoria para cola de ordenes"); return; }
  enlace.setRxBufferSize(2048);
  enlace.begin(115200, SERIAL_8N1, 42, 41);
  enviarRobot(enlace, "STOP");
  puertoLidar.setRxBufferSize(8192);
  puertoLidar.begin(230400, SERIAL_8N1, 1, 2);
  camaraIniciada = iniciarCamara();
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("RobotCam", "CAMBIA_CLAVE_ROBOTCAM", 1, false, 1)) {
    Serial.println("ERROR WIFI");
    return;
  }
  servidoresListos = iniciarServidores();
  Serial.printf("AUTONOMO S3 V1.2 IZQUIERDA DERECHA | camara=%u web=%u | LiDAR RX1 TX2\n", camaraIniciada, servidoresListos);
  Serial.print("Pagina http://"); Serial.println(WiFi.softAPIP());
  Serial.println("Wi-Fi RobotCam | clave CAMBIA_CLAVE_ROBOTCAM | motores parados hasta nueva orden");
  const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
  puertoLidar.write(iniciar, sizeof(iniciar));
  ultimoArranqueLidar = millis();
}

void loop() {
  if (!solicitudesInicio) { delay(100); return; }
  for (size_t i = 0; i < 4096 && puertoLidar.available(); ++i) {
    lidar.recibir(puertoLidar.read(), millis());
  }
  for (size_t i = 0; i < 1024 && enlace.available(); ++i) {
    const char *texto = receptor.recibir(enlace.read());
    if (texto) procesarDevkit(texto);
  }
  leerUSB();
  const uint32_t ahora = millis();
  if (!lidar.vigente(ahora) && ahora - ultimoArranqueLidar > 4000) {
    const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
    puertoLidar.write(iniciar, sizeof(iniciar));
    ultimoArranqueLidar = ahora;
  }
  // Leer primero los sellos escritos por las tareas HTTP y despues el reloj;
  // evita una resta unsigned negativa si llega un frame durante esta iteracion.
  const uint32_t frame = ultimoFrame.load(), navegador = ultimoNavegador.load();
  const uint32_t ahoraSalud = millis();
  const bool video = camaraIniciada && videoActivo.load() && frame && ahoraSalud - frame < 1000;
  const bool operador = navegador && ahoraSalud - navegador < 1200;
  const bool sensor = lidar.vigente(ahora);
  const bool devkit = devkitRecibido && ahora - ultimoDevkit <= 600;
  const ConfigNavegacion config = configuracion();
  const bool salud = servidoresListos && operador && devkit && devkitIMU;
  const char *obstaculo = config.valida() ? navegacion.evaluar(lidar.distancias, config) : "Confirma frente y radio del chasis";
  const char *giro = config.valida() ? navegacion.evaluar(lidar.distancias, config, true) : obstaculo;
  const bool permiso = salud && sensor && config.valida() && (!obstaculo || !giro);
  const bool estabaActiva = navegacion.activa();
  if (solicitudStop.exchange(false)) {
    xQueueReset(solicitudesInicio);
    navegacion.parar(); snprintf(ultimaParada, sizeof(ultimaParada), "STOP o cambio de configuracion"); corrida = 0; enviarRobot(enlace, "STOP");
  } else {
    const char *causa = causaParada(servidoresListos, operador, devkit, devkitIMU,
                                    corrida && bloqueadaDevkit == corrida, motivoDevkit);
    if (navegacion.activa() && causa) {
      snprintf(ultimaParada, sizeof(ultimaParada), "%s", causa);
      navegacion.fallar(ultimaParada);
      xQueueReset(solicitudesInicio);
    }
    uint32_t solicitada;
    if (xQueueReceive(solicitudesInicio, &solicitada, 0) == pdPASS && !navegacion.activa()) {
      if (ahora - solicitada <= 500 && permiso) {
        if (++orden == 0) ++orden;
        corrida = orden; navegacion.arrancar(config, ahora);
      }
    }
    navegacion.actualizar(lidar.distancias, config, sensor, lidar.barridos, ahora);
    if (!navegacion.activa()) corrida = 0;
    if (estabaActiva && !navegacion.activa()) enviarRobot(enlace, "STOP");
  }
  if (navegacion.fase == Navegacion::ESPERANDO_ESPACIO || navegacion.fase == Navegacion::ESPERANDO_DATOS)
    snprintf(evento, sizeof(evento), "Espera automatica: %.70s", navegacion.motivo);
  else if (navegacion.activa()) snprintf(evento, sizeof(evento), "%s", navegacion.motivo);
  else if (navegacion.fase == Navegacion::FALLO) snprintf(evento, sizeof(evento), "%s. Pulsa ARRANCAR para reintentar.", navegacion.motivo);
  else snprintf(evento, sizeof(evento), "%s", !config.valida() ? "Confirma frente y radio del chasis" :
      !sensor ? "Esperando barridos validos del LiDAR" : !devkit ? "Esperando DevKit con firmware autonomo V1" :
      !devkitIMU ? "MPU sin datos" : !operador ? "Esperando navegador" :
      obstaculo && giro ? giro : navegacion.motivo);
  if (ahora - ultimoLatido >= 100) {
    ultimoLatido = ahora;
    char mensaje[140];
    snprintf(mensaje, sizeof(mensaje), "V1 %lu %lu %lu %d %d %d %d",
      (unsigned long)sesion, (unsigned long)++secuencia, (unsigned long)corrida,
      navegacion.izquierda, navegacion.izquierda, navegacion.derecha, navegacion.derecha);
    enviarRobot(enlace, mensaje);
  }
  portENTER_CRITICAL(&muxEstado);
  estado.activa = navegacion.activa();
  estado.horario = config.horario;
  memcpy(estado.ultimaParada, ultimaParada, sizeof(estado.ultimaParada));
  estado.configurada = config.valida(); estado.frente = config.frente; estado.radio = config.radio;
  estado.permiso = permiso;
  estado.camara = video;
  estado.lidar = sensor;
  estado.enlace = devkit;
  estado.imu = devkit && devkitIMU;
  estado.moviendo = devkit && devkitMoviendo;
  estado.operador = operador;
  estado.minimo = lidar.minimo;
  estado.sectores = lidar.cobertura;
  estado.paquetes = lidar.paquetes;
  estado.bytesLidar = lidar.bytesRecibidos;
  estado.erroresCabecera = lidar.erroresCabecera;
  estado.erroresChecksum = lidar.erroresChecksum;
  estado.iniciosLidar = lidar.inicios;
  memcpy(estado.diagnosticoLidar, lidar.diagnostico, sizeof(estado.diagnosticoLidar));
  estado.errores = lidar.errores;
  estado.barridos = lidar.barridos;
  estado.fps = fpsDecimas.load() / 10.0f;
  estado.distancia = distanciaDevkit;
  memcpy(estado.mapa, lidar.distancias, sizeof(estado.mapa));
  memcpy(estado.devkit, mensajeDevkit, sizeof(estado.devkit));
  memcpy(estado.evento, evento, sizeof(estado.evento));
  portEXIT_CRITICAL(&muxEstado);
  if (ahora - ultimoLog >= 2000) {
    ultimoLog = ahora;
    Serial.printf("ESTADO | cam=%u lidar=%u uart=%u imu=%u permiso=%u | min=%u mm | sectores=%u/72 | paquetes=%lu errores=%lu\n",
      video, sensor, devkit, devkitIMU, permiso, lidar.minimo, lidar.cobertura,
      (unsigned long)lidar.paquetes, (unsigned long)lidar.errores);
  }
  delay(1);
}
```

### `navegacion_autonoma/s3_integral/SupervisorMarcha.h`

```cpp
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
```

### `navegacion_autonoma/pruebas/casos_base_test.cpp`

```cpp
#include "../s3_integral/Navegacion.h"
#include "../s3_integral/SupervisorMarcha.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>

void llenarLado(uint16_t *mapa,const ConfigNavegacion &c,int lado,uint16_t d) {
  for(int i=0;i<36;++i) {
    const float a=Navegacion::relativo(i,c)*lado;
    if(a>=20 && a<=100) mapa[2*i]=mapa[2*i+1]=d;
  }
}
void frente(uint16_t *mapa,int grados,uint16_t d) {
  const int i=(grados/10)%36;
  mapa[2*i]=mapa[2*i+1]=d;
}
int main() {
  // El frente no siempre coincide con cero. Ambos montajes angulares.
  for(bool horario : {false,true}) for(int angulo : {0,15,90,180,270,307,359}) {
    ConfigNavegacion c{angulo,150,true,horario};
    for(int salida : {-1,1}) {
      uint16_t mapa[72];std::fill_n(mapa,72,2000);
      llenarLado(mapa,c,-salida,600); frente(mapa,angulo,400);
      Navegacion n;n.arrancar(c,0);n.actualizar(mapa,c,true,1,100);
      assert(n.fase==Navegacion::PAUSA_GIRO && n.sentido==salida);
      n.actualizar(mapa,c,true,2,400);
      assert(n.izquierda==-80*salida && n.derecha==80*salida);
      assert(n.fase==(salida>0?Navegacion::IZQUIERDA:Navegacion::DERECHA));
      // Corregir largo ya no obliga a ARRANCAR otra vez.
      for(unsigned k=3;k<150;++k) {
        n.actualizar(mapa,c,true,k,k*300);
        assert(n.activa());
      }
      std::fill_n(mapa,72,2000);
      // Si esta en PAUSA puede necesitar 250ms antes de los tres barridos libres.
      for(unsigned k=150;k<157;++k)n.actualizar(mapa,c,true,k,k*300);
      assert(n.fase==Navegacion::RECTO && n.izquierda==80 && n.derecha==80);
    }
  }
  ConfigNavegacion c{0,150,true};uint16_t mapa[72];std::fill_n(mapa,72,2000);
  Navegacion n;n.arrancar(c,0);n.actualizar(mapa,c,true,1,100);
  // Sin barrido valido: nunca conservar ultimo PWM. Recuperacion con datos NUEVOS.
  n.actualizar(mapa,c,false,1,700);assert(n.activa() && !n.izquierda && !n.derecha);
  for(unsigned t=800;t<1200;t+=50)n.actualizar(mapa,c,true,1,t);
  assert(!n.izquierda); // no reciclar el ultimo barrido.
  n.actualizar(mapa,c,true,2,1200);n.actualizar(mapa,c,true,3,1300);
  assert(!n.izquierda);n.actualizar(mapa,c,true,4,1400);assert(n.izquierda==80);
  // Reinicio del contador de barridos tras reinicio del sensor.
  n.actualizar(mapa,c,false,1000,1500);
  for(unsigned k=1;k<=3;++k)n.actualizar(mapa,c,true,k,1600+k*100);
  assert(n.izquierda==80);
  // Desconocidos persistentes no son via libre ni obligan a pulsar ARRANCAR.
  std::fill_n(mapa,72,0);n.actualizar(mapa,c,true,5,2000);
  for(unsigned k=6;k<50;++k)n.actualizar(mapa,c,true,k,k*500);
  assert(n.activa() && !n.izquierda && !n.derecha);
  n.parar();std::fill_n(mapa,72,2000);
  for(unsigned k=50;k<60;++k)n.actualizar(mapa,c,true,k,k*500);
  assert(!n.activa() && !n.izquierda); // STOP no se recupera solo.
  n.arrancar(c,0);n.fallar("FALLO_REAL");
  for(unsigned k=1;k<5;++k)n.actualizar(mapa,c,true,k,k*100);
  assert(!n.activa());
  // Escena sintetica que cambia con el giro: obstaculo frontal se desplaza
  // en el marco del sensor. Ambos sentidos encuentran frente libre y continuan.
  for(int lado : {-1,1}) {
    std::fill_n(mapa,72,2000); llenarLado(mapa,c,-lado,600); frente(mapa,0,400);
    Navegacion robot;robot.arrancar(c,0);robot.actualizar(mapa,c,true,1,100);
    robot.actualizar(mapa,c,true,2,400);assert(robot.sentido==lado);
    for(unsigned k=3;k<=18;++k) {
      std::fill_n(mapa,72,2000);
      // Hasta superar el giro el obstaculo sigue por delante; luego queda detras.
      frente(mapa,k<9 ? (lado>0?int((k-3)*10):int(360-(k-3)*10))%360 : 180,400);
      robot.actualizar(mapa,c,true,k,400+(k-2)*100);
    }
    assert(robot.fase==Navegacion::RECTO && robot.izquierda==80 && robot.derecha==80);
  }
  // Supervisor: todas las combinaciones de salud de servidor, operador, UART,
  // MPU y corrida bloqueada. No permite continuar ante un fallo real.
  for(unsigned mask=0;mask<32;++mask) {
    bool servidor=mask&1,operador=mask&2,devkit=mask&4,imu=mask&8,bloqueada=mask&16;
    const char *causa=causaParada(servidor,operador,devkit,imu,bloqueada,"ENCODER_FR_SIN_PULSOS");
    assert((causa==nullptr)==(servidor&&operador&&devkit&&imu&&!bloqueada));
    if(servidor&&operador&&devkit&&bloqueada)assert(!strcmp(causa,"ENCODER_FR_SIN_PULSOS"));
  }
  std::cout<<"OK: 28 casos izquierda/derecha, 2 escenas variables, 32 combinaciones de salud, larga correccion, desconocidos, sensor recuperado, STOP y fallo real\n";
}
```

### `navegacion_autonoma/pruebas/dobles/Arduino.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <deque>
#define CONFIG_IDF_TARGET_ESP32 1
#define ARDUINO_ISR_ATTR
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define FALLING 2
#define SERIAL_8N1 0
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE = int;
inline void portENTER_CRITICAL(int*) {} inline void portEXIT_CRITICAL(int*) {}
inline void portENTER_CRITICAL_ISR(int*) {} inline void portEXIT_CRITICAL_ISR(int*) {}
inline uint32_t reloj = 0;
inline int pines[64]{}, potencia[64]{};
inline uint32_t millis(){return reloj;}
inline void delay(unsigned ms){reloj+=ms;}
inline void pinMode(int,int){}
inline void digitalWrite(int pin,int v){pines[pin]=v;}
inline void analogWrite(int pin,int v){potencia[pin]=v;}
inline void attachInterrupt(int,void(*)(),int){}
class HardwareSerial {
 public:
  explicit HardwareSerial(int=0){}
  std::deque<char> rx; std::string tx;
  void setRxBufferSize(int){} void begin(int,int=0,int=0,int=0){}
  int available(){return rx.size();} char read(){char c=rx.front();rx.pop_front();return c;}
  void print(char c){tx+=c;} void print(const char*s){tx+=s;}
  void println(const char*s){tx+=s;tx+='\n';}
  template<typename... T> void printf(const char* f,T...args){char b[512];snprintf(b,sizeof(b),f,args...);tx+=b;}
};
inline HardwareSerial Serial;
```

### `navegacion_autonoma/pruebas/dobles/Wire.h`

```cpp
#pragma once
#include <stdint.h>
struct WireDoble {
 void begin(int,int,int){} void setTimeOut(int){}
 void beginTransmission(uint8_t){} void write(uint8_t){}
 int endTransmission(bool=true){return 0;}
 int requestFrom(uint8_t,uint8_t n,bool){return n;}
 int available(){return 0;} uint8_t read(){return 0;}
};
inline WireDoble Wire;
```

### `navegacion_autonoma/pruebas/integracion_test.cpp`

```cpp
#include "../devkit_integral/devkit_integral.ino"
#include "../s3_integral/Navegacion.h"
#include <algorithm>
#include <cassert>
#include <iostream>

int main() {
  ConfigNavegacion c{0,150,true}; Navegacion n;
  uint16_t mapa[72]; std::fill_n(mapa,72,2000);
  unsigned seq=0, barrido=0;
  auto enviar=[&](unsigned corrida,int izq,int der) {
    char m[100];snprintf(m,sizeof(m),"V1 123 %u %u %d %d %d %d",++seq,corrida,izq,izq,der,der);
    HardwareSerial tx;enviarRobot(tx,m);
    for(char b:tx.tx)enlace.rx.push_back(b);
    // Simula pulsos, no prueba mecanica real del motor ni consumo/carga.
    for(unsigned i=0;i<4;++i)if(aplicado[i])++pulsos[i];
    direccionMPU=0x68;imuValida=true;ultimaLecturaIMU=reloj;loop();
  };
  auto tick=[&](bool sensor=true) {
    reloj+=100;n.actualizar(mapa,c,sensor,++barrido,reloj);
    enviar(n.activa()?1:0,n.izquierda,n.derecha);
  };
  reloj=100;enviar(0,0,0);n.arrancar(c,reloj);
  for(unsigned k=0;k<10;++k) { tick(); }
  assert(moviendo && guardia.corrida==1);
  // Bloquea izquierda con obstaculos lejanos; elige derecha.
  for(unsigned i=52;i<69;++i)mapa[i]=600;
  mapa[0]=mapa[1]=400;
  for(unsigned k=0;k<15;++k)tick();
  assert(n.fase==Navegacion::DERECHA && objetivo[0]==80 && objetivo[1]==80 && objetivo[2]==-80 && objetivo[3]==-80);
  assert(aplicado[0]==80 && aplicado[1]==80 && aplicado[2]==-80 && aplicado[3]==-80);
  assert(pines[13]==LOW && pines[14]==HIGH && pines[26]==HIGH && pines[27]==LOW);
  assert(pines[18]==HIGH && pines[19]==LOW && pines[4]==LOW && pines[15]==HIGH);
  assert(moviendo && guardia.corrida==1 && guardia.bloqueada==0);
  // Mas de 6,5 s: alterna sin perder identificador ni activar watchdog falso.
  for(unsigned k=0;k<80;++k) { tick(); }
  assert(n.activa() && guardia.corrida==1);
  tick(false);assert(!moviendo && guardia.corrida==1);
  std::fill_n(mapa,72,2000);
  for(unsigned k=0;k<10;++k)tick();
  assert(moviendo && n.fase==Navegacion::RECTO && guardia.corrida==1);
  for(int v:aplicado)assert(v==80);
  // Recuperar sensor no es un nuevo arranque. STOP si cancela la corrida.
  n.parar();tick();assert(!moviendo && guardia.corrida==0);
  for(unsigned k=0;k<10;++k) { tick(); }
  assert(!moviendo);
  std::cout<<"OK: planificador -> CRC/UART -> DevKit real con dobles: derecha, inversion, correccion larga, recuperacion y STOP en una misma corrida\n";
}
```

### `navegacion_autonoma/pruebas/interfaz_test.cjs`

```javascript
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const source=fs.readFileSync(path.join(__dirname,'../s3_integral/s3_integral.ino'),'utf8');
const js=source.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements={},events={},requests=[];
const ctx=new Proxy({},{get:()=>()=>{}});
for(const id of ['avanzar','radar','video','frente','radio','estado','configActual','diagnostico','devkit','sentido']) {
 elements[id]={disabled:false,textContent:'',value:'',checkValidity:()=>true,
  getContext:()=>ctx,addEventListener:(type,fn)=>events[type]=fn,
  getBoundingClientRect:()=>({left:0,top:0,width:420,height:420})};
}
elements.sentido.value="1";
let state={permiso:1,activa:0,configurada:1,frente:0,radio:150,camara:1,lidar:1,enlace:1,imu:1,
 operador:1,moviendo:0,fps:10,distancia:0,sectores:68,minimo:900,paquetes:12,errores:0,barridos:1,
 bytesLidar:1000,erroresCabecera:0,erroresChecksum:0,iniciosLidar:2,mapa:Array(72).fill(900),devkit:'A1',evento:'Listo'};
let disconnected=false;
const sandbox={document:{getElementById:id=>elements[id]},location:{hostname:'192.168.4.1'},
 setTimeout:()=>{},alert:()=>{},fetch:async(url,options)=>{
  requests.push({url,options});if(disconnected)throw Error('Offline');
  return {ok:true,json:async()=>state,text:async()=> 'OK'};
 }};
vm.createContext(sandbox);vm.runInContext(js,sandbox);
(async()=>{
 await sandbox.actualizar();assert.equal(elements.avanzar.disabled,false);
 state.camara=0;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,false);
 state.camara=1; // video no decide permiso, pero la pagina informa su estado.
 state.activa=1;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 await sandbox.mandar('stop');assert.equal(requests.at(-1).url,'/stop');
 state.activa=0;state.permiso=0;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 elements.frente.value='0';elements.radio.value='150';await sandbox.configurar();
 assert.equal(requests.at(-1).url,'/configurar?frente=0&radio=150&confirmar=1&sentido=1');
 elements.sentido.value='-1';await sandbox.configurar();
 assert.equal(requests.at(-1).url,'/configurar?frente=0&radio=150&confirmar=1&sentido=-1');
 const numeroAntes=requests.length;elements.sentido.value='';await sandbox.configurar();
 assert.equal(requests.length,numeroAntes); // no confirmar un sentido supuesto.
 events.click({clientX:310,clientY:210});assert.equal(elements.frente.value,90);
 disconnected=true;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 assert.match(elements.devkit.textContent,/sin telemetria actual/);
 console.log('OK: interfaz real, bloqueo de ARRANCAR, STOP, frente cero, seleccion de angulo y desconexion');
})().catch(e=>{console.error(e);process.exitCode=1});
```

### `navegacion_autonoma/pruebas/motores_test.cpp`

```cpp
#include "../devkit_integral/devkit_integral.ino"
#include <cassert>
#include <iostream>

void tiempo(uint32_t t) { reloj=t; imuValida=true; direccionMPU=0x68; ultimaLecturaIMU=t; }
void orden(const char *s) { procesarEnlace(s); aplicarObjetivo(millis()); }
int main() {
  tiempo(100); orden("V1 123 1 0 0 0 0 0");
  orden("V1 123 2 1 80 80 80 80");
  assert(moviendo && pines[STBY]==HIGH);
  assert(pines[13]==LOW && pines[14]==HIGH); // inversion FL conservada.
  assert(pines[26]==HIGH && pines[27]==LOW);
  for(const Motor&m:motores) assert(potencia[m.pwm]==80);
  tiempo(200); orden("V1 123 3 1 -80 -80 80 80");
  assert(!moviendo && pines[STBY]==LOW);
  tiempo(349); aplicarObjetivo(millis()); assert(!moviendo);
  tiempo(350); aplicarObjetivo(millis());
  assert(moviendo && pines[13]==HIGH && pines[14]==LOW && pines[26]==LOW && pines[27]==HIGH);
  assert(pines[18]==LOW && pines[19]==HIGH && pines[4]==HIGH && pines[15]==LOW);
  tiempo(400); orden("V1 123 4 1 0 0 0 0"); assert(!moviendo);
  tiempo(500); orden("V1 123 5 1 80 80 80 80"); assert(!moviendo);
  tiempo(650); aplicarObjetivo(millis()); assert(moviendo);
  detener("STOP_USB"); orden("V1 123 6 1 80 80 80 80"); assert(!moviendo);
  tiempo(700); orden("V1 123 7 2 80 80 80 80"); assert(moviendo);
  // Una trama tardia ya en el bufer no puede reanimar la corrida caducada.
  tiempo(1200); char linea[80];
  snprintf(linea,sizeof(linea),"V1 123 8 2 80 80 80 80");
  HardwareSerial salida; enviarRobot(salida,linea);
  for(char ch:salida.tx) enlace.rx.push_back(ch);
  loop(); assert(!moviendo && guardia.bloqueada==2);
  tiempo(1300); orden("V1 123 9 3 80 80 80 80"); assert(moviendo);
  tiempo(2810); procesarEnlace("V1 123 10 3 80 80 80 80"); loop();
  assert(!moviendo && std::string(motivo)=="ENCODER_FL_SIN_PULSOS");
  procesarEnlace("STOP"); assert(std::string(motivo)=="ENCODER_FL_SIN_PULSOS");
  tiempo(3000); orden("V1 123 11 4 80 80 80 80"); assert(moviendo);
  imuValida=false; procesarEnlace("V1 123 12 4 80 80 80 80"); assert(!moviendo);
  // Detectar cada encoder por nombre. Los otros tres tienen pulsos actuales.
  unsigned seq=12;
  for(unsigned i=0;i<4;++i) {
    tiempo(4000+i*3000);
    char m[100];snprintf(m,sizeof(m),"V1 123 %u %u 80 80 80 80",++seq,5+i);
    orden(m);if(pausaInversion){tiempo(reloj+150);aplicarObjetivo(millis());}
    assert(moviendo);
    for(unsigned j=0;j<4;++j){anteriores[j]=pulsos[j];ultimoPulso[j]=reloj;}
    ultimoPulso[i]=reloj-1600;
    loop();assert(!moviendo && std::string(motivo)==FALLO_ENCODER[i]);
  }
  std::cout<<"OK: salidas de cuatro motores, inversion, pausa no bloqueante, STOP, UART tardia, encoders y MPU\n";
}
```

### `navegacion_autonoma/pruebas/navegacion_test.cpp`

```cpp
#include "../s3_integral/Navegacion.h"
#include "../devkit_integral/GuardiaMarcha.h"
#include <cassert>
#include <algorithm>
#include <iostream>

int main() {
  uint16_t mapa[72]; std::fill_n(mapa,72,2000);
  ConfigNavegacion c{0,150,true}; Navegacion n;
  assert(!n.arrancar({0,150,false},0));
  assert(!n.arrancar({360,150,true},0));
  assert(n.arrancar(c,0));
  for (uint32_t i=1;i<2000;++i) {
    n.actualizar(mapa,c,true,i,i*100);
    assert(n.activa() && n.izquierda==80 && n.derecha==80);
  } // mas de 3 minutos: sin limite de tiempo o centimetros.
  mapa[0]=400;
  n.actualizar(mapa,c,true,2000,200000);
  assert(n.fase==Navegacion::PAUSA_GIRO && !n.izquierda && !n.derecha);
  n.actualizar(mapa,c,true,2001,200200); assert(!n.izquierda);
  n.actualizar(mapa,c,true,2002,200300);
  assert(n.fase==Navegacion::IZQUIERDA && n.izquierda==-80 && n.derecha==80);
  mapa[0]=2000;
  n.actualizar(mapa,c,true,2003,200400);
  n.actualizar(mapa,c,true,2003,200500); // mismo barrido no vale como confirmacion nueva.
  assert(n.fase==Navegacion::IZQUIERDA);
  n.actualizar(mapa,c,true,2004,200600);
  n.actualizar(mapa,c,true,2005,200700);
  assert(n.fase==Navegacion::RECTO && n.izquierda==80 && n.derecha==80);
  n.actualizar(mapa,c,true,2006,200800); assert(n.izquierda==80);
  n.parar(); n.actualizar(mapa,c,true,2007,200900); assert(!n.activa() && !n.izquierda);
  n.arrancar(c,0); n.actualizar(mapa,c,false,1,1); assert(n.fase==Navegacion::ESPERANDO_DATOS && !n.izquierda);
  n.actualizar(mapa,c,true,2,100); assert(n.activa() && !n.izquierda); // requiere tres barridos nuevos.
  mapa[40]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // ecos cercanos nunca se ocultan.
  mapa[40]=2000; mapa[0]=mapa[1]=0; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // cero frontal no significa libre.
  mapa[0]=400; mapa[1]=2000; mapa[40]=mapa[41]=0; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // zona de giro sin observar.
  mapa[40]=mapa[41]=2000; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  n.actualizar(mapa,c,true,2,400); n.actualizar(mapa,c,true,3,6701);
  assert(n.activa() && n.fase==Navegacion::PAUSA_GIRO && n.sentido==-1); // cambia lado sin cancelar.
  mapa[0]=2000; mapa[18]=400; c.frente=90;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.fase==Navegacion::PAUSA_GIRO);
  // Falta trasera no impide avanzar, pero nunca se considera libre para pivotar.
  c.frente=0; mapa[18]=2000; mapa[40]=mapa[41]=0;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.izquierda==80);
  // Cuatro huecos aislados: cada celda conserva una medida real y el margen angular.
  std::fill_n(mapa,72,2000); mapa[0]=400;
  mapa[11]=mapa[21]=mapa[41]=mapa[51]=0;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.fase==Navegacion::PAUSA_GIRO);
  mapa[10]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda);
  // Recuperar espacio reanuda dentro de la misma corrida y sin nuevo ARRANCAR.
  std::fill_n(mapa,72,2000);
  n.actualizar(mapa,c,true,2,200); n.actualizar(mapa,c,true,2,210);
  assert(!n.izquierda);
  n.actualizar(mapa,c,true,3,300); assert(!n.izquierda);
  n.actualizar(mapa,c,true,4,400); assert(n.izquierda==80 && n.derecha==80);
  // Falta de zona durante el giro: espera y retoma giro con tres barridos.
  mapa[0]=400; n.actualizar(mapa,c,true,5,500);
  n.actualizar(mapa,c,true,6,800); assert(n.fase==Navegacion::IZQUIERDA);
  mapa[40]=mapa[41]=0; n.actualizar(mapa,c,true,7,900);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO);
  mapa[40]=mapa[41]=2000;
  n.actualizar(mapa,c,true,8,1000); n.actualizar(mapa,c,true,9,1100);
  n.actualizar(mapa,c,true,10,1200); assert(n.fase==Navegacion::PAUSA_GIRO);
  n.actualizar(mapa,c,true,11,1500); assert(n.izquierda==-80 && n.derecha==80);
  // STOP durante espera SIEMPRE cancela el rearme automatico.
  mapa[40]=54; n.actualizar(mapa,c,true,12,1600); n.parar();
  std::fill_n(mapa,72,2000);
  for(unsigned i=13;i<20;++i) n.actualizar(mapa,c,true,i,i*200);
  assert(!n.activa() && !n.izquierda && !n.derecha);
  // LiDAR invalido siempre pone velocidades a cero y conserva la solicitud.
  mapa[40]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  n.actualizar(mapa,c,false,2,200); assert(n.fase==Navegacion::ESPERANDO_DATOS && !n.izquierda);
  // Anticipacion reducida: 600 mm ya no bloquea el avance; 400 mm aun exige evasión.
  std::fill_n(mapa,72,2000); mapa[0]=600;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.izquierda==80 && n.derecha==80);
  mapa[0]=400; n.actualizar(mapa,c,true,2,200); assert(n.fase==Navegacion::PAUSA_GIRO);
  GuardiaMarcha g;
  assert(!g.aceptar(123,1,1,0)); // nueva sesion primero en reposo.
  assert(g.aceptar(123,1,0,0)); assert(g.aceptar(123,2,1,100));
  assert(g.vigente(550) && !g.vigente(551));
  g.bloquear(); assert(!g.aceptar(123,3,1,600));
  assert(g.aceptar(123,4,0,700)); assert(!g.aceptar(123,5,1,800));
  assert(g.aceptar(123,6,2,900)); assert(!g.aceptar(123,6,2,1000));
  assert(!g.aceptar(124,1,3,1100)); assert(g.aceptar(124,1,0,1100));
  assert(g.aceptar(124,2,1,1200));
  std::cout << "OK: marcha continua, giro, salida estable, STOP, geometria, ceros, ecos cercanos, caducidad y rearme\n";
}
```

### `navegacion_autonoma/pruebas/protocolo_test.cpp`

```cpp
#include "../s3_integral/LidarD6.h"
#include "../s3_integral/EnlaceRobot.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

std::vector<uint8_t> paquete(bool inicio, unsigned n, float a, float b, unsigned mm) {
  std::vector<uint8_t> p(10 + 3*n);
  auto poner=[&](unsigned i,unsigned v){p[i]=v&255;p[i+1]=v>>8;};
  p[0]=0xAA;p[1]=0x55;p[2]=inicio?21:20;p[3]=n;
  poner(4,unsigned(a*128)+1);poner(6,unsigned(b*128)+1);
  for(unsigned i=0;i<n;++i){p[10+3*i]=0;p[11+3*i]=(mm&63)<<2;p[12+3*i]=mm>>6;}
  unsigned cs=0;
  // SDK: cabecera 16 bits; cada muestra usa 8 bits y luego 16 bits LE.
  for(unsigned i=0;i<8;i+=2)cs^=p[i]|(p[i+1]<<8);
  for(unsigned i=10;i<p.size();i+=3)cs^=p[i]^(p[i+1]|(p[i+2]<<8));
  poner(8,cs);return p;
}
void dar(LidarD6 &l,const std::vector<uint8_t>&p,unsigned t){for(auto b:p)l.recibir(b,t);}
int main(){
  // Vector fijo independiente del generador: XOR SDK=0x5AFC, distancia=946 mm.
  // El agrupamiento anterior produce 0x9F39 y debe rechazar la trama.
  const std::vector<uint8_t> vectorSDK = {
    0xAA,0x55,0x15,0x01,0x01,0x00,0x01,0x00,0xFC,0x5A,0x88,0xCB,0x0E
  };
  LidarD6 referencia;
  dar(referencia,vectorSDK,0);
  dar(referencia,vectorSDK,100);
  assert(referencia.paquetes==2 && referencia.inicios==2 && referencia.errores==0);
  assert(referencia.distancias[0]==946);
  auto checksumAnterior=vectorSDK;
  checksumAnterior[8]=0x39;checksumAnterior[9]=0x9F;
  dar(referencia,checksumAnterior,120);
  assert(referencia.paquetes==2 && referencia.erroresChecksum==1);
  assert(crcRobot("123456789",9)==0x29B1);
  ReceptorRobot r;
  auto recibir=[&](std::string s){std::string salida;for(char c:s){auto p=r.recibir(c);if(p)salida=p;}return salida;};
  assert(recibir("ruido@123456789*29B1\n")=="123456789");
  assert(recibir("@123456788*29B1\n").empty());
  assert(recibir("@incompleto@123456789*29B1\r\n")=="123456789");
  assert(recibir("@"+std::string(500,'x')+"\n").empty());
  assert(recibir("@123456789*29B1\n")=="123456789");
  LidarD6 l;
  dar(l,paquete(true,1,0,0,800),0);
  dar(l,paquete(false,144,0,357.5,800),20);
  dar(l,paquete(true,1,0,0,800),100);
  assert(l.completo&&l.cobertura==72&&l.minimo==800&&l.vigente(600)&&!l.vigente(601));
  auto malo=paquete(false,144,0,357.5,100);malo[10]^=4;
  dar(l,malo,120);dar(l,paquete(true,1,0,0,800),200);
  assert(!l.completo&&l.errores>0);
  dar(l,paquete(false,144,0,357.5,900),220);dar(l,paquete(true,1,0,0,900),300);
  assert(l.completo&&l.minimo==800); // muestra de inicio del barrido anterior.
  dar(l,paquete(false,144,0,357.5,0),320);dar(l,paquete(true,1,0,0,900),400);
  assert(!l.completo); // ceros no prueban que el entorno esta libre.
  LidarD6 wrap;
  dar(wrap,paquete(true,1,0,0,1000),0);
  dar(wrap,paquete(false,5,350,10,200),10);
  dar(wrap,paquete(true,1,0,0,1000),100);
  assert(!wrap.completo&&wrap.distancias[70]==200&&wrap.distancias[0]==200&&wrap.distancias[2]==200);
  std::cout<<"OK: CRC, tramas corruptas, resincronizacion, barridos, caducidad, ceros y cruce de 360 grados\n";
}
```

### `navegacion_autonoma/pruebas/resultado_pruebas.txt`

```text
OK: CRC, tramas corruptas, resincronizacion, barridos, caducidad, ceros y cruce de 360 grados
OK: marcha continua, giro, salida estable, STOP, geometria, ceros, ecos cercanos, caducidad y rearme
OK: salidas de cuatro motores, inversion, pausa no bloqueante, STOP, UART tardia, encoders y MPU
OK: 28 casos izquierda/derecha, 2 escenas variables, 32 combinaciones de salud, larga correccion, desconocidos, sensor recuperado, STOP y fallo real
OK: planificador -> CRC/UART -> DevKit real con dobles: derecha, inversion, correccion larga, recuperacion y STOP en una misma corrida
OK: interfaz real, bloqueo de ARRANCAR, STOP, frente cero, seleccion de angulo y desconexion
```

### `navegacion_autonoma/pruebas/verificar.py`

```python
"""Pruebas del firmware en PC. No abre puertos del robot ni envia movimiento."""
from pathlib import Path
import subprocess
import sys


def main():
    carpeta = Path(__file__).resolve().parent
    if sys.platform == 'win32':
        linux = '/mnt/' + carpeta.drive[0].lower() + '/' + carpeta.as_posix()[3:]
        prefijo = ['wsl', '-e', 'sh', '-c']
    else:
        linux = str(carpeta)
        prefijo = ['sh', '-c']
    script = '''set -e
cd "$1"
for t in protocolo navegacion motores casos_base integracion; do
  g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I dobles "${t}_test.cpp" -o "/tmp/robot_${t}_test"
  "/tmp/robot_${t}_test"
done
'''
    reporte = []
    for comando in (prefijo + [script, 'sh', linux],
                    ['node', str(carpeta / 'interfaz_test.cjs')]):
        r = subprocess.run(comando, capture_output=True, text=True)
        print(r.stdout, end='')
        print(r.stderr, end='', file=sys.stderr)
        reporte.append(r.stdout + r.stderr)
        if r.returncode:
            (carpeta / 'resultado_pruebas.txt').write_text(''.join(reporte), encoding='utf-8')
            return r.returncode
    (carpeta / 'resultado_pruebas.txt').write_text(''.join(reporte), encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
```

## B. Versiones y respaldos del proyecto

### `prueba_autonoma_exitosa_2026-11-24/devkit_integral/devkit_integral.ino`

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "EnlaceRobot.h"
#include "GuardiaMarcha.h"

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
uint32_t ultimoReporte = 0;
uint32_t ultimaLecturaIMU = 0, ultimoIntentoIMU = 0;
bool moviendo = false;
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
  for (int valor : v) if (valor < -80 || valor > 80) return;
  if (!corrida) for (int valor : v) if (valor) return;
  const uint32_t anterior = guardia.corrida;
  if (!guardia.aceptar(sesion, secuencia, corrida, millis())) return;
  if (!corrida) { for (int &valor : objetivo) valor = 0; apagarSalidas(); return; }
  if (!imuValida || millis() - ultimaLecturaIMU > 250) { detener("MPU_SIN_DATOS"); return; }
  if (corrida != anterior) {
    copiarPulsos(iniciales);
    copiarPulsos(anteriores);
    for (uint32_t &t : ultimoPulso) t = millis();
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
  Serial.printf("AUTONOMO DEVKIT V1 | MPU=%s | motores deshabilitados\n", mpu ? "SI" : "NO");
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
    if (!aplicado[i] || cuentas[i] != anteriores[i]) ultimoPulso[i] = ahora;
    anteriores[i] = cuentas[i];
    if (aplicado[i] && ahora - ultimoPulso[i] > LIMITE_ENCODER_MS) {
      detener("ENCODER_SIN_PULSOS"); break;
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
  delay(1);
}
```

### `prueba_autonoma_exitosa_2026-11-24/devkit_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `prueba_autonoma_exitosa_2026-11-24/devkit_integral/GuardiaMarcha.h`

```cpp
#pragma once
#include <stdint.h>

// Cada ARRANCAR crea una corrida nueva. Un fallo/STOP invalida esa corrida:
// recibir otra vez la misma orden no vuelve a poner los motores en marcha.
class GuardiaMarcha {
 public:
  uint32_t sesion = 0, secuencia = 0, corrida = 0, ultimaCorrida = 0, bloqueada = 0, ultimo = 0;
  bool recibido = false;
  void bloquear() { if (corrida) bloqueada = corrida; corrida = 0; }
  bool vigente(uint32_t ahora) const { return recibido && ahora - ultimo <= 450; }
  bool aceptar(uint32_t s, uint32_t q, uint32_t r, uint32_t ahora) {
    if (!s) return false;
    if (s != sesion) {
      if (r != 0) return false; // reinicio requiere primero sincronizacion en reposo.
      sesion = s; secuencia = 0; corrida = ultimaCorrida = bloqueada = 0; recibido = false;
    }
    if (static_cast<int32_t>(q - secuencia) <= 0) return false;
    if (r && (r == bloqueada || static_cast<int32_t>(r - ultimaCorrida) < 0)) return false;
    secuencia = q; ultimo = ahora; recibido = true;
    if (!r) bloquear();
    else { corrida = r; ultimaCorrida = r; }
    return true;
  }
};
```

### `prueba_autonoma_exitosa_2026-11-24/LEEME.md`

```markdown
# Navegación continua — versión experimental V1.1

Preparada el 14 de noviembre de 2026. Conserva el cableado y el lector COIN-D6 V3 que funcionó. El respaldo anterior está en `primera_prueba_exitosa_2026-11-14`, con manifiesto SHA256. Los originales en `prueba_integral_50cm` también permanecen sin cambios.

## Qué hace

- **ARRANCAR:** inicia una corrida sin meta de distancia y sin límite de duración del avance.
- **PARAR / STOP:** cancela la corrida y deshabilita los motores.
- El S3 evalúa el LiDAR y decide avance o giro. El DevKit controla por separado las cuatro salidas, con signo: positivo avanza, negativo retrocede, cero deshabilita.
- Cuando un obstáculo entra en la trayectoria, hace una pausa de al menos 250 ms y pivota a la izquierda: FL y RL hacia atrás, FR y RR hacia adelante. No retrocede el vehículo completo.
- Al observar una trayectoria libre durante tres barridos nuevos consecutivos, termina el giro y envía avance en ese mismo ciclo, sin añadir una orden cero entre giro y avance. El cambio de sentido incluye una pausa de potencia de al menos 150 ms.
- No gira una cantidad fija de grados: gira hasta encontrar una trayectoria libre. No hay un botón de vuelta de 360 grados ni navegación hacia un destino.
- Si falta espacio o una celda necesaria para girar, espera con los motores parados pero conserva la corrida activa. Reevalúa cada barrido y reanuda avance o giro tras tres barridos nuevos que confirmen la misma salida. STOP cancela esa espera. Los fallos reales de sensor/comunicación/MPU/encoder y el giro sin salida siguen cancelando la corrida y requieren otro ARRANCAR; nunca se rearma por recuperar Wi-Fi o por repetir una orden vieja.

## Archivos para Arduino IDE: cargar AMBAS placas

1. DevKitC: abrir `devkit_integral/devkit_integral.ino`. Sus pestañas incluyen `EnlaceRobot.h` y `GuardiaMarcha.h`.
2. ESP32-S3: abrir `s3_integral/s3_integral.ino`. Sus pestañas incluyen `EnlaceRobot.h`, `EstadoIntegral.h`, `LidarD6.h` y `Navegacion.h`.

Abrir los archivos desde ESTA carpeta. No pegar fragmentos dentro de los programas anteriores ni juntar los dos `.ino` en un sketch. Los encabezados se compilan automáticamente.

**Hay que actualizar los dos ESP32.** El protocolo nuevo `V1/A1` es incompatible con el de la prueba de 50 cm. Si uno conserva el firmware antiguo, la página mostrará que falta el DevKit y no permitirá arrancar. No se cargó firmware automáticamente y no se añadió OTA.

Opciones verificadas con el paquete Arduino-ESP32 3.3.12 instalado:

| Placa física | Selección en Arduino IDE | Opciones |
|---|---|---|
| DevKitC WROOM-32D | ESP32 Dev Module | Flash Mode DIO |
| S3 | ESP32S3 Dev Module | Flash 16MB; OPI PSRAM; APP 3MB/FATFS 9MB; USB CDC On Boot Disabled con el USB-UART usado anteriormente |

Puerto: seleccionar el de cada placa; los números COM pueden cambiar. Mantener el aislamiento de alimentación externa/USB ya adoptado y no alimentar una placa apagada por su enlace UART. Programar con los motores sin alimentación.

## Cableado conservado

| Conexión | Destino |
|---|---|
| D6 amarillo TX | S3 GPIO1 RX, UART 230400 8N1 |
| D6 verde RX | S3 GPIO2 TX |
| D6 rojo / negro | 5V regulados / GND común |
| S3 TX41 | DevKit RX16, UART 115200 8N1 |
| DevKit TX17 | S3 RX42 |
| GND | Común entre las placas, sensor y alimentación |

No cambiar pines, regulador ni las inversiones de los motores. XL4016 continúa a 5V y se conserva la lógica a 3V3 del montaje probado. Los GPIO no reciben señales de 5V.

| Rueda | IN1 | IN2 | PWM | Encoder | Invertir |
|---|---:|---:|---:|---:|---|
| FL, delantera izquierda | 13 | 14 | 25 | 34 | Sí |
| RL, trasera izquierda | 26 | 27 | 32 | 35 | No |
| FR, delantera derecha | 18 | 19 | 23 | 36 | Sí |
| RR, trasera derecha | 4 | 15 | 33 | 39 | No |

STBY DevKit GPIO2; MPU SDA21/SCL22. Cámara y demás conexiones iguales a la prueba integral.

## Arranque desde Wi-Fi

1. Cargar ambos sketches y reiniciar las placas con su alimentación de funcionamiento. Ningún motor arranca por encender.
2. Conectar a **RobotCam**, contraseña **CAMBIA_CLAVE_ROBOTCAM**. Abrir **http://192.168.4.1/**. Mantener página y video abiertos; el AP admite un cliente.
3. Configurar la geometría estando detenido. El radio inicial del formulario es **150 mm**, según tu confirmación de que el LiDAR está centrado y hay 15 cm hasta cada punta del chasis. Debe cubrir también cualquier pieza que sobresalga.
4. Identificar el frente: colocar un objeto reconocible delante del carro, a la altura del plano del LiDAR, y observar qué puntos aparecen o se desplazan en el radar. Pulsar ese punto para rellenar su ángulo, o escribir el ángulo frontal. No asumir que 0 grados es el frente. Pulsar **Confirmar frente y radio** y retirar el objeto. Una línea amarilla indica el frente elegido.
5. Esa configuración se conserva en RAM hasta reiniciar el S3. Tras un reinicio vuelve a requerir confirmación; no hay arranque automático.
6. Confirmar Cámara, LiDAR, UART y MPU OK. Debajo se indica el motivo exacto si ARRANCAR sigue bloqueado.
7. En la primera validación del firmware nuevo, comprobar el sentido de las cuatro ruedas con el chasis elevado y los sentidos de avance/giro antes de probar obstáculos en el suelo. No se repiten las mediciones de voltaje ya realizadas.
8. En suelo despejado y a baja velocidad, pulsar ARRANCAR. Para la primera prueba de evasión usar un obstáculo ancho que corte el plano del LiDAR y dejar espacio alrededor del vehículo. Mantener acceso al interruptor físico y al botón PARAR / STOP.

## Cómo decide y por qué puede detenerse

La geometría se evalúa en `Navegacion.h`. Usa un círculo que cubre el chasis, con **100 mm de margen inicial**, y una trayectoria de anticipación de **150 mm** hacia adelante. Esos 150 mm son anticipación para esquivar: NO son un límite de recorrido. Se añade margen angular por el ancho de las celdas de planificación de 10 grados.

PWM de avance y giro: **80/255**, conservado de la prueba de motores. No es velocidad en cm/s ni una regulación PID. El margen de frenado y el giro aún deben validarse físicamente con el peso y el suelo reales. El recorrido de ruedas mostrado incluye giros; no representa desplazamiento recto ni posición del robot.

- Un eco a menos de radio + margen (**250 mm con radio de 150**) impide avanzar y pivotar. No se borran esos puntos ni se ignoran como si fueran cables. Las capturas anteriores mostraban 54–71 mm: si persisten, despejar el plano del sensor y comprobar qué está detectando.
- Los ceros significan **sin medida**, nunca distancia infinita. Para decidir, los 72 sectores se agrupan en 36 celdas de 10 grados, usando la menor distancia real de cada par y ampliando el margen angular. Un hueco aislado de 5 grados no elimina la medida vecina; una celda sin ninguna medida permanece desconocida. Avanzar requiere observaciones en las celdas delanteras; pivotar exige las **36 celdas observadas en el barrido actual** y despejadas alrededor del círculo. Con 67/72 o 68/72 sectores originales puede haber observación de las 36 celdas. Si faltan celdas para girar, espera automáticamente, lo explica y reevalúa sin cancelar la corrida. No se interpolan distancias ni se ocultan ecos cercanos. Esta discretización no certifica ausencia de objetos entre rayos.
- Barridos inválidos o de más de 500 ms cancelan la corrida. El lector conserva los requisitos de integridad del protocolo y de cobertura de la versión que funcionó.
- Un giro sin salida en 6,5 segundos termina en fallo, en lugar de dar vueltas indefinidamente.
- Cerrar la página, perder video o perder telemetría del DevKit cancela la corrida.
- El DevKit tiene su propio plazo de 450 ms sin órdenes válidas, además de parada por MPU sin datos y por una rueda activada sin pulsos durante 1,5 segundos.
- Ante un fallo real se invalida la corrida actual; una espera geométrica por espacio o celdas desconocidas no es un fallo enclavado. Una trama atrasada no la vuelve a activar. La causa local se conserva en la telemetría.

El comportamiento solicitado se cumple en condiciones normales: marcha continua, evasión y botón para detener. Las paradas de protección se conservan cuando continuar no tiene una maniobra válida.

Es evasión reactiva experimental, con preferencia fija por girar a la izquierda. No hay mapa, planificación global, rectitud garantizada, detección de huecos/escaleras ni garantía de evitar todo objeto: un LiDAR de un solo plano puede no ver objetos bajos o fuera de ese plano. No es todavía Nav2 ni navegación hacia objetos.

## Verificación del software

- Compilación de ambos sketches para las placas indicadas.
- `pruebas/protocolo_test.cpp`: checksum D6 corregido, CRC UART, resincronización, datos corruptos, barridos y caducidad.
- `pruebas/navegacion_test.cpp`: avance durante más de tres minutos simulados, obstáculo, pausa, giro, tres barridos de salida, STOP, orientación, puntos cercanos y sectores desconocidos; secuencias, sesiones y rearme.
- `pruebas/motores_test.cpp`: ejecuta el sketch DevKit con dobles de GPIO/UART/I2C. Comprueba cuatro salidas y sus inversiones, pausa entre sentidos, STOP, trama tardía después del plazo, falta de pulsos y fallo MPU. Los dobles se usan solo en PC, nunca se cargan al robot.
- `pruebas/interfaz_test.cjs`: ejecuta el JavaScript real con una página simulada; comprueba habilitación de ARRANCAR, STOP, configuración del ángulo cero, selección del frente en radar y desconexión. Sintaxis comprobada con Node.

Estas verificaciones no sustituyen la prueba física de evasión. Estado: compilado y probado en software; pendiente de cargar y validar en el vehículo.


## Corrección V1.1 — 24 de noviembre de 2026

Respaldo previo: `respaldo_autonomia_antes_reanudacion_2026-11-24`, verificado con SHA256. La primera prueba exitosa del 14 de noviembre permanece intacta.

El usuario observa que se detiene al corregir. Se encontraron dos comportamientos del planificador que podían intervenir: orden cero adicional al salir del giro y fallos enclavados por falta temporal de espacio/observaciones. Se corrigieron, sin atribuirles todavía la causa física: /estado no era accesible desde el PC al revisar. Falta el motivo mostrado por la página cuando ocurre la parada real.

Solo cambió el firmware S3 y su planificador: volver a cargar `s3_integral.ino` junto con todos sus encabezados. El protocolo V1/A1 y el DevKit no cambian. Conservar firmware autónomo en el DevKit. No es necesario cargar de nuevo el DevKit para esta corrección.

Una espera automática mantiene `activa=1`, velocidades cero y el mismo identificador de corrida. Requiere tres barridos distintos que confirmen el mismo tipo de salida (avance o giro); no acepta datos repetidos como tres confirmaciones. La página muestra `Espera automatica:`. No se baja el umbral de distancia ni se ignoran puntos cercanos. Si la obstrucción no desaparece, sigue esperando; no fuerza movimiento. Las pausas de inversión de motores y los fallos reales se conservan.

Los 50 cm sugeridos al identificar el ángulo frontal con una caja son solo una distancia práctica de ejemplo, no una condición de calibración ni una conversión de centímetros a grados. El radio físico confirmado sigue siendo 150 mm; comprobar la línea de configuración aplicada, no solo los valores escritos en el formulario.

Pruebas añadidas: salida de giro con avance inmediato, espera y reanudación sin ARRANCAR nuevo, recuperación durante giro, STOP mientras espera y fallo LiDAR durante espera. Pendiente verificación física con esta versión.


### Ajuste final de anticipación solicitado: 150 mm

El usuario pidió reducir de 25 a 15 cm la anticipación. `ANTICIPACION_MM=150` desplaza hacia delante la envolvente del chasis para decidir evasión. No significa permitir un obstáculo a 150 mm del eje del sensor. Con radio150 y margen100, el límite global permanece250mm; el comienzo frontal de evasión es aproximadamente400mm desde el eje, más ensanchamiento angular de sectores. La distancia al borde delantero es distinta de la distancia al sensor. No se ha medido ni certificado el frenado real a esa distancia.

El giro usa las cuatro ruedas: FL/RL a -80 y FR/RR a +80. Al recuperar frente libre vuelve a +80 en las cuatro, dentro de la misma corrida. Continúa siendo pivote con pausas breves de inversión, no un arco continuo ni aprendizaje por refuerzo. La última captura mostraba pérdida de /estado y datos congelados, frente escrito307 y aplicado90; corregir el frente aplicado antes de probar. La interfaz nueva identifica V1.1 y150mm y retira telemetría vieja cuando pierde la consulta.
```

### `prueba_autonoma_exitosa_2026-11-24/PRUEBA_CONFIRMADA.md`

```markdown
# Prueba autónoma confirmada por el usuario — 24 de noviembre de 2026

Copia de los 15 archivos que estaban en `navegacion_autonoma` antes de introducir la versión V1.2. Cada archivo se comparó mediante SHA256; el manifiesto original está en SHA256.txt. Las pruebas y fuentes de esa copia no se modificaron.

El usuario califica esta prueba como un rotundo éxito para la aproximación y corrección ante objetos cercanos. También informa límites reales: la corrección solo iba a la izquierda, a veces se detenía y exigía ARRANCAR de nuevo, y la evasión en escenarios más amplios no satisfacía el comportamiento esperado. Conservar esta evidencia y no describir la versión como navegación completamente validada.

Para restaurar, cargar ambos sketches de ESTA carpeta con todos sus encabezados. La versión nueva se mantiene separada de este respaldo en `navegacion_autonoma`.
```

### `prueba_autonoma_exitosa_2026-11-24/pruebas/dobles/Arduino.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <deque>
#define CONFIG_IDF_TARGET_ESP32 1
#define ARDUINO_ISR_ATTR
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define FALLING 2
#define SERIAL_8N1 0
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE = int;
inline void portENTER_CRITICAL(int*) {} inline void portEXIT_CRITICAL(int*) {}
inline void portENTER_CRITICAL_ISR(int*) {} inline void portEXIT_CRITICAL_ISR(int*) {}
inline uint32_t reloj = 0;
inline int pines[64]{}, potencia[64]{};
inline uint32_t millis(){return reloj;}
inline void delay(unsigned ms){reloj+=ms;}
inline void pinMode(int,int){}
inline void digitalWrite(int pin,int v){pines[pin]=v;}
inline void analogWrite(int pin,int v){potencia[pin]=v;}
inline void attachInterrupt(int,void(*)(),int){}
class HardwareSerial {
 public:
  explicit HardwareSerial(int=0){}
  std::deque<char> rx; std::string tx;
  void setRxBufferSize(int){} void begin(int,int=0,int=0,int=0){}
  int available(){return rx.size();} char read(){char c=rx.front();rx.pop_front();return c;}
  void print(char c){tx+=c;} void print(const char*s){tx+=s;}
  void println(const char*s){tx+=s;tx+='\n';}
  template<typename... T> void printf(const char* f,T...args){char b[512];snprintf(b,sizeof(b),f,args...);tx+=b;}
};
inline HardwareSerial Serial;
```

### `prueba_autonoma_exitosa_2026-11-24/pruebas/dobles/Wire.h`

```cpp
#pragma once
#include <stdint.h>
struct WireDoble {
 void begin(int,int,int){} void setTimeOut(int){}
 void beginTransmission(uint8_t){} void write(uint8_t){}
 int endTransmission(bool=true){return 0;}
 int requestFrom(uint8_t,uint8_t n,bool){return n;}
 int available(){return 0;} uint8_t read(){return 0;}
};
inline WireDoble Wire;
```

### `prueba_autonoma_exitosa_2026-11-24/pruebas/interfaz_test.cjs`

```javascript
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const source=fs.readFileSync(path.join(__dirname,'../s3_integral/s3_integral.ino'),'utf8');
const js=source.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements={},events={},requests=[];
const ctx=new Proxy({},{get:()=>()=>{}});
for(const id of ['avanzar','radar','video','frente','radio','estado','configActual','diagnostico','devkit']) {
 elements[id]={disabled:false,textContent:'',value:'',checkValidity:()=>true,
  getContext:()=>ctx,addEventListener:(type,fn)=>events[type]=fn,
  getBoundingClientRect:()=>({left:0,top:0,width:420,height:420})};
}
let state={permiso:1,activa:0,configurada:1,frente:0,radio:150,camara:1,lidar:1,enlace:1,imu:1,
 operador:1,moviendo:0,fps:10,distancia:0,sectores:68,minimo:900,paquetes:12,errores:0,barridos:1,
 bytesLidar:1000,erroresCabecera:0,erroresChecksum:0,iniciosLidar:2,mapa:Array(72).fill(900),devkit:'A1',evento:'Listo'};
let disconnected=false;
const sandbox={document:{getElementById:id=>elements[id]},location:{hostname:'192.168.4.1'},
 setTimeout:()=>{},alert:()=>{},fetch:async(url,options)=>{
  requests.push({url,options});if(disconnected)throw Error('Offline');
  return {ok:true,json:async()=>state,text:async()=> 'OK'};
 }};
vm.createContext(sandbox);vm.runInContext(js,sandbox);
(async()=>{
 await sandbox.actualizar();assert.equal(elements.avanzar.disabled,false);
 state.activa=1;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 await sandbox.mandar('stop');assert.equal(requests.at(-1).url,'/stop');
 state.activa=0;state.permiso=0;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 elements.frente.value='0';elements.radio.value='150';await sandbox.configurar();
 assert.equal(requests.at(-1).url,'/configurar?frente=0&radio=150&confirmar=1');
 events.click({clientX:310,clientY:210});assert.equal(elements.frente.value,90);
 disconnected=true;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 console.log('OK: interfaz real, bloqueo de ARRANCAR, STOP, frente cero, seleccion de angulo y desconexion');
})().catch(e=>{console.error(e);process.exitCode=1});
```

### `prueba_autonoma_exitosa_2026-11-24/pruebas/motores_test.cpp`

```cpp
#include "../devkit_integral/devkit_integral.ino"
#include <cassert>
#include <iostream>

void tiempo(uint32_t t) { reloj=t; imuValida=true; direccionMPU=0x68; ultimaLecturaIMU=t; }
void orden(const char *s) { procesarEnlace(s); aplicarObjetivo(millis()); }
int main() {
  tiempo(100); orden("V1 123 1 0 0 0 0 0");
  orden("V1 123 2 1 80 80 80 80");
  assert(moviendo && pines[STBY]==HIGH);
  assert(pines[13]==LOW && pines[14]==HIGH); // inversion FL conservada.
  assert(pines[26]==HIGH && pines[27]==LOW);
  for(const Motor&m:motores) assert(potencia[m.pwm]==80);
  tiempo(200); orden("V1 123 3 1 -80 -80 80 80");
  assert(!moviendo && pines[STBY]==LOW);
  tiempo(349); aplicarObjetivo(millis()); assert(!moviendo);
  tiempo(350); aplicarObjetivo(millis());
  assert(moviendo && pines[13]==HIGH && pines[14]==LOW && pines[26]==LOW && pines[27]==HIGH);
  assert(pines[18]==LOW && pines[19]==HIGH && pines[4]==HIGH && pines[15]==LOW);
  tiempo(400); orden("V1 123 4 1 0 0 0 0"); assert(!moviendo);
  tiempo(500); orden("V1 123 5 1 80 80 80 80"); assert(!moviendo);
  tiempo(650); aplicarObjetivo(millis()); assert(moviendo);
  detener("STOP_USB"); orden("V1 123 6 1 80 80 80 80"); assert(!moviendo);
  tiempo(700); orden("V1 123 7 2 80 80 80 80"); assert(moviendo);
  // Una trama tardia ya en el bufer no puede reanimar la corrida caducada.
  tiempo(1200); char linea[80];
  snprintf(linea,sizeof(linea),"V1 123 8 2 80 80 80 80");
  HardwareSerial salida; enviarRobot(salida,linea);
  for(char ch:salida.tx) enlace.rx.push_back(ch);
  loop(); assert(!moviendo && guardia.bloqueada==2);
  tiempo(1300); orden("V1 123 9 3 80 80 80 80"); assert(moviendo);
  tiempo(2810); procesarEnlace("V1 123 10 3 80 80 80 80"); loop();
  assert(!moviendo && std::string(motivo)=="ENCODER_SIN_PULSOS");
  procesarEnlace("STOP"); assert(std::string(motivo)=="ENCODER_SIN_PULSOS");
  tiempo(3000); orden("V1 123 11 4 80 80 80 80"); assert(moviendo);
  imuValida=false; procesarEnlace("V1 123 12 4 80 80 80 80"); assert(!moviendo);
  std::cout<<"OK: salidas de cuatro motores, inversion, pausa no bloqueante, STOP, UART tardia, encoders y MPU\n";
}
```

### `prueba_autonoma_exitosa_2026-11-24/pruebas/navegacion_test.cpp`

```cpp
#include "../s3_integral/Navegacion.h"
#include "../devkit_integral/GuardiaMarcha.h"
#include <cassert>
#include <algorithm>
#include <iostream>

int main() {
  uint16_t mapa[72]; std::fill_n(mapa,72,2000);
  ConfigNavegacion c{0,150,true}; Navegacion n;
  assert(!n.arrancar({0,150,false},0));
  assert(!n.arrancar({360,150,true},0));
  assert(n.arrancar(c,0));
  for (uint32_t i=1;i<2000;++i) {
    n.actualizar(mapa,c,true,i,i*100);
    assert(n.activa() && n.izquierda==80 && n.derecha==80);
  } // mas de 3 minutos: sin limite de tiempo o centimetros.
  mapa[0]=400;
  n.actualizar(mapa,c,true,2000,200000);
  assert(n.fase==Navegacion::PAUSA_GIRO && !n.izquierda && !n.derecha);
  n.actualizar(mapa,c,true,2001,200200); assert(!n.izquierda);
  n.actualizar(mapa,c,true,2002,200300);
  assert(n.fase==Navegacion::IZQUIERDA && n.izquierda==-80 && n.derecha==80);
  mapa[0]=2000;
  n.actualizar(mapa,c,true,2003,200400);
  n.actualizar(mapa,c,true,2003,200500); // mismo barrido no vale como confirmacion nueva.
  assert(n.fase==Navegacion::IZQUIERDA);
  n.actualizar(mapa,c,true,2004,200600);
  n.actualizar(mapa,c,true,2005,200700);
  assert(n.fase==Navegacion::RECTO && n.izquierda==80 && n.derecha==80);
  n.actualizar(mapa,c,true,2006,200800); assert(n.izquierda==80);
  n.parar(); n.actualizar(mapa,c,true,2007,200900); assert(!n.activa() && !n.izquierda);
  n.arrancar(c,0); n.actualizar(mapa,c,false,1,1); assert(n.fase==Navegacion::FALLO);
  n.actualizar(mapa,c,true,2,100); assert(!n.activa()); // no rearme automatico.
  mapa[40]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // ecos cercanos nunca se ocultan.
  mapa[40]=2000; mapa[0]=mapa[1]=0; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // cero frontal no significa libre.
  mapa[0]=400; mapa[1]=2000; mapa[40]=mapa[41]=0; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda); // zona de giro sin observar.
  mapa[40]=mapa[41]=2000; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  n.actualizar(mapa,c,true,2,400); n.actualizar(mapa,c,true,3,6701);
  assert(!n.activa()); // atrapado, no girar indefinidamente.
  mapa[0]=2000; mapa[18]=400; c.frente=90;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.fase==Navegacion::PAUSA_GIRO);
  // Falta trasera no impide avanzar, pero nunca se considera libre para pivotar.
  c.frente=0; mapa[18]=2000; mapa[40]=mapa[41]=0;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.izquierda==80);
  // Cuatro huecos aislados: cada celda conserva una medida real y el margen angular.
  std::fill_n(mapa,72,2000); mapa[0]=400;
  mapa[11]=mapa[21]=mapa[41]=mapa[51]=0;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.fase==Navegacion::PAUSA_GIRO);
  mapa[10]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO && !n.izquierda);
  // Recuperar espacio reanuda dentro de la misma corrida y sin nuevo ARRANCAR.
  std::fill_n(mapa,72,2000);
  n.actualizar(mapa,c,true,2,200); n.actualizar(mapa,c,true,2,210);
  assert(!n.izquierda);
  n.actualizar(mapa,c,true,3,300); assert(!n.izquierda);
  n.actualizar(mapa,c,true,4,400); assert(n.izquierda==80 && n.derecha==80);
  // Falta de zona durante el giro: espera y retoma giro con tres barridos.
  mapa[0]=400; n.actualizar(mapa,c,true,5,500);
  n.actualizar(mapa,c,true,6,800); assert(n.fase==Navegacion::IZQUIERDA);
  mapa[40]=mapa[41]=0; n.actualizar(mapa,c,true,7,900);
  assert(n.fase==Navegacion::ESPERANDO_ESPACIO);
  mapa[40]=mapa[41]=2000;
  n.actualizar(mapa,c,true,8,1000); n.actualizar(mapa,c,true,9,1100);
  n.actualizar(mapa,c,true,10,1200); assert(n.fase==Navegacion::PAUSA_GIRO);
  n.actualizar(mapa,c,true,11,1500); assert(n.izquierda==-80 && n.derecha==80);
  // STOP durante espera SIEMPRE cancela el rearme automatico.
  mapa[40]=54; n.actualizar(mapa,c,true,12,1600); n.parar();
  std::fill_n(mapa,72,2000);
  for(unsigned i=13;i<20;++i) n.actualizar(mapa,c,true,i,i*200);
  assert(!n.activa() && !n.izquierda && !n.derecha);
  // Fallo de LiDAR durante espera sigue siendo un fallo real enclavado.
  mapa[40]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  n.actualizar(mapa,c,false,2,200); assert(n.fase==Navegacion::FALLO);
  // Anticipacion reducida: 600 mm ya no bloquea el avance; 400 mm aun exige evasión.
  std::fill_n(mapa,72,2000); mapa[0]=600;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.izquierda==80 && n.derecha==80);
  mapa[0]=400; n.actualizar(mapa,c,true,2,200); assert(n.fase==Navegacion::PAUSA_GIRO);
  GuardiaMarcha g;
  assert(!g.aceptar(123,1,1,0)); // nueva sesion primero en reposo.
  assert(g.aceptar(123,1,0,0)); assert(g.aceptar(123,2,1,100));
  assert(g.vigente(550) && !g.vigente(551));
  g.bloquear(); assert(!g.aceptar(123,3,1,600));
  assert(g.aceptar(123,4,0,700)); assert(!g.aceptar(123,5,1,800));
  assert(g.aceptar(123,6,2,900)); assert(!g.aceptar(123,6,2,1000));
  assert(!g.aceptar(124,1,3,1100)); assert(g.aceptar(124,1,0,1100));
  assert(g.aceptar(124,2,1,1200));
  std::cout << "OK: marcha continua, giro, salida estable, STOP, geometria, ceros, ecos cercanos, caducidad y rearme\n";
}
```

### `prueba_autonoma_exitosa_2026-11-24/pruebas/protocolo_test.cpp`

```cpp
#include "../s3_integral/LidarD6.h"
#include "../s3_integral/EnlaceRobot.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

std::vector<uint8_t> paquete(bool inicio, unsigned n, float a, float b, unsigned mm) {
  std::vector<uint8_t> p(10 + 3*n);
  auto poner=[&](unsigned i,unsigned v){p[i]=v&255;p[i+1]=v>>8;};
  p[0]=0xAA;p[1]=0x55;p[2]=inicio?21:20;p[3]=n;
  poner(4,unsigned(a*128)+1);poner(6,unsigned(b*128)+1);
  for(unsigned i=0;i<n;++i){p[10+3*i]=0;p[11+3*i]=(mm&63)<<2;p[12+3*i]=mm>>6;}
  unsigned cs=0;
  // SDK: cabecera 16 bits; cada muestra usa 8 bits y luego 16 bits LE.
  for(unsigned i=0;i<8;i+=2)cs^=p[i]|(p[i+1]<<8);
  for(unsigned i=10;i<p.size();i+=3)cs^=p[i]^(p[i+1]|(p[i+2]<<8));
  poner(8,cs);return p;
}
void dar(LidarD6 &l,const std::vector<uint8_t>&p,unsigned t){for(auto b:p)l.recibir(b,t);}
int main(){
  // Vector fijo independiente del generador: XOR SDK=0x5AFC, distancia=946 mm.
  // El agrupamiento anterior produce 0x9F39 y debe rechazar la trama.
  const std::vector<uint8_t> vectorSDK = {
    0xAA,0x55,0x15,0x01,0x01,0x00,0x01,0x00,0xFC,0x5A,0x88,0xCB,0x0E
  };
  LidarD6 referencia;
  dar(referencia,vectorSDK,0);
  dar(referencia,vectorSDK,100);
  assert(referencia.paquetes==2 && referencia.inicios==2 && referencia.errores==0);
  assert(referencia.distancias[0]==946);
  auto checksumAnterior=vectorSDK;
  checksumAnterior[8]=0x39;checksumAnterior[9]=0x9F;
  dar(referencia,checksumAnterior,120);
  assert(referencia.paquetes==2 && referencia.erroresChecksum==1);
  assert(crcRobot("123456789",9)==0x29B1);
  ReceptorRobot r;
  auto recibir=[&](std::string s){std::string salida;for(char c:s){auto p=r.recibir(c);if(p)salida=p;}return salida;};
  assert(recibir("ruido@123456789*29B1\n")=="123456789");
  assert(recibir("@123456788*29B1\n").empty());
  assert(recibir("@incompleto@123456789*29B1\r\n")=="123456789");
  assert(recibir("@"+std::string(500,'x')+"\n").empty());
  assert(recibir("@123456789*29B1\n")=="123456789");
  LidarD6 l;
  dar(l,paquete(true,1,0,0,800),0);
  dar(l,paquete(false,144,0,357.5,800),20);
  dar(l,paquete(true,1,0,0,800),100);
  assert(l.completo&&l.cobertura==72&&l.minimo==800&&l.vigente(600)&&!l.vigente(601));
  auto malo=paquete(false,144,0,357.5,100);malo[10]^=4;
  dar(l,malo,120);dar(l,paquete(true,1,0,0,800),200);
  assert(!l.completo&&l.errores>0);
  dar(l,paquete(false,144,0,357.5,900),220);dar(l,paquete(true,1,0,0,900),300);
  assert(l.completo&&l.minimo==800); // muestra de inicio del barrido anterior.
  dar(l,paquete(false,144,0,357.5,0),320);dar(l,paquete(true,1,0,0,900),400);
  assert(!l.completo); // ceros no prueban que el entorno esta libre.
  LidarD6 wrap;
  dar(wrap,paquete(true,1,0,0,1000),0);
  dar(wrap,paquete(false,5,350,10,200),10);
  dar(wrap,paquete(true,1,0,0,1000),100);
  assert(!wrap.completo&&wrap.distancias[70]==200&&wrap.distancias[0]==200&&wrap.distancias[2]==200);
  std::cout<<"OK: CRC, tramas corruptas, resincronizacion, barridos, caducidad, ceros y cruce de 360 grados\n";
}
```

### `prueba_autonoma_exitosa_2026-11-24/s3_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `prueba_autonoma_exitosa_2026-11-24/s3_integral/EstadoIntegral.h`

```cpp
#pragma once
#include <stdint.h>

struct EstadoIntegral {
  bool activa = false, configurada = false;
  int frente = 0, radio = 150;
  bool permiso = false;
  bool camara = false;
  bool lidar = false;
  bool enlace = false;
  bool imu = false;
  bool moviendo = false;
  bool operador = false;
  uint16_t minimo = 0;
  uint8_t sectores = 0;
  uint32_t paquetes = 0;
  uint32_t bytesLidar = 0;
  uint32_t erroresCabecera = 0;
  uint32_t erroresChecksum = 0;
  uint32_t iniciosLidar = 0;
  char diagnosticoLidar[180] = "Esperando bytes del LiDAR";
  uint32_t errores = 0;
  uint32_t barridos = 0;
  float fps = 0;
  float distancia = 0;
  uint16_t mapa[72]{};
  char devkit[300] = "Esperando DevKitC";
  char evento[96] = "Esperando video, LiDAR y DevKitC";
};
```

### `prueba_autonoma_exitosa_2026-11-24/s3_integral/LidarD6.h`

```cpp
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
        completo = !corrupto && cobertura >= 60 && puntos >= 100 &&
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
```

### `prueba_autonoma_exitosa_2026-11-24/s3_integral/Navegacion.h`

```cpp
#pragma once
#include <stdint.h>
#include <math.h>

// Planificador reactivo conservador, independiente del hardware. Nunca retrocede.
// El radio debe incluir TODO el chasis desde el eje del LiDAR.
struct ConfigNavegacion {
  int frente = 0;
  int radio = 0;
  bool confirmada = false;
  bool valida() const { return confirmada && frente >= 0 && frente < 360 && radio >= 100 && radio <= 1000; }
};

class Navegacion {
 public:
  static constexpr float ANTICIPACION_MM = 150.0f;
  enum Fase { PARADO, RECTO, PAUSA_GIRO, IZQUIERDA, ESPERANDO_ESPACIO, FALLO };
  Fase fase = PARADO;
  int izquierda = 0, derecha = 0;
  const char *motivo = "Pulsa ARRANCAR";
  bool activa() const { return fase == RECTO || fase == PAUSA_GIRO || fase == IZQUIERDA || fase == ESPERANDO_ESPACIO; }
  void parar(const char *causa = "Detenido por el operador") {
    fase = PARADO; izquierda = derecha = 0; motivo = causa;
  }
  void fallar(const char *causa) { fase = FALLO; izquierda = derecha = 0; motivo = causa; }
  void esperar(const char *causa) {
    fase = ESPERANDO_ESPACIO; izquierda = derecha = 0; motivo = causa; libres = 0; salida = 0;
  }
  bool arrancar(const ConfigNavegacion &c, uint32_t ahora) {
    if (!c.valida()) { fallar("Configura frente y radio del chasis"); return false; }
    fase = RECTO; izquierda = derecha = 0; inicioFase = ahora; ultimoBarrido = UINT32_MAX; libres = 0;
    motivo = "Evaluando barrido antes de mover"; return true;
  }
  // 0 significa sin medida, NUNCA espacio libre. No se filtran ecos cercanos.
  const char *evaluar(const uint16_t *mapa, const ConfigNavegacion &c, bool giro = false) const {
    const float radio = c.radio + 100.0f; // margen inicial de 10 cm, por validar fisicamente.
    // Planificar en 36 celdas de 10 grados: conservar el eco menor de cada par
    // de sectores de 5 grados. Una celda sin ninguna medida sigue desconocida.
    // No se interpolan distancias ni se elimina un eco cercano.
    for (int i = 0; i < 36; ++i) {
      const uint16_t a0 = mapa[2*i], a1 = mapa[2*i+1];
      const uint16_t medida = !a0 ? a1 : !a1 ? a0 : a0 < a1 ? a0 : a1;
      float a = (i * 10.0f + 5.0f - c.frente) * 0.01745329252f;
      float x = cosf(a), y = sinf(a);
      if (medida && medida <= radio) return "Obstaculo dentro del margen del chasis";
      if (giro && !medida) return "Giro bloqueado: falta observar una zona del LiDAR";
      if (giro) continue;
      if (x <= 0) continue;
      if (!medida) return "Falta medida en la zona de avance";
      // Capsula barrida: circulo del chasis desplazado 15 cm hacia adelante.
      // Agrandar por media apertura del sector para no tratar el centro como un rayo exacto.
      const float d = medida;
      const float ancho = radio + d * 0.08716f;
      const float px = d * x, py = fabsf(d * y);
      const float dx = px > ANTICIPACION_MM ? px - ANTICIPACION_MM : 0.0f;
      if (dx * dx + py * py <= ancho * ancho) return "Obstaculo en trayectoria: buscando giro";
    }
    return nullptr;
  }
  void actualizar(const uint16_t *mapa, const ConfigNavegacion &c, bool sensor,
                  uint32_t barrido, uint32_t ahora) {
    if (!activa()) return;
    if (!sensor) { fallar("LiDAR sin barrido valido reciente"); return; }
    if (!c.valida()) { fallar("Configuracion de geometria invalida"); return; }
    if ((fase == IZQUIERDA || fase == PAUSA_GIRO) && ahora - inicioFase > 6500) {
      fallar("No se encontro salida durante el giro"); return;
    }
    if (barrido == ultimoBarrido) return;
    ultimoBarrido = barrido;
    const char *frente = evaluar(mapa, c);
    const char *giro = evaluar(mapa, c, true);
    if (fase == ESPERANDO_ESPACIO) {
      // Una obstruccion con LiDAR sano no cancela la corrida. Tres barridos
      // nuevos confirman la salida; un STOP o fallo real siguen cancelandola.
      if (!frente) {
        if (salida != 1) libres = 0;
        salida = 1;
        if (++libres >= 3) {
          fase = RECTO; izquierda = derecha = 80;
          motivo = "Espacio libre: avance reanudado automaticamente";
        }
      } else if (!giro) {
        if (salida != 2) libres = 0;
        salida = 2;
        if (++libres >= 3) {
          fase = PAUSA_GIRO; inicioFase = ahora; libres = 0;
          motivo = "Espacio para giro recuperado: esquivando";
        }
      } else { libres = 0; salida = 0; motivo = giro; }
      return;
    }
    if (fase == RECTO) {
      if (!frente) { izquierda = derecha = 80; motivo = "Avanzando sin limite de distancia"; return; }
      izquierda = derecha = 0;
      if (giro) { esperar(giro); return; }
      fase = PAUSA_GIRO; inicioFase = ahora; libres = 0;
      motivo = "Pausa antes de girar a la izquierda"; return;
    }
    if (giro) { esperar(giro); return; }
    if (fase == PAUSA_GIRO) {
      if (ahora - inicioFase >= 250) { fase = IZQUIERDA; izquierda = -80; derecha = 80; motivo = "Girando a la izquierda para esquivar"; }
      return;
    }
    if (!frente) ++libres; else libres = 0;
    if (libres >= 3) {
      fase = RECTO; izquierda = derecha = 80; motivo = "Salida libre: retomando avance";
      return;
    }
    izquierda = -80; derecha = 80;
  }
 private:
  uint32_t inicioFase = 0, ultimoBarrido = UINT32_MAX;
  uint8_t libres = 0, salida = 0;
};
```

### `prueba_autonoma_exitosa_2026-11-24/s3_integral/s3_integral.ino`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "EnlaceRobot.h"
#include "LidarD6.h"
#include "EstadoIntegral.h"
#include "Navegacion.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Selecciona ESP32S3 Dev Module."
#endif

Navegacion navegacion;
std::atomic<uint32_t> geometria{0}; // RAM: vuelve a confirmar el frente tras reiniciar.
uint32_t corrida = 0, bloqueadaDevkit = 0;
ConfigNavegacion configuracion() {
  const uint32_t g = geometria.load();
  return {int(g & 511), int((g >> 9) & 2047), bool(g & 0x80000000u)};
}
HardwareSerial enlace(1);
HardwareSerial puertoLidar(2);
ReceptorRobot receptor;
LidarD6 lidar;
httpd_handle_t servidorWeb = nullptr, servidorVideo = nullptr;
QueueHandle_t solicitudesInicio = nullptr;
portMUX_TYPE muxEstado = portMUX_INITIALIZER_UNLOCKED;
EstadoIntegral estado;
std::atomic<bool> solicitudStop{false};
std::atomic<bool> videoActivo{false};
std::atomic<uint32_t> ultimoFrame{0}, ultimoNavegador{0};
std::atomic<uint32_t> fpsDecimas{0};
bool camaraIniciada = false, servidoresListos = false;
bool devkitIMU = false, devkitMoviendo = false, devkitRecibido = false;
float distanciaDevkit = 0;
uint32_t sesion = 0, secuencia = 0, orden = 0;
uint32_t ultimoDevkit = 0, ultimoLatido = 0, ultimoLog = 0;
uint32_t ultimoArranqueLidar = 0;
char mensajeDevkit[300] = "Esperando DevKitC";
char evento[96] = "Abre la pagina y espera los indicadores";
char lineaUSB[32]{};
size_t longitudUSB = 0;
bool descartarUSB = false;

const char PAGINA[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot 4WD · navegación continua</title>
<style>
body{font:16px Arial;background:#142b43;color:#fff;margin:20px auto;max-width:1100px;padding:0 16px}
h1{font-size:25px}.paneles{display:flex;gap:16px;flex-wrap:wrap}.panel{flex:1;min-width:280px}
img,canvas{width:100%;max-width:520px;background:#203c55;border-radius:12px}
button{font-size:19px;padding:14px 22px;border:0;border-radius:8px;margin:8px 10px 8px 0;cursor:pointer}
#avanzar{background:#77dec4}#parar{background:#ff6666;font-weight:bold}button:disabled{opacity:.4}
pre{white-space:pre-wrap;background:#203c55;padding:14px;border-radius:8px;overflow-wrap:anywhere}
</style></head><body>
<h1>Robot 4WD · navegación continua</h1>
<p>Version 1.1: reanudacion automatica | anticipacion de evasion: 150 mm mas el margen del chasis.</p>
<p>ARRANCAR inicia el avance sin límite de centímetros. Ante un obstáculo busca salida girando a la izquierda. Si falta espacio, espera y reanuda al recuperarlo. PARAR / STOP cancela la marcha. Los fallos de sensores, enlace o giro sin salida requieren un nuevo ARRANCAR.</p>
<div class="paneles"><div class="panel"><h2>Camara OV5640</h2>
<img id="video" alt="Video de la camara"><br>
<button onclick="reconectar()">Reconectar video</button>
<a style="color:#8deadb" href="/captura" target="_blank">Captura</a></div>
<div class="panel"><h2>LiDAR · vista de 72 sectores</h2>
<canvas id="radar" width="420" height="420"></canvas>
<p>0 grados es la referencia del LiDAR. El dibujo no supone el frente del carro. Escala 3 m.</p></div></div>
<p>Configuración antes de arrancar: coloca un objeto delante del carro y pulsa su punto en el radar para elegir el ángulo frontal. Retira el objeto antes de arrancar.</p>
<label>Frente del carro (grados): <input id="frente" type="number" min="0" max="359" step="1" placeholder="Selecciona en radar"></label>
<label>Radio hasta la esquina (mm): <input id="radio" type="number" min="100" max="1000" value="150"></label>
<button onclick="configurar()">Confirmar frente y radio</button><p id="configActual">Frente sin confirmar. Radio indicado por ti: 150 mm.</p>
<button id="avanzar" disabled onclick="mandar('avanzar')">ARRANCAR</button>
<button id="parar" onclick="mandar('stop')">PARAR / STOP</button>
<pre id="estado">Esperando datos...</pre><pre id="diagnostico"></pre><pre id="devkit"></pre>
<p>Mantener esta pagina abierta. Al perder el enlace con la pagina se cancela el movimiento.</p>
<script>
const boton=document.getElementById('avanzar');
const radar=document.getElementById('radar');const ctx=radar.getContext('2d');
radar.addEventListener('click',e=>{const b=radar.getBoundingClientRect();const x=(e.clientX-b.left)/b.width*420-210,y=(e.clientY-b.top)/b.height*420-210;
 if(Math.hypot(x,y)<8)return;document.getElementById('frente').value=Math.round((Math.atan2(x,-y)*180/Math.PI+360)%360)%360});
async function configurar(){
 const f=document.getElementById('frente'),r=document.getElementById('radio');
 if(!f.value||!r.value||!f.checkValidity()||!r.checkValidity()){alert('Indica el ángulo frontal y el radio real del chasis.');return}
 try{const res=await fetch('/configurar?frente='+f.value+'&radio='+r.value+'&confirmar=1',{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('configActual').textContent=await res.text()}catch(e){}
}
function reconectar(){document.getElementById('video').src='http://'+location.hostname+':81/stream?t='+Date.now()}
async function mandar(accion){
 boton.disabled=true;
 try{const r=await fetch('/'+accion,{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('estado').textContent=await r.text()}catch(e){}
}
function dibujar(mapa,frente,configurada){
 ctx.clearRect(0,0,420,420);ctx.strokeStyle='#56738b';ctx.fillStyle='#8deadb';
 for(let r=42;r<=168;r+=42){ctx.beginPath();ctx.arc(210,210,r,0,2*Math.PI);ctx.stroke()}
 ctx.beginPath();ctx.moveTo(210,20);ctx.lineTo(210,400);ctx.moveTo(20,210);ctx.lineTo(400,210);ctx.stroke();
 ctx.font='14px Arial';ctx.fillText('0 grados',220,20);
 mapa.forEach((d,i)=>{if(d>0&&d<=3000){const a=i*5*Math.PI/180;
 const r=d/3000*180;ctx.beginPath();ctx.arc(210+Math.sin(a)*r,210-Math.cos(a)*r,3,0,2*Math.PI);ctx.fill()}});
 ctx.fillStyle='#ffcc55';ctx.fillRect(205,205,10,10);
 if(configurada){const a=frente*Math.PI/180;ctx.strokeStyle='#ffcc55';ctx.beginPath();ctx.moveTo(210,210);ctx.lineTo(210+Math.sin(a)*110,210-Math.cos(a)*110);ctx.stroke()}
}
async function actualizar(){
 try{
  const r=await fetch('/estado',{cache:'no-store'});if(!r.ok)throw Error();const d=await r.json();
  const si=v=>v?'OK':'PENDIENTE';
  document.getElementById('estado').textContent=
   'Camara: '+si(d.camara)+' | FPS enviados: '+d.fps+'\n'+
   'LiDAR: '+si(d.lidar)+' | sectores: '+d.sectores+'/72 | minimo: '+d.minimo+' mm\n'+
   'UART: '+si(d.enlace)+' | MPU: '+si(d.imu)+' | operador: '+si(d.operador)+'\n'+
   'Recorrido de ruedas (incluye giros): '+d.distancia+' cm | marcha autónoma: '+(d.activa?'ACTIVA':'PARADA')+' | motores: '+(d.moviendo?'SI':'NO')+'\n'+
   'LiDAR paquetes: '+d.paquetes+' | errores: '+d.errores+' | barridos: '+d.barridos+'\n'+d.evento;
  document.getElementById('devkit').textContent='DevKitC\n'+d.devkit;
  document.getElementById('diagnostico').textContent=
   'DIAGNOSTICO D6 | bytes: '+d.bytesLidar+' | cabeceras invalidas: '+d.erroresCabecera+
   ' | checksum invalidos: '+d.erroresChecksum+' | inicios validos: '+d.iniciosLidar+'\n'+(d.lidar?'Barridos válidos recibidos':d.diagnosticoLidar);
  boton.disabled=!(d.permiso&&!d.activa);
  document.getElementById('configActual').textContent=d.configurada?'Frente confirmado: '+d.frente+' grados | radio: '+d.radio+' mm':'Confirma el frente del carro para habilitar la marcha';
  dibujar(d.mapa,d.frente,d.configurada);
 }catch(e){boton.disabled=true;document.getElementById('estado').textContent='Sin datos actuales del S3. Comprueba RobotCam; los valores anteriores pueden estar desactualizados.';
 document.getElementById('devkit').textContent='DevKitC: sin telemetria actual; esperando reconexion';
 document.getElementById('diagnostico').textContent='Sin diagnostico actual; esperando reconexion';}
 setTimeout(actualizar,300);
}
reconectar();actualizar();
</script></body></html>
)HTML";

bool peticionLocal(httpd_req_t *req) {
  char valor[4];
  return httpd_req_get_hdr_value_str(req, "X-Robot", valor, sizeof(valor)) == ESP_OK &&
         strcmp(valor, "1") == 0;
}

esp_err_t paginaWeb(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, PAGINA, HTTPD_RESP_USE_STRLEN);
}

esp_err_t estadoWeb(httpd_req_t *req) {
  ultimoNavegador.store(millis());
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  char mapa[512];
  size_t usados = 0;
  for (size_t i = 0; i < 72; ++i) {
    usados += snprintf(mapa + usados, sizeof(mapa) - usados, "%s%u", i ? "," : "", copia.mapa[i]);
  }
  char json[2500];
  snprintf(json, sizeof(json),
    "{\"activa\":%u,\"configurada\":%u,\"frente\":%d,\"radio\":%d,\"permiso\":%u,\"camara\":%u,\"lidar\":%u,\"enlace\":%u,\"imu\":%u,"
    "\"moviendo\":%u,\"operador\":%u,\"minimo\":%u,\"sectores\":%u,\"paquetes\":%lu,"
    "\"errores\":%lu,\"barridos\":%lu,\"fps\":%.1f,\"distancia\":%.1f,"
    "\"bytesLidar\":%lu,\"erroresCabecera\":%lu,\"erroresChecksum\":%lu,\"iniciosLidar\":%lu,"
    "\"diagnosticoLidar\":\"%s\","
    "\"devkit\":\"%s\",\"evento\":\"%s\",\"mapa\":[%s]}",
    copia.activa, copia.configurada, copia.frente, copia.radio, copia.permiso, copia.camara, copia.lidar, copia.enlace, copia.imu, copia.moviendo,
    copia.operador, copia.minimo, copia.sectores, (unsigned long)copia.paquetes,
    (unsigned long)copia.errores, (unsigned long)copia.barridos, copia.fps, copia.distancia,
    (unsigned long)copia.bytesLidar, (unsigned long)copia.erroresCabecera,
    (unsigned long)copia.erroresChecksum, (unsigned long)copia.iniciosLidar, copia.diagnosticoLidar,
    copia.devkit, copia.evento, mapa);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

esp_err_t avanzarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  if (!copia.permiso || !copia.enlace || !copia.imu || copia.activa) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan condiciones. Revisa los indicadores.");
  }
  const uint32_t ahora = millis();
  if (xQueueSend(solicitudesInicio, &ahora, 0) != pdPASS) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ya hay una orden pendiente.");
  }
  return httpd_resp_send(req, "Orden recibida. Esperando respuesta del DevKitC.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t configurarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado); copia = estado; portEXIT_CRITICAL(&muxEstado);
  if (copia.activa || copia.moviendo) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Pulsa STOP antes de configurar");
  char query[100], f[12], r[12], c[4];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "frente", f, sizeof(f)) != ESP_OK ||
      httpd_query_key_value(query, "radio", r, sizeof(r)) != ESP_OK ||
      httpd_query_key_value(query, "confirmar", c, sizeof(c)) != ESP_OK || strcmp(c, "1") != 0)
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan frente, radio y confirmacion");
  char *finF, *finR;
  long frente = strtol(f, &finF, 10), radio = strtol(r, &finR, 10);
  if (!*f || !*r || *finF || *finR || frente < 0 || frente >= 360 || radio < 100 || radio > 1000)
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Geometria fuera de rango");
  geometria.store(0x80000000u | (uint32_t(radio) << 9) | uint32_t(frente));
  solicitudStop.store(true); // evita carreras con un ARRANCAR simultaneo.
  return httpd_resp_send(req, "Frente y radio confirmados. Retira el objeto de referencia.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t pararWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  solicitudStop.store(true);
  return httpd_resp_send(req, "STOP solicitado", HTTPD_RESP_USE_STRLEN);
}

esp_err_t capturaWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  camera_fb_t *imagen = esp_camera_fb_get();
  if (!imagen) return httpd_resp_send_500(req);
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  const esp_err_t resultado = httpd_resp_send(req, (const char *)imagen->buf, imagen->len);
  esp_camera_fb_return(imagen);
  return resultado;
}

esp_err_t videoWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  esp_err_t resultado = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=robotframe");
  if (resultado != ESP_OK) return resultado;
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  videoActivo.store(true);
  uint32_t cuadros = 0, inicioFPS = millis();
  while (true) {
    camera_fb_t *imagen = esp_camera_fb_get();
    if (!imagen) { resultado = ESP_FAIL; break; }
    char cabecera[100];
    const int n = snprintf(cabecera, sizeof(cabecera),
      "\r\n--robotframe\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)imagen->len);
    resultado = httpd_resp_send_chunk(req, cabecera, n);
    if (resultado == ESP_OK) resultado = httpd_resp_send_chunk(req, (const char *)imagen->buf, imagen->len);
    esp_camera_fb_return(imagen);
    if (resultado != ESP_OK) break;
    ultimoFrame.store(millis());
    ++cuadros;
    const uint32_t transcurrido = millis() - inicioFPS;
    if (transcurrido >= 3000) {
      fpsDecimas.store(cuadros * 10000UL / transcurrido);
      inicioFPS = millis();
      cuadros = 0;
    }
    delay(1);
  }
  videoActivo.store(false);
  fpsDecimas.store(0);
  return resultado;
}

bool registrarRuta(httpd_handle_t servidor, const char *uri, httpd_method_t metodo,
                   esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t ruta = {};
  ruta.uri = uri;
  ruta.method = metodo;
  ruta.handler = handler;
  return httpd_register_uri_handler(servidor, &ruta) == ESP_OK;
}

bool iniciarServidores() {
  httpd_config_t web = HTTPD_DEFAULT_CONFIG();
  web.server_port = 80;
  web.stack_size = 8192;
  web.send_wait_timeout = 2;
  if (httpd_start(&servidorWeb, &web) != ESP_OK) return false;
  bool bien = registrarRuta(servidorWeb, "/", HTTP_GET, paginaWeb) &&
    registrarRuta(servidorWeb, "/estado", HTTP_GET, estadoWeb) &&
    registrarRuta(servidorWeb, "/avanzar", HTTP_POST, avanzarWeb) &&
    registrarRuta(servidorWeb, "/configurar", HTTP_POST, configurarWeb) &&
    registrarRuta(servidorWeb, "/stop", HTTP_POST, pararWeb) &&
    registrarRuta(servidorWeb, "/captura", HTTP_GET, capturaWeb);
  httpd_config_t video = HTTPD_DEFAULT_CONFIG();
  video.server_port = 81;
  video.ctrl_port = web.ctrl_port + 1;
  video.stack_size = 8192;
  video.send_wait_timeout = 2;
  bien = bien && httpd_start(&servidorVideo, &video) == ESP_OK &&
         registrarRuta(servidorVideo, "/stream", HTTP_GET, videoWeb);
  if (!bien) {
    if (servidorVideo) httpd_stop(servidorVideo);
    httpd_stop(servidorWeb);
  }
  return bien;
}

bool iniciarCamara() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11; config.pin_d1 = 9; config.pin_d2 = 8; config.pin_d3 = 10;
  config.pin_d4 = 12; config.pin_d5 = 18; config.pin_d6 = 17; config.pin_d7 = 16;
  config.pin_xclk = 15; config.pin_pclk = 13; config.pin_vsync = 6; config.pin_href = 7;
  config.pin_sccb_sda = 4; config.pin_sccb_scl = 5;
  config.pin_pwdn = -1; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 16;
  const bool psram = psramFound();
  config.fb_count = psram ? 2 : 1;
  config.fb_location = psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = psram ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  Serial.printf("PSRAM=%s | buferes=%d\n", psram ? "SI" : "NO", config.fb_count);
  const esp_err_t resultado = esp_camera_init(&config);
  Serial.printf("CAMARA init=0x%X\n", (unsigned)resultado);
  return resultado == ESP_OK;
}

void procesarDevkit(const char *texto) {
  unsigned long recibida, activa, bloqueada;
  unsigned int moviendo, imu;
  float distancia; char causa[48];
  if (sscanf(texto, "A1 %lu %lu %lu %u %u %f %47s", &recibida, &activa, &bloqueada,
      &moviendo, &imu, &distancia, causa) != 7 || recibida != sesion ||
      moviendo > 1 || imu > 1 || !isfinite(distancia) || distancia < 0) return;
  for (const char *p = texto; *p; ++p) {
    if (*p == '"' || *p == '\\' || static_cast<unsigned char>(*p) < 32) return;
  }
  devkitRecibido = true; ultimoDevkit = millis(); devkitMoviendo = moviendo;
  devkitIMU = imu; distanciaDevkit = distancia; bloqueadaDevkit = bloqueada;
  snprintf(mensajeDevkit, sizeof(mensajeDevkit), "%s", texto);
}

void leerUSB() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineaUSB[longitudUSB] = '\0';
      if (!descartarUSB) {
        if (strcmp(lineaUSB, "STOP") == 0) solicitudStop.store(true);
        else if (strcmp(lineaUSB, "ARRANCAR") == 0) {
          const uint32_t ahora = millis();
          xQueueSend(solicitudesInicio, &ahora, 0);
        } else Serial.println("Comandos: ARRANCAR | STOP. Mantener pagina con video abierta.");
      }
      longitudUSB = 0;
      descartarUSB = false;
    } else if (longitudUSB < sizeof(lineaUSB) - 1) lineaUSB[longitudUSB++] = c;
    else descartarUSB = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  sesion = esp_random();
  if (!sesion) sesion = 1;
  solicitudesInicio = xQueueCreate(1, sizeof(uint32_t));
  if (!solicitudesInicio) { Serial.println("ERROR memoria para cola de ordenes"); return; }
  enlace.setRxBufferSize(2048);
  enlace.begin(115200, SERIAL_8N1, 42, 41);
  enviarRobot(enlace, "STOP");
  puertoLidar.setRxBufferSize(8192);
  puertoLidar.begin(230400, SERIAL_8N1, 1, 2);
  camaraIniciada = iniciarCamara();
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("RobotCam", "CAMBIA_CLAVE_ROBOTCAM", 1, false, 1)) {
    Serial.println("ERROR WIFI");
    return;
  }
  servidoresListos = iniciarServidores();
  Serial.printf("AUTONOMO S3 V1.1 REANUDACION | camara=%u web=%u | LiDAR RX1 TX2\n", camaraIniciada, servidoresListos);
  Serial.print("Pagina http://"); Serial.println(WiFi.softAPIP());
  Serial.println("Wi-Fi RobotCam | clave CAMBIA_CLAVE_ROBOTCAM | motores parados hasta nueva orden");
  const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
  puertoLidar.write(iniciar, sizeof(iniciar));
  ultimoArranqueLidar = millis();
}

void loop() {
  if (!solicitudesInicio) { delay(100); return; }
  for (size_t i = 0; i < 4096 && puertoLidar.available(); ++i) {
    lidar.recibir(puertoLidar.read(), millis());
  }
  for (size_t i = 0; i < 1024 && enlace.available(); ++i) {
    const char *texto = receptor.recibir(enlace.read());
    if (texto) procesarDevkit(texto);
  }
  leerUSB();
  const uint32_t ahora = millis();
  if (lidar.paquetes == 0 && ahora - ultimoArranqueLidar > 4000) {
    const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
    puertoLidar.write(iniciar, sizeof(iniciar));
    ultimoArranqueLidar = ahora;
  }
  // Leer primero los sellos escritos por las tareas HTTP y despues el reloj;
  // evita una resta unsigned negativa si llega un frame durante esta iteracion.
  const uint32_t frame = ultimoFrame.load(), navegador = ultimoNavegador.load();
  const uint32_t ahoraSalud = millis();
  const bool video = camaraIniciada && videoActivo.load() && frame && ahoraSalud - frame < 1000;
  const bool operador = navegador && ahoraSalud - navegador < 1200;
  const bool sensor = lidar.vigente(ahora);
  const bool devkit = devkitRecibido && ahora - ultimoDevkit <= 600;
  const ConfigNavegacion config = configuracion();
  const bool salud = servidoresListos && video && operador && sensor && devkit && devkitIMU;
  const char *obstaculo = config.valida() ? navegacion.evaluar(lidar.distancias, config) : "Confirma frente y radio del chasis";
  const char *giro = config.valida() ? navegacion.evaluar(lidar.distancias, config, true) : obstaculo;
  const bool permiso = salud && config.valida() && (!obstaculo || !giro);
  const bool estabaActiva = navegacion.activa();
  if (solicitudStop.exchange(false)) {
    xQueueReset(solicitudesInicio);
    navegacion.parar(); corrida = 0; enviarRobot(enlace, "STOP");
  } else {
    if (navegacion.activa() && (!salud || bloqueadaDevkit == corrida)) {
      navegacion.fallar(bloqueadaDevkit == corrida ? "DevKit detuvo la corrida: revisa su motivo" :
        !sensor ? "LiDAR sin barrido valido" : !devkit ? "Sin telemetria del DevKit" :
        !devkitIMU ? "MPU sin datos" : !operador ? "Navegador desconectado" : "Video o servidor no disponible");
      xQueueReset(solicitudesInicio);
    }
    uint32_t solicitada;
    if (xQueueReceive(solicitudesInicio, &solicitada, 0) == pdPASS && !navegacion.activa()) {
      if (ahora - solicitada <= 500 && permiso) {
        if (++orden == 0) ++orden;
        corrida = orden; navegacion.arrancar(config, ahora);
      }
    }
    navegacion.actualizar(lidar.distancias, config, sensor, lidar.barridos, ahora);
    if (!navegacion.activa()) corrida = 0;
    if (estabaActiva && !navegacion.activa()) enviarRobot(enlace, "STOP");
  }
  if (navegacion.fase == Navegacion::ESPERANDO_ESPACIO)
    snprintf(evento, sizeof(evento), "Espera automatica: %.70s", navegacion.motivo);
  else if (navegacion.activa()) snprintf(evento, sizeof(evento), "%s", navegacion.motivo);
  else if (navegacion.fase == Navegacion::FALLO) snprintf(evento, sizeof(evento), "%s. Pulsa ARRANCAR para reintentar.", navegacion.motivo);
  else snprintf(evento, sizeof(evento), "%s", !config.valida() ? "Confirma frente y radio del chasis" :
      !sensor ? "Esperando barridos validos del LiDAR" : !devkit ? "Esperando DevKit con firmware autonomo V1" :
      !devkitIMU ? "MPU sin datos" : !operador ? "Esperando navegador" : !video ? "Esperando video" :
      obstaculo && giro ? giro : navegacion.motivo);
  if (ahora - ultimoLatido >= 100) {
    ultimoLatido = ahora;
    char mensaje[140];
    snprintf(mensaje, sizeof(mensaje), "V1 %lu %lu %lu %d %d %d %d",
      (unsigned long)sesion, (unsigned long)++secuencia, (unsigned long)corrida,
      navegacion.izquierda, navegacion.izquierda, navegacion.derecha, navegacion.derecha);
    enviarRobot(enlace, mensaje);
  }
  portENTER_CRITICAL(&muxEstado);
  estado.activa = navegacion.activa();
  estado.configurada = config.valida(); estado.frente = config.frente; estado.radio = config.radio;
  estado.permiso = permiso;
  estado.camara = video;
  estado.lidar = sensor;
  estado.enlace = devkit;
  estado.imu = devkit && devkitIMU;
  estado.moviendo = devkit && devkitMoviendo;
  estado.operador = operador;
  estado.minimo = lidar.minimo;
  estado.sectores = lidar.cobertura;
  estado.paquetes = lidar.paquetes;
  estado.bytesLidar = lidar.bytesRecibidos;
  estado.erroresCabecera = lidar.erroresCabecera;
  estado.erroresChecksum = lidar.erroresChecksum;
  estado.iniciosLidar = lidar.inicios;
  memcpy(estado.diagnosticoLidar, lidar.diagnostico, sizeof(estado.diagnosticoLidar));
  estado.errores = lidar.errores;
  estado.barridos = lidar.barridos;
  estado.fps = fpsDecimas.load() / 10.0f;
  estado.distancia = distanciaDevkit;
  memcpy(estado.mapa, lidar.distancias, sizeof(estado.mapa));
  memcpy(estado.devkit, mensajeDevkit, sizeof(estado.devkit));
  memcpy(estado.evento, evento, sizeof(estado.evento));
  portEXIT_CRITICAL(&muxEstado);
  if (ahora - ultimoLog >= 2000) {
    ultimoLog = ahora;
    Serial.printf("ESTADO | cam=%u lidar=%u uart=%u imu=%u permiso=%u | min=%u mm | sectores=%u/72 | paquetes=%lu errores=%lu\n",
      video, sensor, devkit, devkitIMU, permiso, lidar.minimo, lidar.cobertura,
      (unsigned long)lidar.paquetes, (unsigned long)lidar.errores);
  }
  delay(1);
}
```

### `prueba_autonoma_exitosa_2026-11-24/SHA256.txt`

```text
0CC7F43E26AF8519B87313C617E680062B8362E4CC4D33BF76AEA15C4E037D87  LEEME.md
FDBADE6494661BF15CFB7DA1B7586330A8001BCFF62639267DA087C1577EA401  devkit_integral\devkit_integral.ino
C90B940B9E9032B232D8A4212F3786C72321A66CD426A6A954CE8C87AC178A24  devkit_integral\EnlaceRobot.h
387BEDD6033B4FF52F3AA7C7877DC47EDE9C203041EE3E23C4DA13D7D4CEA3A0  devkit_integral\GuardiaMarcha.h
1C440E876BD358C0680A3CCB9846D0D9A6FB2F634F8C852F7CF345C8CD48E7BF  pruebas\interfaz_test.cjs
3771627CEAED3DF9DA1E65A20A68979787A046CD53D2219311F2C3BCAF32A993  pruebas\motores_test.cpp
F79DB1C39ACFDDD14033E83398F3F82E4FD00BF74326D04D174B3E12852E02BE  pruebas\navegacion_test.cpp
68190E0970F0A2D3E64CAE069C257E8F1019FF9CC0B5554F7F5C9508176B6FA4  pruebas\protocolo_test.cpp
5D00191302BDD86C0AA36D70EF4E3CA7F7D25CF4F70B6AE4E99F0C5EAE6AC699  pruebas\dobles\Arduino.h
A6693E4BA03EBDFA60D152C674B92EC2EA877AB27A50423D2E45A376441318B7  pruebas\dobles\Wire.h
C90B940B9E9032B232D8A4212F3786C72321A66CD426A6A954CE8C87AC178A24  s3_integral\EnlaceRobot.h
0E44935939FD49D09F07B0640A5566D8CFE7F4FE3F9AB948C59C5BD28738AAD4  s3_integral\EstadoIntegral.h
F173D33189FEEECF20F6527CE96A81CB8B31768BBD2F57F5708101021B51B541  s3_integral\LidarD6.h
980A38FC936F5FC045AEA7EBD89C1B6E089A77E18B321D50E21788B4BC60FA5D  s3_integral\Navegacion.h
330D507D11337AFF0444CC0B7A9C5F3A154D4FC5B1B845671DFC13498ADFE979  s3_integral\s3_integral.ino
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/devkit_integral/devkit_integral.ino`

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "EnlaceRobot.h"
#include "GuardiaMarcha.h"

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
uint32_t ultimoReporte = 0;
uint32_t ultimaLecturaIMU = 0, ultimoIntentoIMU = 0;
bool moviendo = false;
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
  for (int valor : v) if (valor < -80 || valor > 80) return;
  if (!corrida) for (int valor : v) if (valor) return;
  const uint32_t anterior = guardia.corrida;
  if (!guardia.aceptar(sesion, secuencia, corrida, millis())) return;
  if (!corrida) { for (int &valor : objetivo) valor = 0; apagarSalidas(); return; }
  if (!imuValida || millis() - ultimaLecturaIMU > 250) { detener("MPU_SIN_DATOS"); return; }
  if (corrida != anterior) {
    copiarPulsos(iniciales);
    copiarPulsos(anteriores);
    for (uint32_t &t : ultimoPulso) t = millis();
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
  Serial.printf("AUTONOMO DEVKIT V1 | MPU=%s | motores deshabilitados\n", mpu ? "SI" : "NO");
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
    if (!aplicado[i] || cuentas[i] != anteriores[i]) ultimoPulso[i] = ahora;
    anteriores[i] = cuentas[i];
    if (aplicado[i] && ahora - ultimoPulso[i] > LIMITE_ENCODER_MS) {
      detener("ENCODER_SIN_PULSOS"); break;
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
  delay(1);
}
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/devkit_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/devkit_integral/GuardiaMarcha.h`

```cpp
#pragma once
#include <stdint.h>

// Cada ARRANCAR crea una corrida nueva. Un fallo/STOP invalida esa corrida:
// recibir otra vez la misma orden no vuelve a poner los motores en marcha.
class GuardiaMarcha {
 public:
  uint32_t sesion = 0, secuencia = 0, corrida = 0, ultimaCorrida = 0, bloqueada = 0, ultimo = 0;
  bool recibido = false;
  void bloquear() { if (corrida) bloqueada = corrida; corrida = 0; }
  bool vigente(uint32_t ahora) const { return recibido && ahora - ultimo <= 450; }
  bool aceptar(uint32_t s, uint32_t q, uint32_t r, uint32_t ahora) {
    if (!s) return false;
    if (s != sesion) {
      if (r != 0) return false; // reinicio requiere primero sincronizacion en reposo.
      sesion = s; secuencia = 0; corrida = ultimaCorrida = bloqueada = 0; recibido = false;
    }
    if (static_cast<int32_t>(q - secuencia) <= 0) return false;
    if (r && (r == bloqueada || static_cast<int32_t>(r - ultimaCorrida) < 0)) return false;
    secuencia = q; ultimo = ahora; recibido = true;
    if (!r) bloquear();
    else { corrida = r; ultimaCorrida = r; }
    return true;
  }
};
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/LEEME.md`

```markdown
# Navegación continua — versión experimental V1

Preparada el 14 de noviembre de 2026. Conserva el cableado y el lector COIN-D6 V3 que funcionó. El respaldo anterior está en `primera_prueba_exitosa_2026-11-14`, con manifiesto SHA256. Los originales en `prueba_integral_50cm` también permanecen sin cambios.

## Qué hace

- **ARRANCAR:** inicia una corrida sin meta de distancia y sin límite de duración del avance.
- **PARAR / STOP:** cancela la corrida y deshabilita los motores.
- El S3 evalúa el LiDAR y decide avance o giro. El DevKit controla por separado las cuatro salidas, con signo: positivo avanza, negativo retrocede, cero deshabilita.
- Cuando un obstáculo entra en la trayectoria, hace una pausa de al menos 250 ms y pivota a la izquierda: FL y RL hacia atrás, FR y RR hacia adelante. No retrocede el vehículo completo.
- Al observar una trayectoria libre durante tres barridos nuevos consecutivos, termina el giro y vuelve al avance. El cambio de sentido incluye una pausa de potencia de al menos 150 ms.
- No gira una cantidad fija de grados: gira hasta encontrar una trayectoria libre. No hay un botón de vuelta de 360 grados ni navegación hacia un destino.
- Si queda atrapado, falta información para girar o fallan sensores/comunicaciones, se detiene. Requiere otro ARRANCAR; nunca se rearma por recuperar Wi-Fi o por repetir una orden vieja.

## Archivos para Arduino IDE: cargar AMBAS placas

1. DevKitC: abrir `devkit_integral/devkit_integral.ino`. Sus pestañas incluyen `EnlaceRobot.h` y `GuardiaMarcha.h`.
2. ESP32-S3: abrir `s3_integral/s3_integral.ino`. Sus pestañas incluyen `EnlaceRobot.h`, `EstadoIntegral.h`, `LidarD6.h` y `Navegacion.h`.

Abrir los archivos desde ESTA carpeta. No pegar fragmentos dentro de los programas anteriores ni juntar los dos `.ino` en un sketch. Los encabezados se compilan automáticamente.

**Hay que actualizar los dos ESP32.** El protocolo nuevo `V1/A1` es incompatible con el de la prueba de 50 cm. Si uno conserva el firmware antiguo, la página mostrará que falta el DevKit y no permitirá arrancar. No se cargó firmware automáticamente y no se añadió OTA.

Opciones verificadas con el paquete Arduino-ESP32 3.3.12 instalado:

| Placa física | Selección en Arduino IDE | Opciones |
|---|---|---|
| DevKitC WROOM-32D | ESP32 Dev Module | Flash Mode DIO |
| S3 | ESP32S3 Dev Module | Flash 16MB; OPI PSRAM; APP 3MB/FATFS 9MB; USB CDC On Boot Disabled con el USB-UART usado anteriormente |

Puerto: seleccionar el de cada placa; los números COM pueden cambiar. Mantener el aislamiento de alimentación externa/USB ya adoptado y no alimentar una placa apagada por su enlace UART. Programar con los motores sin alimentación.

## Cableado conservado

| Conexión | Destino |
|---|---|
| D6 amarillo TX | S3 GPIO1 RX, UART 230400 8N1 |
| D6 verde RX | S3 GPIO2 TX |
| D6 rojo / negro | 5V regulados / GND común |
| S3 TX41 | DevKit RX16, UART 115200 8N1 |
| DevKit TX17 | S3 RX42 |
| GND | Común entre las placas, sensor y alimentación |

No cambiar pines, regulador ni las inversiones de los motores. XL4016 continúa a 5V y se conserva la lógica a 3V3 del montaje probado. Los GPIO no reciben señales de 5V.

| Rueda | IN1 | IN2 | PWM | Encoder | Invertir |
|---|---:|---:|---:|---:|---|
| FL, delantera izquierda | 13 | 14 | 25 | 34 | Sí |
| RL, trasera izquierda | 26 | 27 | 32 | 35 | No |
| FR, delantera derecha | 18 | 19 | 23 | 36 | Sí |
| RR, trasera derecha | 4 | 15 | 33 | 39 | No |

STBY DevKit GPIO2; MPU SDA21/SCL22. Cámara y demás conexiones iguales a la prueba integral.

## Arranque desde Wi-Fi

1. Cargar ambos sketches y reiniciar las placas con su alimentación de funcionamiento. Ningún motor arranca por encender.
2. Conectar a **RobotCam**, contraseña **CAMBIA_CLAVE_ROBOTCAM**. Abrir **http://192.168.4.1/**. Mantener página y video abiertos; el AP admite un cliente.
3. Configurar la geometría estando detenido. El radio inicial del formulario es **150 mm**, según tu confirmación de que el LiDAR está centrado y hay 15 cm hasta cada punta del chasis. Debe cubrir también cualquier pieza que sobresalga.
4. Identificar el frente: colocar un objeto reconocible delante del carro, a la altura del plano del LiDAR, y observar qué puntos aparecen o se desplazan en el radar. Pulsar ese punto para rellenar su ángulo, o escribir el ángulo frontal. No asumir que 0 grados es el frente. Pulsar **Confirmar frente y radio** y retirar el objeto. Una línea amarilla indica el frente elegido.
5. Esa configuración se conserva en RAM hasta reiniciar el S3. Tras un reinicio vuelve a requerir confirmación; no hay arranque automático.
6. Confirmar Cámara, LiDAR, UART y MPU OK. Debajo se indica el motivo exacto si ARRANCAR sigue bloqueado.
7. En la primera validación del firmware nuevo, comprobar el sentido de las cuatro ruedas con el chasis elevado y los sentidos de avance/giro antes de probar obstáculos en el suelo. No se repiten las mediciones de voltaje ya realizadas.
8. En suelo despejado y a baja velocidad, pulsar ARRANCAR. Para la primera prueba de evasión usar un obstáculo ancho que corte el plano del LiDAR y dejar espacio alrededor del vehículo. Mantener acceso al interruptor físico y al botón PARAR / STOP.

## Cómo decide y por qué puede detenerse

La geometría se evalúa en `Navegacion.h`. Usa un círculo que cubre el chasis, con **100 mm de margen inicial**, y una trayectoria de anticipación de **500 mm** hacia adelante. Esos 500 mm son anticipación para esquivar: NO son un límite de recorrido. Se añade margen angular por el ancho de las celdas de planificación de 10 grados.

PWM de avance y giro: **80/255**, conservado de la prueba de motores. No es velocidad en cm/s ni una regulación PID. El margen de frenado y el giro aún deben validarse físicamente con el peso y el suelo reales. El recorrido de ruedas mostrado incluye giros; no representa desplazamiento recto ni posición del robot.

- Un eco a menos de radio + margen (**250 mm con radio de 150**) impide avanzar y pivotar. No se borran esos puntos ni se ignoran como si fueran cables. Las capturas anteriores mostraban 54–71 mm: si persisten, despejar el plano del sensor y comprobar qué está detectando.
- Los ceros significan **sin medida**, nunca distancia infinita. Para decidir, los 72 sectores se agrupan en 36 celdas de 10 grados, usando la menor distancia real de cada par y ampliando el margen angular. Un hueco aislado de 5 grados no elimina la medida vecina; una celda sin ninguna medida permanece desconocida. Avanzar requiere observaciones en las celdas delanteras; pivotar exige las **36 celdas observadas en el barrido actual** y despejadas alrededor del círculo. Con 67/72 o 68/72 sectores originales puede haber observación de las 36 celdas. Si faltan celdas para girar, se detiene y lo explica. No se interpolan distancias ni se ocultan ecos cercanos. Esta discretización no certifica ausencia de objetos entre rayos.
- Barridos inválidos o de más de 500 ms cancelan la corrida. El lector conserva los requisitos de integridad del protocolo y de cobertura de la versión que funcionó.
- Un giro sin salida en 6,5 segundos termina en fallo, en lugar de dar vueltas indefinidamente.
- Cerrar la página, perder video o perder telemetría del DevKit cancela la corrida.
- El DevKit tiene su propio plazo de 450 ms sin órdenes válidas, además de parada por MPU sin datos y por una rueda activada sin pulsos durante 1,5 segundos.
- Ante un fallo se invalida la corrida actual. Una trama atrasada no la vuelve a activar. La causa local se conserva en la telemetría.

El comportamiento solicitado se cumple en condiciones normales: marcha continua, evasión y botón para detener. Las paradas de protección se conservan cuando continuar no tiene una maniobra válida.

Es evasión reactiva experimental, con preferencia fija por girar a la izquierda. No hay mapa, planificación global, rectitud garantizada, detección de huecos/escaleras ni garantía de evitar todo objeto: un LiDAR de un solo plano puede no ver objetos bajos o fuera de ese plano. No es todavía Nav2 ni navegación hacia objetos.

## Verificación del software

- Compilación de ambos sketches para las placas indicadas.
- `pruebas/protocolo_test.cpp`: checksum D6 corregido, CRC UART, resincronización, datos corruptos, barridos y caducidad.
- `pruebas/navegacion_test.cpp`: avance durante más de tres minutos simulados, obstáculo, pausa, giro, tres barridos de salida, STOP, orientación, puntos cercanos y sectores desconocidos; secuencias, sesiones y rearme.
- `pruebas/motores_test.cpp`: ejecuta el sketch DevKit con dobles de GPIO/UART/I2C. Comprueba cuatro salidas y sus inversiones, pausa entre sentidos, STOP, trama tardía después del plazo, falta de pulsos y fallo MPU. Los dobles se usan solo en PC, nunca se cargan al robot.
- `pruebas/interfaz_test.cjs`: ejecuta el JavaScript real con una página simulada; comprueba habilitación de ARRANCAR, STOP, configuración del ángulo cero, selección del frente en radar y desconexión. Sintaxis comprobada con Node.

Estas verificaciones no sustituyen la prueba física de evasión. Estado: compilado y probado en software; pendiente de cargar y validar en el vehículo.
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/pruebas/dobles/Arduino.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <deque>
#define CONFIG_IDF_TARGET_ESP32 1
#define ARDUINO_ISR_ATTR
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define FALLING 2
#define SERIAL_8N1 0
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE = int;
inline void portENTER_CRITICAL(int*) {} inline void portEXIT_CRITICAL(int*) {}
inline void portENTER_CRITICAL_ISR(int*) {} inline void portEXIT_CRITICAL_ISR(int*) {}
inline uint32_t reloj = 0;
inline int pines[64]{}, potencia[64]{};
inline uint32_t millis(){return reloj;}
inline void delay(unsigned ms){reloj+=ms;}
inline void pinMode(int,int){}
inline void digitalWrite(int pin,int v){pines[pin]=v;}
inline void analogWrite(int pin,int v){potencia[pin]=v;}
inline void attachInterrupt(int,void(*)(),int){}
class HardwareSerial {
 public:
  explicit HardwareSerial(int=0){}
  std::deque<char> rx; std::string tx;
  void setRxBufferSize(int){} void begin(int,int=0,int=0,int=0){}
  int available(){return rx.size();} char read(){char c=rx.front();rx.pop_front();return c;}
  void print(char c){tx+=c;} void print(const char*s){tx+=s;}
  void println(const char*s){tx+=s;tx+='\n';}
  template<typename... T> void printf(const char* f,T...args){char b[512];snprintf(b,sizeof(b),f,args...);tx+=b;}
};
inline HardwareSerial Serial;
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/pruebas/dobles/Wire.h`

```cpp
#pragma once
#include <stdint.h>
struct WireDoble {
 void begin(int,int,int){} void setTimeOut(int){}
 void beginTransmission(uint8_t){} void write(uint8_t){}
 int endTransmission(bool=true){return 0;}
 int requestFrom(uint8_t,uint8_t n,bool){return n;}
 int available(){return 0;} uint8_t read(){return 0;}
};
inline WireDoble Wire;
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/pruebas/interfaz_test.cjs`

```javascript
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const source=fs.readFileSync(path.join(__dirname,'../s3_integral/s3_integral.ino'),'utf8');
const js=source.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements={},events={},requests=[];
const ctx=new Proxy({},{get:()=>()=>{}});
for(const id of ['avanzar','radar','video','frente','radio','estado','configActual','diagnostico','devkit']) {
 elements[id]={disabled:false,textContent:'',value:'',checkValidity:()=>true,
  getContext:()=>ctx,addEventListener:(type,fn)=>events[type]=fn,
  getBoundingClientRect:()=>({left:0,top:0,width:420,height:420})};
}
let state={permiso:1,activa:0,configurada:1,frente:0,radio:150,camara:1,lidar:1,enlace:1,imu:1,
 operador:1,moviendo:0,fps:10,distancia:0,sectores:68,minimo:900,paquetes:12,errores:0,barridos:1,
 bytesLidar:1000,erroresCabecera:0,erroresChecksum:0,iniciosLidar:2,mapa:Array(72).fill(900),devkit:'A1',evento:'Listo'};
let disconnected=false;
const sandbox={document:{getElementById:id=>elements[id]},location:{hostname:'192.168.4.1'},
 setTimeout:()=>{},alert:()=>{},fetch:async(url,options)=>{
  requests.push({url,options});if(disconnected)throw Error('Offline');
  return {ok:true,json:async()=>state,text:async()=> 'OK'};
 }};
vm.createContext(sandbox);vm.runInContext(js,sandbox);
(async()=>{
 await sandbox.actualizar();assert.equal(elements.avanzar.disabled,false);
 state.activa=1;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 await sandbox.mandar('stop');assert.equal(requests.at(-1).url,'/stop');
 state.activa=0;state.permiso=0;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 elements.frente.value='0';elements.radio.value='150';await sandbox.configurar();
 assert.equal(requests.at(-1).url,'/configurar?frente=0&radio=150&confirmar=1');
 events.click({clientX:310,clientY:210});assert.equal(elements.frente.value,90);
 disconnected=true;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 console.log('OK: interfaz real, bloqueo de ARRANCAR, STOP, frente cero, seleccion de angulo y desconexion');
})().catch(e=>{console.error(e);process.exitCode=1});
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/pruebas/motores_test.cpp`

```cpp
#include "../devkit_integral/devkit_integral.ino"
#include <cassert>
#include <iostream>

void tiempo(uint32_t t) { reloj=t; imuValida=true; direccionMPU=0x68; ultimaLecturaIMU=t; }
void orden(const char *s) { procesarEnlace(s); aplicarObjetivo(millis()); }
int main() {
  tiempo(100); orden("V1 123 1 0 0 0 0 0");
  orden("V1 123 2 1 80 80 80 80");
  assert(moviendo && pines[STBY]==HIGH);
  assert(pines[13]==LOW && pines[14]==HIGH); // inversion FL conservada.
  assert(pines[26]==HIGH && pines[27]==LOW);
  for(const Motor&m:motores) assert(potencia[m.pwm]==80);
  tiempo(200); orden("V1 123 3 1 -80 -80 80 80");
  assert(!moviendo && pines[STBY]==LOW);
  tiempo(349); aplicarObjetivo(millis()); assert(!moviendo);
  tiempo(350); aplicarObjetivo(millis());
  assert(moviendo && pines[13]==HIGH && pines[14]==LOW && pines[26]==LOW && pines[27]==HIGH);
  assert(pines[18]==LOW && pines[19]==HIGH && pines[4]==HIGH && pines[15]==LOW);
  tiempo(400); orden("V1 123 4 1 0 0 0 0"); assert(!moviendo);
  tiempo(500); orden("V1 123 5 1 80 80 80 80"); assert(!moviendo);
  tiempo(650); aplicarObjetivo(millis()); assert(moviendo);
  detener("STOP_USB"); orden("V1 123 6 1 80 80 80 80"); assert(!moviendo);
  tiempo(700); orden("V1 123 7 2 80 80 80 80"); assert(moviendo);
  // Una trama tardia ya en el bufer no puede reanimar la corrida caducada.
  tiempo(1200); char linea[80];
  snprintf(linea,sizeof(linea),"V1 123 8 2 80 80 80 80");
  HardwareSerial salida; enviarRobot(salida,linea);
  for(char ch:salida.tx) enlace.rx.push_back(ch);
  loop(); assert(!moviendo && guardia.bloqueada==2);
  tiempo(1300); orden("V1 123 9 3 80 80 80 80"); assert(moviendo);
  tiempo(2810); procesarEnlace("V1 123 10 3 80 80 80 80"); loop();
  assert(!moviendo && std::string(motivo)=="ENCODER_SIN_PULSOS");
  procesarEnlace("STOP"); assert(std::string(motivo)=="ENCODER_SIN_PULSOS");
  tiempo(3000); orden("V1 123 11 4 80 80 80 80"); assert(moviendo);
  imuValida=false; procesarEnlace("V1 123 12 4 80 80 80 80"); assert(!moviendo);
  std::cout<<"OK: salidas de cuatro motores, inversion, pausa no bloqueante, STOP, UART tardia, encoders y MPU\n";
}
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/pruebas/navegacion_test.cpp`

```cpp
#include "../s3_integral/Navegacion.h"
#include "../devkit_integral/GuardiaMarcha.h"
#include <cassert>
#include <algorithm>
#include <iostream>

int main() {
  uint16_t mapa[72]; std::fill_n(mapa,72,2000);
  ConfigNavegacion c{0,150,true}; Navegacion n;
  assert(!n.arrancar({0,150,false},0));
  assert(!n.arrancar({360,150,true},0));
  assert(n.arrancar(c,0));
  for (uint32_t i=1;i<2000;++i) {
    n.actualizar(mapa,c,true,i,i*100);
    assert(n.activa() && n.izquierda==80 && n.derecha==80);
  } // mas de 3 minutos: sin limite de tiempo o centimetros.
  mapa[0]=600;
  n.actualizar(mapa,c,true,2000,200000);
  assert(n.fase==Navegacion::PAUSA_GIRO && !n.izquierda && !n.derecha);
  n.actualizar(mapa,c,true,2001,200200); assert(!n.izquierda);
  n.actualizar(mapa,c,true,2002,200300);
  assert(n.fase==Navegacion::IZQUIERDA && n.izquierda==-80 && n.derecha==80);
  mapa[0]=2000;
  n.actualizar(mapa,c,true,2003,200400);
  n.actualizar(mapa,c,true,2003,200500); // mismo barrido no vale como confirmacion nueva.
  assert(n.fase==Navegacion::IZQUIERDA);
  n.actualizar(mapa,c,true,2004,200600);
  n.actualizar(mapa,c,true,2005,200700);
  assert(n.fase==Navegacion::RECTO && n.izquierda==0);
  n.actualizar(mapa,c,true,2006,200800); assert(n.izquierda==80);
  n.parar(); n.actualizar(mapa,c,true,2007,200900); assert(!n.activa() && !n.izquierda);
  n.arrancar(c,0); n.actualizar(mapa,c,false,1,1); assert(n.fase==Navegacion::FALLO);
  n.actualizar(mapa,c,true,2,100); assert(!n.activa()); // no rearme automatico.
  mapa[40]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(n.fase==Navegacion::FALLO && !n.izquierda); // ecos cercanos nunca se ocultan.
  mapa[40]=2000; mapa[0]=mapa[1]=0; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(!n.activa()); // cero frontal no significa libre.
  mapa[0]=600; mapa[1]=2000; mapa[40]=mapa[41]=0; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  assert(!n.activa()); // zona de giro sin observar.
  mapa[40]=mapa[41]=2000; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100);
  n.actualizar(mapa,c,true,2,400); n.actualizar(mapa,c,true,3,6701);
  assert(!n.activa()); // atrapado, no girar indefinidamente.
  mapa[0]=2000; mapa[18]=600; c.frente=90;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.fase==Navegacion::PAUSA_GIRO);
  // Falta trasera no impide avanzar, pero nunca se considera libre para pivotar.
  c.frente=0; mapa[18]=2000; mapa[40]=mapa[41]=0;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.izquierda==80);
  // Cuatro huecos aislados: cada celda conserva una medida real y el margen angular.
  std::fill_n(mapa,72,2000); mapa[0]=600;
  mapa[11]=mapa[21]=mapa[41]=mapa[51]=0;
  n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(n.fase==Navegacion::PAUSA_GIRO);
  mapa[10]=54; n.arrancar(c,0); n.actualizar(mapa,c,true,1,100); assert(!n.activa());
  GuardiaMarcha g;
  assert(!g.aceptar(123,1,1,0)); // nueva sesion primero en reposo.
  assert(g.aceptar(123,1,0,0)); assert(g.aceptar(123,2,1,100));
  assert(g.vigente(550) && !g.vigente(551));
  g.bloquear(); assert(!g.aceptar(123,3,1,600));
  assert(g.aceptar(123,4,0,700)); assert(!g.aceptar(123,5,1,800));
  assert(g.aceptar(123,6,2,900)); assert(!g.aceptar(123,6,2,1000));
  assert(!g.aceptar(124,1,3,1100)); assert(g.aceptar(124,1,0,1100));
  assert(g.aceptar(124,2,1,1200));
  std::cout << "OK: marcha continua, giro, salida estable, STOP, geometria, ceros, ecos cercanos, caducidad y rearme\n";
}
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/pruebas/protocolo_test.cpp`

```cpp
#include "../s3_integral/LidarD6.h"
#include "../s3_integral/EnlaceRobot.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

std::vector<uint8_t> paquete(bool inicio, unsigned n, float a, float b, unsigned mm) {
  std::vector<uint8_t> p(10 + 3*n);
  auto poner=[&](unsigned i,unsigned v){p[i]=v&255;p[i+1]=v>>8;};
  p[0]=0xAA;p[1]=0x55;p[2]=inicio?21:20;p[3]=n;
  poner(4,unsigned(a*128)+1);poner(6,unsigned(b*128)+1);
  for(unsigned i=0;i<n;++i){p[10+3*i]=0;p[11+3*i]=(mm&63)<<2;p[12+3*i]=mm>>6;}
  unsigned cs=0;
  // SDK: cabecera 16 bits; cada muestra usa 8 bits y luego 16 bits LE.
  for(unsigned i=0;i<8;i+=2)cs^=p[i]|(p[i+1]<<8);
  for(unsigned i=10;i<p.size();i+=3)cs^=p[i]^(p[i+1]|(p[i+2]<<8));
  poner(8,cs);return p;
}
void dar(LidarD6 &l,const std::vector<uint8_t>&p,unsigned t){for(auto b:p)l.recibir(b,t);}
int main(){
  // Vector fijo independiente del generador: XOR SDK=0x5AFC, distancia=946 mm.
  // El agrupamiento anterior produce 0x9F39 y debe rechazar la trama.
  const std::vector<uint8_t> vectorSDK = {
    0xAA,0x55,0x15,0x01,0x01,0x00,0x01,0x00,0xFC,0x5A,0x88,0xCB,0x0E
  };
  LidarD6 referencia;
  dar(referencia,vectorSDK,0);
  dar(referencia,vectorSDK,100);
  assert(referencia.paquetes==2 && referencia.inicios==2 && referencia.errores==0);
  assert(referencia.distancias[0]==946);
  auto checksumAnterior=vectorSDK;
  checksumAnterior[8]=0x39;checksumAnterior[9]=0x9F;
  dar(referencia,checksumAnterior,120);
  assert(referencia.paquetes==2 && referencia.erroresChecksum==1);
  assert(crcRobot("123456789",9)==0x29B1);
  ReceptorRobot r;
  auto recibir=[&](std::string s){std::string salida;for(char c:s){auto p=r.recibir(c);if(p)salida=p;}return salida;};
  assert(recibir("ruido@123456789*29B1\n")=="123456789");
  assert(recibir("@123456788*29B1\n").empty());
  assert(recibir("@incompleto@123456789*29B1\r\n")=="123456789");
  assert(recibir("@"+std::string(500,'x')+"\n").empty());
  assert(recibir("@123456789*29B1\n")=="123456789");
  LidarD6 l;
  dar(l,paquete(true,1,0,0,800),0);
  dar(l,paquete(false,144,0,357.5,800),20);
  dar(l,paquete(true,1,0,0,800),100);
  assert(l.completo&&l.cobertura==72&&l.minimo==800&&l.vigente(600)&&!l.vigente(601));
  auto malo=paquete(false,144,0,357.5,100);malo[10]^=4;
  dar(l,malo,120);dar(l,paquete(true,1,0,0,800),200);
  assert(!l.completo&&l.errores>0);
  dar(l,paquete(false,144,0,357.5,900),220);dar(l,paquete(true,1,0,0,900),300);
  assert(l.completo&&l.minimo==800); // muestra de inicio del barrido anterior.
  dar(l,paquete(false,144,0,357.5,0),320);dar(l,paquete(true,1,0,0,900),400);
  assert(!l.completo); // ceros no prueban que el entorno esta libre.
  LidarD6 wrap;
  dar(wrap,paquete(true,1,0,0,1000),0);
  dar(wrap,paquete(false,5,350,10,200),10);
  dar(wrap,paquete(true,1,0,0,1000),100);
  assert(!wrap.completo&&wrap.distancias[70]==200&&wrap.distancias[0]==200&&wrap.distancias[2]==200);
  std::cout<<"OK: CRC, tramas corruptas, resincronizacion, barridos, caducidad, ceros y cruce de 360 grados\n";
}
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/s3_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/s3_integral/EstadoIntegral.h`

```cpp
#pragma once
#include <stdint.h>

struct EstadoIntegral {
  bool activa = false, configurada = false;
  int frente = 0, radio = 150;
  bool permiso = false;
  bool camara = false;
  bool lidar = false;
  bool enlace = false;
  bool imu = false;
  bool moviendo = false;
  bool operador = false;
  uint16_t minimo = 0;
  uint8_t sectores = 0;
  uint32_t paquetes = 0;
  uint32_t bytesLidar = 0;
  uint32_t erroresCabecera = 0;
  uint32_t erroresChecksum = 0;
  uint32_t iniciosLidar = 0;
  char diagnosticoLidar[180] = "Esperando bytes del LiDAR";
  uint32_t errores = 0;
  uint32_t barridos = 0;
  float fps = 0;
  float distancia = 0;
  uint16_t mapa[72]{};
  char devkit[300] = "Esperando DevKitC";
  char evento[96] = "Esperando video, LiDAR y DevKitC";
};
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/s3_integral/LidarD6.h`

```cpp
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
        completo = !corrupto && cobertura >= 60 && puntos >= 100 &&
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
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/s3_integral/Navegacion.h`

```cpp
#pragma once
#include <stdint.h>
#include <math.h>

// Planificador reactivo conservador, independiente del hardware. Nunca retrocede.
// El radio debe incluir TODO el chasis desde el eje del LiDAR.
struct ConfigNavegacion {
  int frente = 0;
  int radio = 0;
  bool confirmada = false;
  bool valida() const { return confirmada && frente >= 0 && frente < 360 && radio >= 100 && radio <= 1000; }
};

class Navegacion {
 public:
  enum Fase { PARADO, RECTO, PAUSA_GIRO, IZQUIERDA, FALLO };
  Fase fase = PARADO;
  int izquierda = 0, derecha = 0;
  const char *motivo = "Pulsa ARRANCAR";
  bool activa() const { return fase == RECTO || fase == PAUSA_GIRO || fase == IZQUIERDA; }
  void parar(const char *causa = "Detenido por el operador") {
    fase = PARADO; izquierda = derecha = 0; motivo = causa;
  }
  void fallar(const char *causa) { fase = FALLO; izquierda = derecha = 0; motivo = causa; }
  bool arrancar(const ConfigNavegacion &c, uint32_t ahora) {
    if (!c.valida()) { fallar("Configura frente y radio del chasis"); return false; }
    fase = RECTO; izquierda = derecha = 0; inicioFase = ahora; ultimoBarrido = UINT32_MAX; libres = 0;
    motivo = "Evaluando barrido antes de mover"; return true;
  }
  // 0 significa sin medida, NUNCA espacio libre. No se filtran ecos cercanos.
  const char *evaluar(const uint16_t *mapa, const ConfigNavegacion &c, bool giro = false) const {
    const float radio = c.radio + 100.0f; // margen inicial de 10 cm, por validar fisicamente.
    // Planificar en 36 celdas de 10 grados: conservar el eco menor de cada par
    // de sectores de 5 grados. Una celda sin ninguna medida sigue desconocida.
    // No se interpolan distancias ni se elimina un eco cercano.
    for (int i = 0; i < 36; ++i) {
      const uint16_t a0 = mapa[2*i], a1 = mapa[2*i+1];
      const uint16_t medida = !a0 ? a1 : !a1 ? a0 : a0 < a1 ? a0 : a1;
      float a = (i * 10.0f + 5.0f - c.frente) * 0.01745329252f;
      float x = cosf(a), y = sinf(a);
      if (medida && medida <= radio) return "Obstaculo dentro del margen del chasis";
      if (giro && !medida) return "Giro bloqueado: falta observar una zona del LiDAR";
      if (giro) continue;
      if (x <= 0) continue;
      if (!medida) return "Falta medida en la zona de avance";
      // Capsula barrida: circulo del chasis desplazado 50 cm hacia adelante.
      // Agrandar por media apertura del sector para no tratar el centro como un rayo exacto.
      const float d = medida;
      const float ancho = radio + d * 0.08716f;
      const float px = d * x, py = fabsf(d * y);
      const float dx = px > 500.0f ? px - 500.0f : 0.0f;
      if (dx * dx + py * py <= ancho * ancho) return "Obstaculo en trayectoria: buscando giro";
    }
    return nullptr;
  }
  void actualizar(const uint16_t *mapa, const ConfigNavegacion &c, bool sensor,
                  uint32_t barrido, uint32_t ahora) {
    if (!activa()) return;
    if (!sensor) { fallar("LiDAR sin barrido valido reciente"); return; }
    if (!c.valida()) { fallar("Configuracion de geometria invalida"); return; }
    if ((fase == IZQUIERDA || fase == PAUSA_GIRO) && ahora - inicioFase > 6500) {
      fallar("No se encontro salida durante el giro"); return;
    }
    if (barrido == ultimoBarrido) return;
    ultimoBarrido = barrido;
    const char *frente = evaluar(mapa, c);
    const char *giro = evaluar(mapa, c, true);
    if (fase == RECTO) {
      if (!frente) { izquierda = derecha = 80; motivo = "Avanzando sin limite de distancia"; return; }
      izquierda = derecha = 0;
      if (giro) { fallar(giro); return; }
      fase = PAUSA_GIRO; inicioFase = ahora; libres = 0;
      motivo = "Pausa antes de girar a la izquierda"; return;
    }
    if (giro) { fallar(giro); return; }
    if (fase == PAUSA_GIRO) {
      if (ahora - inicioFase >= 250) { fase = IZQUIERDA; izquierda = -80; derecha = 80; motivo = "Girando a la izquierda para esquivar"; }
      return;
    }
    if (!frente) ++libres; else libres = 0;
    if (libres >= 3) {
      fase = RECTO; izquierda = derecha = 0; motivo = "Salida libre: retomando avance";
      return;
    }
    izquierda = -80; derecha = 80;
  }
 private:
  uint32_t inicioFase = 0, ultimoBarrido = UINT32_MAX;
  uint8_t libres = 0;
};
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/s3_integral/s3_integral.ino`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "EnlaceRobot.h"
#include "LidarD6.h"
#include "EstadoIntegral.h"
#include "Navegacion.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Selecciona ESP32S3 Dev Module."
#endif

Navegacion navegacion;
std::atomic<uint32_t> geometria{0}; // RAM: vuelve a confirmar el frente tras reiniciar.
uint32_t corrida = 0, bloqueadaDevkit = 0;
ConfigNavegacion configuracion() {
  const uint32_t g = geometria.load();
  return {int(g & 511), int((g >> 9) & 2047), bool(g & 0x80000000u)};
}
HardwareSerial enlace(1);
HardwareSerial puertoLidar(2);
ReceptorRobot receptor;
LidarD6 lidar;
httpd_handle_t servidorWeb = nullptr, servidorVideo = nullptr;
QueueHandle_t solicitudesInicio = nullptr;
portMUX_TYPE muxEstado = portMUX_INITIALIZER_UNLOCKED;
EstadoIntegral estado;
std::atomic<bool> solicitudStop{false};
std::atomic<bool> videoActivo{false};
std::atomic<uint32_t> ultimoFrame{0}, ultimoNavegador{0};
std::atomic<uint32_t> fpsDecimas{0};
bool camaraIniciada = false, servidoresListos = false;
bool devkitIMU = false, devkitMoviendo = false, devkitRecibido = false;
float distanciaDevkit = 0;
uint32_t sesion = 0, secuencia = 0, orden = 0;
uint32_t ultimoDevkit = 0, ultimoLatido = 0, ultimoLog = 0;
uint32_t ultimoArranqueLidar = 0;
char mensajeDevkit[300] = "Esperando DevKitC";
char evento[96] = "Abre la pagina y espera los indicadores";
char lineaUSB[32]{};
size_t longitudUSB = 0;
bool descartarUSB = false;

const char PAGINA[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot 4WD · navegación continua</title>
<style>
body{font:16px Arial;background:#142b43;color:#fff;margin:20px auto;max-width:1100px;padding:0 16px}
h1{font-size:25px}.paneles{display:flex;gap:16px;flex-wrap:wrap}.panel{flex:1;min-width:280px}
img,canvas{width:100%;max-width:520px;background:#203c55;border-radius:12px}
button{font-size:19px;padding:14px 22px;border:0;border-radius:8px;margin:8px 10px 8px 0;cursor:pointer}
#avanzar{background:#77dec4}#parar{background:#ff6666;font-weight:bold}button:disabled{opacity:.4}
pre{white-space:pre-wrap;background:#203c55;padding:14px;border-radius:8px;overflow-wrap:anywhere}
</style></head><body>
<h1>Robot 4WD · navegación continua</h1>
<p>ARRANCAR inicia el avance sin límite de centímetros. Ante un obstáculo busca salida girando a la izquierda. STOP cancela la marcha. Si faltan datos o espacio para girar, se detiene y exige un nuevo ARRANCAR.</p>
<div class="paneles"><div class="panel"><h2>Camara OV5640</h2>
<img id="video" alt="Video de la camara"><br>
<button onclick="reconectar()">Reconectar video</button>
<a style="color:#8deadb" href="/captura" target="_blank">Captura</a></div>
<div class="panel"><h2>LiDAR · vista de 72 sectores</h2>
<canvas id="radar" width="420" height="420"></canvas>
<p>0 grados es la referencia del LiDAR. El dibujo no supone el frente del carro. Escala 3 m.</p></div></div>
<p>Configuración antes de arrancar: coloca un objeto delante del carro y pulsa su punto en el radar para elegir el ángulo frontal. Retira el objeto antes de arrancar.</p>
<label>Frente del carro (grados): <input id="frente" type="number" min="0" max="359" step="1" placeholder="Selecciona en radar"></label>
<label>Radio hasta la esquina (mm): <input id="radio" type="number" min="100" max="1000" value="150"></label>
<button onclick="configurar()">Confirmar frente y radio</button><p id="configActual">Frente sin confirmar. Radio indicado por ti: 150 mm.</p>
<button id="avanzar" disabled onclick="mandar('avanzar')">ARRANCAR</button>
<button id="parar" onclick="mandar('stop')">PARAR / STOP</button>
<pre id="estado">Esperando datos...</pre><pre id="diagnostico"></pre><pre id="devkit"></pre>
<p>Mantener esta pagina abierta. Al perder el enlace con la pagina se cancela el movimiento.</p>
<script>
const boton=document.getElementById('avanzar');
const radar=document.getElementById('radar');const ctx=radar.getContext('2d');
radar.addEventListener('click',e=>{const b=radar.getBoundingClientRect();const x=(e.clientX-b.left)/b.width*420-210,y=(e.clientY-b.top)/b.height*420-210;
 if(Math.hypot(x,y)<8)return;document.getElementById('frente').value=Math.round((Math.atan2(x,-y)*180/Math.PI+360)%360)%360});
async function configurar(){
 const f=document.getElementById('frente'),r=document.getElementById('radio');
 if(!f.value||!r.value||!f.checkValidity()||!r.checkValidity()){alert('Indica el ángulo frontal y el radio real del chasis.');return}
 try{const res=await fetch('/configurar?frente='+f.value+'&radio='+r.value+'&confirmar=1',{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('configActual').textContent=await res.text()}catch(e){}
}
function reconectar(){document.getElementById('video').src='http://'+location.hostname+':81/stream?t='+Date.now()}
async function mandar(accion){
 boton.disabled=true;
 try{const r=await fetch('/'+accion,{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('estado').textContent=await r.text()}catch(e){}
}
function dibujar(mapa,frente,configurada){
 ctx.clearRect(0,0,420,420);ctx.strokeStyle='#56738b';ctx.fillStyle='#8deadb';
 for(let r=42;r<=168;r+=42){ctx.beginPath();ctx.arc(210,210,r,0,2*Math.PI);ctx.stroke()}
 ctx.beginPath();ctx.moveTo(210,20);ctx.lineTo(210,400);ctx.moveTo(20,210);ctx.lineTo(400,210);ctx.stroke();
 ctx.font='14px Arial';ctx.fillText('0 grados',220,20);
 mapa.forEach((d,i)=>{if(d>0&&d<=3000){const a=i*5*Math.PI/180;
 const r=d/3000*180;ctx.beginPath();ctx.arc(210+Math.sin(a)*r,210-Math.cos(a)*r,3,0,2*Math.PI);ctx.fill()}});
 ctx.fillStyle='#ffcc55';ctx.fillRect(205,205,10,10);
 if(configurada){const a=frente*Math.PI/180;ctx.strokeStyle='#ffcc55';ctx.beginPath();ctx.moveTo(210,210);ctx.lineTo(210+Math.sin(a)*110,210-Math.cos(a)*110);ctx.stroke()}
}
async function actualizar(){
 try{
  const r=await fetch('/estado',{cache:'no-store'});if(!r.ok)throw Error();const d=await r.json();
  const si=v=>v?'OK':'PENDIENTE';
  document.getElementById('estado').textContent=
   'Camara: '+si(d.camara)+' | FPS enviados: '+d.fps+'\n'+
   'LiDAR: '+si(d.lidar)+' | sectores: '+d.sectores+'/72 | minimo: '+d.minimo+' mm\n'+
   'UART: '+si(d.enlace)+' | MPU: '+si(d.imu)+' | operador: '+si(d.operador)+'\n'+
   'Recorrido de ruedas (incluye giros): '+d.distancia+' cm | marcha autónoma: '+(d.activa?'ACTIVA':'PARADA')+' | motores: '+(d.moviendo?'SI':'NO')+'\n'+
   'LiDAR paquetes: '+d.paquetes+' | errores: '+d.errores+' | barridos: '+d.barridos+'\n'+d.evento;
  document.getElementById('devkit').textContent='DevKitC\n'+d.devkit;
  document.getElementById('diagnostico').textContent=
   'DIAGNOSTICO D6 | bytes: '+d.bytesLidar+' | cabeceras invalidas: '+d.erroresCabecera+
   ' | checksum invalidos: '+d.erroresChecksum+' | inicios validos: '+d.iniciosLidar+'\n'+(d.lidar?'Barridos válidos recibidos':d.diagnosticoLidar);
  boton.disabled=!(d.permiso&&!d.activa);
  document.getElementById('configActual').textContent=d.configurada?'Frente confirmado: '+d.frente+' grados | radio: '+d.radio+' mm':'Confirma el frente del carro para habilitar la marcha';
  dibujar(d.mapa,d.frente,d.configurada);
 }catch(e){boton.disabled=true;document.getElementById('estado').textContent='Sin conexion al S3. Reconecta RobotCam.'}
 setTimeout(actualizar,300);
}
reconectar();actualizar();
</script></body></html>
)HTML";

bool peticionLocal(httpd_req_t *req) {
  char valor[4];
  return httpd_req_get_hdr_value_str(req, "X-Robot", valor, sizeof(valor)) == ESP_OK &&
         strcmp(valor, "1") == 0;
}

esp_err_t paginaWeb(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, PAGINA, HTTPD_RESP_USE_STRLEN);
}

esp_err_t estadoWeb(httpd_req_t *req) {
  ultimoNavegador.store(millis());
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  char mapa[512];
  size_t usados = 0;
  for (size_t i = 0; i < 72; ++i) {
    usados += snprintf(mapa + usados, sizeof(mapa) - usados, "%s%u", i ? "," : "", copia.mapa[i]);
  }
  char json[2500];
  snprintf(json, sizeof(json),
    "{\"activa\":%u,\"configurada\":%u,\"frente\":%d,\"radio\":%d,\"permiso\":%u,\"camara\":%u,\"lidar\":%u,\"enlace\":%u,\"imu\":%u,"
    "\"moviendo\":%u,\"operador\":%u,\"minimo\":%u,\"sectores\":%u,\"paquetes\":%lu,"
    "\"errores\":%lu,\"barridos\":%lu,\"fps\":%.1f,\"distancia\":%.1f,"
    "\"bytesLidar\":%lu,\"erroresCabecera\":%lu,\"erroresChecksum\":%lu,\"iniciosLidar\":%lu,"
    "\"diagnosticoLidar\":\"%s\","
    "\"devkit\":\"%s\",\"evento\":\"%s\",\"mapa\":[%s]}",
    copia.activa, copia.configurada, copia.frente, copia.radio, copia.permiso, copia.camara, copia.lidar, copia.enlace, copia.imu, copia.moviendo,
    copia.operador, copia.minimo, copia.sectores, (unsigned long)copia.paquetes,
    (unsigned long)copia.errores, (unsigned long)copia.barridos, copia.fps, copia.distancia,
    (unsigned long)copia.bytesLidar, (unsigned long)copia.erroresCabecera,
    (unsigned long)copia.erroresChecksum, (unsigned long)copia.iniciosLidar, copia.diagnosticoLidar,
    copia.devkit, copia.evento, mapa);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

esp_err_t avanzarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  if (!copia.permiso || !copia.enlace || !copia.imu || copia.activa) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan condiciones. Revisa los indicadores.");
  }
  const uint32_t ahora = millis();
  if (xQueueSend(solicitudesInicio, &ahora, 0) != pdPASS) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ya hay una orden pendiente.");
  }
  return httpd_resp_send(req, "Orden recibida. Esperando respuesta del DevKitC.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t configurarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado); copia = estado; portEXIT_CRITICAL(&muxEstado);
  if (copia.activa || copia.moviendo) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Pulsa STOP antes de configurar");
  char query[100], f[12], r[12], c[4];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "frente", f, sizeof(f)) != ESP_OK ||
      httpd_query_key_value(query, "radio", r, sizeof(r)) != ESP_OK ||
      httpd_query_key_value(query, "confirmar", c, sizeof(c)) != ESP_OK || strcmp(c, "1") != 0)
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan frente, radio y confirmacion");
  char *finF, *finR;
  long frente = strtol(f, &finF, 10), radio = strtol(r, &finR, 10);
  if (!*f || !*r || *finF || *finR || frente < 0 || frente >= 360 || radio < 100 || radio > 1000)
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Geometria fuera de rango");
  geometria.store(0x80000000u | (uint32_t(radio) << 9) | uint32_t(frente));
  solicitudStop.store(true); // evita carreras con un ARRANCAR simultaneo.
  return httpd_resp_send(req, "Frente y radio confirmados. Retira el objeto de referencia.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t pararWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  solicitudStop.store(true);
  return httpd_resp_send(req, "STOP solicitado", HTTPD_RESP_USE_STRLEN);
}

esp_err_t capturaWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  camera_fb_t *imagen = esp_camera_fb_get();
  if (!imagen) return httpd_resp_send_500(req);
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  const esp_err_t resultado = httpd_resp_send(req, (const char *)imagen->buf, imagen->len);
  esp_camera_fb_return(imagen);
  return resultado;
}

esp_err_t videoWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  esp_err_t resultado = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=robotframe");
  if (resultado != ESP_OK) return resultado;
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  videoActivo.store(true);
  uint32_t cuadros = 0, inicioFPS = millis();
  while (true) {
    camera_fb_t *imagen = esp_camera_fb_get();
    if (!imagen) { resultado = ESP_FAIL; break; }
    char cabecera[100];
    const int n = snprintf(cabecera, sizeof(cabecera),
      "\r\n--robotframe\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)imagen->len);
    resultado = httpd_resp_send_chunk(req, cabecera, n);
    if (resultado == ESP_OK) resultado = httpd_resp_send_chunk(req, (const char *)imagen->buf, imagen->len);
    esp_camera_fb_return(imagen);
    if (resultado != ESP_OK) break;
    ultimoFrame.store(millis());
    ++cuadros;
    const uint32_t transcurrido = millis() - inicioFPS;
    if (transcurrido >= 3000) {
      fpsDecimas.store(cuadros * 10000UL / transcurrido);
      inicioFPS = millis();
      cuadros = 0;
    }
    delay(1);
  }
  videoActivo.store(false);
  fpsDecimas.store(0);
  return resultado;
}

bool registrarRuta(httpd_handle_t servidor, const char *uri, httpd_method_t metodo,
                   esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t ruta = {};
  ruta.uri = uri;
  ruta.method = metodo;
  ruta.handler = handler;
  return httpd_register_uri_handler(servidor, &ruta) == ESP_OK;
}

bool iniciarServidores() {
  httpd_config_t web = HTTPD_DEFAULT_CONFIG();
  web.server_port = 80;
  web.stack_size = 8192;
  web.send_wait_timeout = 2;
  if (httpd_start(&servidorWeb, &web) != ESP_OK) return false;
  bool bien = registrarRuta(servidorWeb, "/", HTTP_GET, paginaWeb) &&
    registrarRuta(servidorWeb, "/estado", HTTP_GET, estadoWeb) &&
    registrarRuta(servidorWeb, "/avanzar", HTTP_POST, avanzarWeb) &&
    registrarRuta(servidorWeb, "/configurar", HTTP_POST, configurarWeb) &&
    registrarRuta(servidorWeb, "/stop", HTTP_POST, pararWeb) &&
    registrarRuta(servidorWeb, "/captura", HTTP_GET, capturaWeb);
  httpd_config_t video = HTTPD_DEFAULT_CONFIG();
  video.server_port = 81;
  video.ctrl_port = web.ctrl_port + 1;
  video.stack_size = 8192;
  video.send_wait_timeout = 2;
  bien = bien && httpd_start(&servidorVideo, &video) == ESP_OK &&
         registrarRuta(servidorVideo, "/stream", HTTP_GET, videoWeb);
  if (!bien) {
    if (servidorVideo) httpd_stop(servidorVideo);
    httpd_stop(servidorWeb);
  }
  return bien;
}

bool iniciarCamara() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11; config.pin_d1 = 9; config.pin_d2 = 8; config.pin_d3 = 10;
  config.pin_d4 = 12; config.pin_d5 = 18; config.pin_d6 = 17; config.pin_d7 = 16;
  config.pin_xclk = 15; config.pin_pclk = 13; config.pin_vsync = 6; config.pin_href = 7;
  config.pin_sccb_sda = 4; config.pin_sccb_scl = 5;
  config.pin_pwdn = -1; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 16;
  const bool psram = psramFound();
  config.fb_count = psram ? 2 : 1;
  config.fb_location = psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = psram ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  Serial.printf("PSRAM=%s | buferes=%d\n", psram ? "SI" : "NO", config.fb_count);
  const esp_err_t resultado = esp_camera_init(&config);
  Serial.printf("CAMARA init=0x%X\n", (unsigned)resultado);
  return resultado == ESP_OK;
}

void procesarDevkit(const char *texto) {
  unsigned long recibida, activa, bloqueada;
  unsigned int moviendo, imu;
  float distancia; char causa[48];
  if (sscanf(texto, "A1 %lu %lu %lu %u %u %f %47s", &recibida, &activa, &bloqueada,
      &moviendo, &imu, &distancia, causa) != 7 || recibida != sesion ||
      moviendo > 1 || imu > 1 || !isfinite(distancia) || distancia < 0) return;
  for (const char *p = texto; *p; ++p) {
    if (*p == '"' || *p == '\\' || static_cast<unsigned char>(*p) < 32) return;
  }
  devkitRecibido = true; ultimoDevkit = millis(); devkitMoviendo = moviendo;
  devkitIMU = imu; distanciaDevkit = distancia; bloqueadaDevkit = bloqueada;
  snprintf(mensajeDevkit, sizeof(mensajeDevkit), "%s", texto);
}

void leerUSB() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineaUSB[longitudUSB] = '\0';
      if (!descartarUSB) {
        if (strcmp(lineaUSB, "STOP") == 0) solicitudStop.store(true);
        else if (strcmp(lineaUSB, "ARRANCAR") == 0) {
          const uint32_t ahora = millis();
          xQueueSend(solicitudesInicio, &ahora, 0);
        } else Serial.println("Comandos: ARRANCAR | STOP. Mantener pagina con video abierta.");
      }
      longitudUSB = 0;
      descartarUSB = false;
    } else if (longitudUSB < sizeof(lineaUSB) - 1) lineaUSB[longitudUSB++] = c;
    else descartarUSB = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  sesion = esp_random();
  if (!sesion) sesion = 1;
  solicitudesInicio = xQueueCreate(1, sizeof(uint32_t));
  if (!solicitudesInicio) { Serial.println("ERROR memoria para cola de ordenes"); return; }
  enlace.setRxBufferSize(2048);
  enlace.begin(115200, SERIAL_8N1, 42, 41);
  enviarRobot(enlace, "STOP");
  puertoLidar.setRxBufferSize(8192);
  puertoLidar.begin(230400, SERIAL_8N1, 1, 2);
  camaraIniciada = iniciarCamara();
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("RobotCam", "CAMBIA_CLAVE_ROBOTCAM", 1, false, 1)) {
    Serial.println("ERROR WIFI");
    return;
  }
  servidoresListos = iniciarServidores();
  Serial.printf("AUTONOMO S3 V1 | camara=%u web=%u | LiDAR RX1 TX2\n", camaraIniciada, servidoresListos);
  Serial.print("Pagina http://"); Serial.println(WiFi.softAPIP());
  Serial.println("Wi-Fi RobotCam | clave CAMBIA_CLAVE_ROBOTCAM | motores parados hasta nueva orden");
  const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
  puertoLidar.write(iniciar, sizeof(iniciar));
  ultimoArranqueLidar = millis();
}

void loop() {
  if (!solicitudesInicio) { delay(100); return; }
  for (size_t i = 0; i < 4096 && puertoLidar.available(); ++i) {
    lidar.recibir(puertoLidar.read(), millis());
  }
  for (size_t i = 0; i < 1024 && enlace.available(); ++i) {
    const char *texto = receptor.recibir(enlace.read());
    if (texto) procesarDevkit(texto);
  }
  leerUSB();
  const uint32_t ahora = millis();
  if (lidar.paquetes == 0 && ahora - ultimoArranqueLidar > 4000) {
    const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
    puertoLidar.write(iniciar, sizeof(iniciar));
    ultimoArranqueLidar = ahora;
  }
  // Leer primero los sellos escritos por las tareas HTTP y despues el reloj;
  // evita una resta unsigned negativa si llega un frame durante esta iteracion.
  const uint32_t frame = ultimoFrame.load(), navegador = ultimoNavegador.load();
  const uint32_t ahoraSalud = millis();
  const bool video = camaraIniciada && videoActivo.load() && frame && ahoraSalud - frame < 1000;
  const bool operador = navegador && ahoraSalud - navegador < 1200;
  const bool sensor = lidar.vigente(ahora);
  const bool devkit = devkitRecibido && ahora - ultimoDevkit <= 600;
  const ConfigNavegacion config = configuracion();
  const bool salud = servidoresListos && video && operador && sensor && devkit && devkitIMU;
  const char *obstaculo = config.valida() ? navegacion.evaluar(lidar.distancias, config) : "Confirma frente y radio del chasis";
  const char *giro = config.valida() ? navegacion.evaluar(lidar.distancias, config, true) : obstaculo;
  const bool permiso = salud && config.valida() && (!obstaculo || !giro);
  const bool estabaActiva = navegacion.activa();
  if (solicitudStop.exchange(false)) {
    xQueueReset(solicitudesInicio);
    navegacion.parar(); corrida = 0; enviarRobot(enlace, "STOP");
  } else {
    if (navegacion.activa() && (!salud || bloqueadaDevkit == corrida)) {
      navegacion.fallar(bloqueadaDevkit == corrida ? "DevKit detuvo la corrida: revisa su motivo" :
        !sensor ? "LiDAR sin barrido valido" : !devkit ? "Sin telemetria del DevKit" :
        !devkitIMU ? "MPU sin datos" : !operador ? "Navegador desconectado" : "Video o servidor no disponible");
      xQueueReset(solicitudesInicio);
    }
    uint32_t solicitada;
    if (xQueueReceive(solicitudesInicio, &solicitada, 0) == pdPASS && !navegacion.activa()) {
      if (ahora - solicitada <= 500 && permiso) {
        if (++orden == 0) ++orden;
        corrida = orden; navegacion.arrancar(config, ahora);
      }
    }
    navegacion.actualizar(lidar.distancias, config, sensor, lidar.barridos, ahora);
    if (!navegacion.activa()) corrida = 0;
    if (estabaActiva && !navegacion.activa()) enviarRobot(enlace, "STOP");
  }
  if (navegacion.activa()) snprintf(evento, sizeof(evento), "%s", navegacion.motivo);
  else if (navegacion.fase == Navegacion::FALLO) snprintf(evento, sizeof(evento), "%s. Pulsa ARRANCAR para reintentar.", navegacion.motivo);
  else snprintf(evento, sizeof(evento), "%s", !config.valida() ? "Confirma frente y radio del chasis" :
      !sensor ? "Esperando barridos validos del LiDAR" : !devkit ? "Esperando DevKit con firmware autonomo V1" :
      !devkitIMU ? "MPU sin datos" : !operador ? "Esperando navegador" : !video ? "Esperando video" :
      obstaculo && giro ? giro : navegacion.motivo);
  if (ahora - ultimoLatido >= 100) {
    ultimoLatido = ahora;
    char mensaje[140];
    snprintf(mensaje, sizeof(mensaje), "V1 %lu %lu %lu %d %d %d %d",
      (unsigned long)sesion, (unsigned long)++secuencia, (unsigned long)corrida,
      navegacion.izquierda, navegacion.izquierda, navegacion.derecha, navegacion.derecha);
    enviarRobot(enlace, mensaje);
  }
  portENTER_CRITICAL(&muxEstado);
  estado.activa = navegacion.activa();
  estado.configurada = config.valida(); estado.frente = config.frente; estado.radio = config.radio;
  estado.permiso = permiso;
  estado.camara = video;
  estado.lidar = sensor;
  estado.enlace = devkit;
  estado.imu = devkit && devkitIMU;
  estado.moviendo = devkit && devkitMoviendo;
  estado.operador = operador;
  estado.minimo = lidar.minimo;
  estado.sectores = lidar.cobertura;
  estado.paquetes = lidar.paquetes;
  estado.bytesLidar = lidar.bytesRecibidos;
  estado.erroresCabecera = lidar.erroresCabecera;
  estado.erroresChecksum = lidar.erroresChecksum;
  estado.iniciosLidar = lidar.inicios;
  memcpy(estado.diagnosticoLidar, lidar.diagnostico, sizeof(estado.diagnosticoLidar));
  estado.errores = lidar.errores;
  estado.barridos = lidar.barridos;
  estado.fps = fpsDecimas.load() / 10.0f;
  estado.distancia = distanciaDevkit;
  memcpy(estado.mapa, lidar.distancias, sizeof(estado.mapa));
  memcpy(estado.devkit, mensajeDevkit, sizeof(estado.devkit));
  memcpy(estado.evento, evento, sizeof(estado.evento));
  portEXIT_CRITICAL(&muxEstado);
  if (ahora - ultimoLog >= 2000) {
    ultimoLog = ahora;
    Serial.printf("ESTADO | cam=%u lidar=%u uart=%u imu=%u permiso=%u | min=%u mm | sectores=%u/72 | paquetes=%lu errores=%lu\n",
      video, sensor, devkit, devkitIMU, permiso, lidar.minimo, lidar.cobertura,
      (unsigned long)lidar.paquetes, (unsigned long)lidar.errores);
  }
  delay(1);
}
```

### `respaldo_autonomia_antes_reanudacion_2026-11-24/SHA256.txt`

```text
16F7AC603B55EBCD052545659E55B9F64C093930DC79D42CFD4FD1070CCE0F9B  LEEME.md
FDBADE6494661BF15CFB7DA1B7586330A8001BCFF62639267DA087C1577EA401  devkit_integral\devkit_integral.ino
C90B940B9E9032B232D8A4212F3786C72321A66CD426A6A954CE8C87AC178A24  devkit_integral\EnlaceRobot.h
387BEDD6033B4FF52F3AA7C7877DC47EDE9C203041EE3E23C4DA13D7D4CEA3A0  devkit_integral\GuardiaMarcha.h
1C440E876BD358C0680A3CCB9846D0D9A6FB2F634F8C852F7CF345C8CD48E7BF  pruebas\interfaz_test.cjs
3771627CEAED3DF9DA1E65A20A68979787A046CD53D2219311F2C3BCAF32A993  pruebas\motores_test.cpp
35E90933CEB40C85F97922CFB77528969E8FA77212FBCDB3EE381EA6FE26D7EB  pruebas\navegacion_test.cpp
68190E0970F0A2D3E64CAE069C257E8F1019FF9CC0B5554F7F5C9508176B6FA4  pruebas\protocolo_test.cpp
5D00191302BDD86C0AA36D70EF4E3CA7F7D25CF4F70B6AE4E99F0C5EAE6AC699  pruebas\dobles\Arduino.h
A6693E4BA03EBDFA60D152C674B92EC2EA877AB27A50423D2E45A376441318B7  pruebas\dobles\Wire.h
C90B940B9E9032B232D8A4212F3786C72321A66CD426A6A954CE8C87AC178A24  s3_integral\EnlaceRobot.h
0E44935939FD49D09F07B0640A5566D8CFE7F4FE3F9AB948C59C5BD28738AAD4  s3_integral\EstadoIntegral.h
F173D33189FEEECF20F6527CE96A81CB8B31768BBD2F57F5708101021B51B541  s3_integral\LidarD6.h
4EFE9CAE00C2A08E9784C2B17F4B2B5120EED7E2CD16E4660809AED0C52597E5  s3_integral\Navegacion.h
224DE28AD744BB247ED839FCDC25C2ABE65489DD3397BBAB963F3D9F3528B9CF  s3_integral\s3_integral.ino
```

### `primera_prueba_exitosa_2026-11-14/devkit_integral/devkit_integral.ino`

```cpp
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
```

### `primera_prueba_exitosa_2026-11-14/devkit_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `primera_prueba_exitosa_2026-11-14/LEEME.md`

````markdown
# Prueba integral: cámara + COIN-D6 + dos ESP32 + avance de 50 cm

Preparado el 14 de noviembre de 2026, revisando este chat, su conversación anterior «Analiza video y material de clase (2)» y los programas del proyecto. **Programas compilados; la integración completa de este paquete todavía debe probarse en el robot.** No se cargó firmware automáticamente.

## 1. Qué hace cada componente

| Componente | Función en esta prueba |
|---|---|
| ESP32-S3 | Video MJPEG, lectura UART del D6, página de control, órdenes y recepción de telemetría del DevKitC |
| Cámara OV5640 | Video en directo; si deja de enviarse video, se cancela el permiso de movimiento |
| COIN-D6 | Barrido y distancia mínima: bloquea/cancela movimiento si detecta menos de 250 mm en cualquier dirección |
| ESP32 DevKitC V4 / WROOM-32D | Cuatro motores, cuatro encoders, MPU6050 y parada local por pérdida de órdenes |
| Dos TB6612FNG | Un canal por motor, STBY común |
| Cuatro LM393 | Conteo de pulsos y estimación de distancia por rueda |
| MPU6050 | Lectura de aceleración y giro Z; si falla, se bloquea/cancela el avance |
| XL4016 | Salida regulada de 5,0 V, decisión recuperada del montaje anterior |

La cámara aporta supervisión visual; no se ejecuta reconocimiento de objetos. La MPU aporta telemetría y comprobación de comunicación, no corrección de trayectoria. No se implementan PID, SLAM, Nav2 ni navegación autónoma. Los encoders de un canal no determinan por sí mismos la dirección real. Los condensadores, interruptor y fusible son elementos eléctricos, no dispositivos programables. No es necesario introducir el LM2596 en este montaje si ya se adoptó el XL4016 único.

## 2. Archivos que debes cargar

1. DevKitC: abrir `devkit_integral/devkit_integral.ino`. Mantener `EnlaceRobot.h` en la misma carpeta.
2. S3: abrir `s3_integral/s3_integral.ino`. Mantener `EnlaceRobot.h`, `LidarD6.h` y `EstadoIntegral.h` en la misma carpeta.

Extraer TODO el ZIP antes de abrir los `.ino`. No pegar ambos programas en el mismo sketch. No hace falta instalar bibliotecas adicionales: se usan las incluidas en Arduino-ESP32.

## 3. Conexiones del LiDAR al S3: nombres y colores

Hacer los cambios con la alimentación apagada. El adaptador USB CH340 se retira del circuito del LiDAR para esta prueba; no conectar simultáneamente sus señales a las del S3.

| Cable del D6 mostrado en las fotos | Nombre / dirección | Conectar a |
|---|---|---|
| **Amarillo** | TXD: datos que salen del LiDAR | **GPIO1 del S3**, RX del puerto del LiDAR |
| **Verde** | RXD: órdenes que entran al LiDAR | **GPIO2 del S3**, TX del puerto del LiDAR |
| **Negro** | GND | Distribución GND común |
| **Rojo** | VCC | **5,0 V regulados del XL4016** |

Esta correspondencia se recuperó del pinout y el montaje del chat anterior; los colores no son una norma universal para otros cables. UART D6: **230400 baudios, 8N1, lógica de 3,3 V**. Los 5 V son alimentación, nunca una señal GPIO. COM9 era el CH340 del PC: no es un puerto que se configure dentro del firmware del S3.

## 4. UART entre los dos ESP32

| Origen | Destino | Función |
|---|---|---|
| S3 GPIO41 TX | DevKitC GPIO16 RX | Permiso, avance y STOP |
| DevKitC GPIO17 TX | S3 GPIO42 RX | Distancia, pulsos, MPU y estado |
| GND S3 | GND común DevKitC | Referencia eléctrica |

**115200 baudios, 8N1.** No unir los pines 3V3 de ambas placas. Los colores de estos tres puentes no quedaron confirmados en la evidencia disponible: identificarlos por sus extremos, no inventar un color.

## 5. Motores y encoders: mapa conservado

FL = delantero izquierdo; RL = trasero izquierdo; FR = delantero derecho; RR = trasero derecho.

| Rueda / canal | IN1 DevKitC | IN2 DevKitC | PWM DevKitC | Encoder DO/OUT DevKitC | Inversión en programa |
|---|---:|---:|---:|---:|---|
| FL / driver izquierdo A | 13 (AIN1) | 14 (AIN2) | 25 (PWMA) | 34 | Sí |
| RL / driver izquierdo B | 26 (BIN1) | 27 (BIN2) | 32 (PWMB) | 35 | No |
| FR / driver derecho A | 18 (AIN1) | 19 (AIN2) | 23 (PWMA) | 36 / VP | Sí |
| RR / driver derecho B | 4 (BIN1) | 15 (BIN2) | 33 (PWMB) | 39 / VN | No |

- STBY de ambos TB6612FNG → GPIO2 **del DevKitC**. Es otra placa distinta del GPIO2 del S3 usado para LiDAR.
- VCC lógico de ambos drivers → 3V3 del DevKitC.
- VM de ambos drivers → rama de motores a 5,0 V adoptada en el montaje; GND a distribución común.
- Canal A: AO1/AO2 al motor delantero; canal B: BO1/BO2 al trasero de ese lado. En el mapa previo aparecen pares rojo/negro de motor; la inversión de software conserva el sentido ya probado, no cambiar polaridad por el color.
- Cada encoder: DO/OUT a la entrada de la tabla; GND común. Conservar la alimentación del montaje ya probado y comprobar que DO no exceda 3,3 V. No alimentar a 5 V por deducción del nombre LM393. AO no se usa.
- GPIO34, 35, 36 y 39 no tienen resistencias pull-up internas; el firmware usa INPUT y depende de la salida/pull-up físico ya instalado.
- No hay registro fiable del color de cada puente IN/PWM/OUT: la tabla de GPIO es la referencia.

## 6. MPU6050 en el DevKitC

| Pin MPU | Conectar a |
|---|---|
| VCC | 3V3, según montaje que funcionó en la sesión |
| GND | GND común verificado; anteriormente un contacto de tierra falló |
| SDA | GPIO21 DevKitC |
| SCL | GPIO22 DevKitC |
| AD0 | Conservar montaje; el programa busca 0x68 y 0x69 |
| INT, XDA, XCL | No usados por este programa |

I2C a 100 kHz, lectura cada 50 ms. Se valida WHO_AM_I. Se reportan ax, ay, az en g y gz en grados/s. No se afirma que exista calibración de sesgo ni orientación absoluta. Si no detecta la MPU al arrancar, corregir su conexión y reiniciar; no cambiarla a 5 V como solución automática.

## 7. Cámara en el S3

| Señal | GPIO S3 |
|---|---:|
| D0, D1, D2, D3 | 11, 9, 8, 10 |
| D4, D5, D6, D7 | 12, 18, 17, 16 |
| XCLK / PCLK | 15 / 13 |
| VSYNC / HREF | 6 / 7 |
| SCCB SDA / SCL | 4 / 5 |
| PWDN / RESET | -1 / -1 |

Son conexiones del módulo de cámara existente. No se asignan estos GPIO también a sensores del S3. Video QVGA JPEG, calidad 16 y XCLK 20 MHz. Si detecta PSRAM usa dos buffers; si no, usa un buffer en DRAM. Se elimina el bloqueo anterior «PSRAM no detectada»: este programa puede continuar sin PSRAM. Los FPS se muestran como imágenes enviadas por segundo, sin prometer una tasa determinada.

## 8. Alimentación y carga de los programas

Decisión del chat anterior: XL4016 ajustado con multímetro a **5,0 V**, con ramas en paralelo hacia VM de ambos drivers, entradas **5V** de ambas placas y rojo VCC del D6. La lógica TB6612 permanece a **3V3 del DevKitC**. GND común con retornos de motores separados del cableado fino de lógica. No conectar batería directa a GPIO/3V3. Conservar interruptor, fusible y desacoplo del montaje; no hay evidencia suficiente para inventar valores de fusible o condensadores.

Para programar por USB, apagar la alimentación externa y **aislar sus positivos de 5 V de las placas** antes de enchufar USB, salvo que ya exista una protección de alimentación documentada. Evitar alimentar una placa apagada a través de TX/RX: al cargar cada placa por separado desconectar temporalmente el enlace UART entre placas. No mantener alimentación externa y USB unidos por sus positivos sin aislamiento verificado.

Una vez cargadas ambas placas: desconectar USB, apagar todo, restaurar UART y alimentación externa y ejecutar desde navegador. Así no necesitas USB durante el movimiento. Verificar que los 5 V no caigan durante el arranque simultáneo de motores y LiDAR; este software no puede corregir una caída de alimentación.

## 9. Carga paso a paso en Arduino IDE

### A. ESP32 DevKitC

1. Abrir `devkit_integral.ino`.
2. Placa: **ESP32 Dev Module**. Puerto correspondiente al DevKitC, históricamente COM5; comprobar el actual.
3. Flash Mode: DIO. El paquete se verificó con **Arduino-ESP32 3.3.12**.
4. Pulsar Verificar y Subir. Si aparece el antiguo error de modo de arranque, sostener BOOT, pulsar y soltar EN, iniciar/concluir conexión de carga y soltar BOOT cuando comience la escritura. No borrar flash como primer remedio.
5. Monitor a 115200 si lo necesitas durante programación. El arranque deja los motores deshabilitados.

### B. ESP32-S3

1. Abrir `s3_integral.ino`.
2. Placa **ESP32S3 Dev Module**; puerto del S3, históricamente COM6.
3. Flash Size **16MB**; PSRAM **OPI PSRAM**; USB CDC On Boot **Disabled** usando el USB-UART que se venía usando.
4. Partition Scheme: opción **16M con APP de 3MB y FATFS de 9MB** (`app3M_fat9M_16MB`).
5. Verificar y subir. Monitor 115200. Si PSRAM sigue sin detectarse, el firmware intenta cámara en DRAM y lo informa; no queda bloqueado por ese motivo.

Los puertos COM pueden cambiar al reconectar. No seleccionar COM9 del CH340 como si fuera una de estas placas.

## 10. Ejecutar la prueba completa

1. Con todo apagado, comprobar las conexiones anteriores y restaurar alimentación externa sin USB. Fijar la MPU y el LiDAR; que las ruedas, cables y chasis no tapen el plano de barrido.
2. Para el primer arranque de este firmware mantener las ruedas elevadas y despejadas, sin tocar la parte giratoria del LiDAR. Encender: los motores no deben arrancar automáticamente. Confirmar primero que STOP y estados responden. Apagar antes de bajar el robot.
3. En el suelo, preparar al menos 50 cm de recorrido más margen de parada, con espacio libre alrededor. La regla LiDAR es de **25 cm en los 360°**, no solo por delante; también una mano cerca por detrás puede bloquearlo. La distancia se mide desde el sensor, no desde el borde del chasis; no constituye una garantía anticolisión.
4. Conectar el teléfono/PC a Wi-Fi **RobotCam**, contraseña **CAMBIA_CLAVE_ROBOTCAM**. Aceptar permanecer en esa red aunque no tenga internet.
5. Abrir **http://192.168.4.1/**. Debe aparecer video y radar. Mantener esa página en primer plano; el AP admite un cliente Wi-Fi para esta prueba.
6. Esperar Cámara OK, LiDAR OK, UART OK y MPU OK. Si falta cualquiera, AVANZA permanece bloqueado. No anular el bloqueo para forzar la prueba.
7. Pulsar **AVANZA 50 cm una sola vez**. Los cuatro motores reciben PWM 80. Se muestra distancia estimada, pulsos y motivo del DevKitC. Mantener acceso al interruptor físico y no poner las manos delante del sensor durante el recorrido.
8. Debe detenerse con **FIN_50CM** cuando el promedio de las cuatro distancias alcance 50 cm. Medir con regla la distancia real. Un final por obstáculo, pérdida de enlace o tiempo no es una prueba de 50 cm completada.
9. Registrar distancia real, pulsos FL/RL/FR/RR, motivo final, estabilidad del video y mínimos LiDAR. No hay reanudación automática: tras una parada se requiere una orden nueva.

Los factores se reconstruyeron de los últimos registros aceptados de 30 cm: FL=13,2/27, RL=13,2/25, FR=13,2/26, RR=13,2/24 cm/pulso. **No son una nueva calibración validada de 50 cm.** No se reutilizaron los coeficientes antiguos incompatibles de `pruebas_distancia`. Deslizamiento, diferencias entre ruedas e inercia pueden cambiar el desplazamiento real; este programa no garantiza rectitud ni exactitud de 50,0 cm.

## 11. Paradas y condiciones de permiso

- Arranque y reinicio: motores parados; no arranque automático.
- STOP desde la página: orden UART al DevKitC, deshabilita STBY y PWM.
- LiDAR: checksum XOR, paquetes y barridos; un barrido válido requiere al menos 60 de 72 sectores con retorno válido, 100 puntos y duración de 40–300 ms. Se considera vencido tras 500 ms. Retornos cero no se interpretan como espacio libre. En entornos con pocos retornos puede bloquear aunque no haya un obstáculo: revisar cobertura, no forzar un permiso.
- Distancia mínima LiDAR inferior a 250 mm: se cancela el permiso. Se usa el mínimo de todo el barrido; no hace falta inventar cuál es el frente del sensor.
- DevKitC: si no recibe latido válido en 450 ms, se detiene. El S3 envía cada 100 ms.
- El S3 exige telemetría de retorno del DevKitC reciente (600 ms) y MPU válida: se comprueba también el sentido DevKitC→S3.
- MPU: lecturas válidas y recientes; fallo durante marcha causa parada local.
- Encoder: si cualquiera no cambia durante 1500 ms, parada. No detecta todos los casos de ruido o deslizamiento.
- Tiempo máximo de marcha: 8 s. Si vence antes de 50 cm, se detiene; no aumentar este límite sin identificar la causa.
- Video sin imágenes enviadas durante 1 s o página sin consultar estado durante 1,2 s: cancelación del permiso. Los plazos se suman al procesamiento y comunicación; no son latencias de parada física certificadas.

La orden manual no convierte este sistema en un dispositivo de seguridad. La desconexión del controlador web puede impedir entregar STOP; por eso existe además el vencimiento local de permiso y el interruptor físico.

## 12. Lectura de telemetría y problemas

### Corrección D6 V3 SDK — checksum y recepción en S3

La captura posterior del diagnóstico V2 mostró 483267 bytes, cero errores de
cabecera, 5985 errores de checksum y solo 25 paquetes aceptados. El SDK ROS 2
proporcionado en la carpeta del fabricante calcula el XOR de cada muestra como
`Si_L ^ (Si_2nd | (Si_H << 8))`. El dibujo del manual V1.0 muestra otra agrupación,
que fue la implementada inicialmente. Se corrigió `LidarD6.h` según el SDK,
archivo `m1ct_d2/src/lidar_data_processing.cpp`, líneas 571–580, del paquete
[m1ct_d4_ros2_v16.zip](https://drive.google.com/file/d/1j1spco3obOxyYVIh0IL4nm6542XacO9O/view).
Solo se acepta el checksum del SDK; no se deshabilita la validación.

La página muestra ahora `DIAGNOSTICO D6 V3 SDK`. Se agregó un vector fijo
con checksum 0x5AFC y distancia 946 mm, independiente del generador de paquetes,
y se comprueba el rechazo de 0x9F39 calculado con la agrupación anterior.
Falta comprobar esta corrección con el LiDAR físico tras cargar el S3.

Tras conectar amarillo a GPIO1 y verde a GPIO2, la captura del usuario mostró
125 paquetes aceptados, 30732 errores y cero barridos. Confirma recepción de
algunas tramas aceptadas, pero no validaba un barrido. El diagnóstico V2 permitió
localizar el rechazo en el checksum y contrastarlo posteriormente con el SDK.

La página ahora incluye `DIAGNOSTICO D6 V2`, con bytes recibidos, errores de
cabecera, errores de checksum, inicios válidos y una muestra hexadecimal de
la última trama rechazada muestreada (como máximo una actualización por segundo).
Solo hace falta volver a cargar el sketch del S3 con sus tres encabezados.
Tras restablecer alimentación y abrir la página, esperar diez segundos y
guardar una captura del diagnóstico. Los contadores son acumulados desde
el arranque. Esta actualización agrega evidencia para el diagnóstico y mantiene
las condiciones de permiso de movimiento. Tras la corrección V3 todavía hace
falta verificar barridos, cobertura y medidas físicas antes de aprobar la integración.

La línea del DevKitC tiene este orden:

```text
D sesion moviendo imu distancia_cm pulsos_FL pulsos_RL pulsos_FR pulsos_RR ax_g ay_g az_g gz_grados_s motivo
```

| Síntoma / motivo | Revisar |
|---|---|
| Cámara PENDIENTE | Video cargado, alimentación, conector cámara, botón Reconectar video; el movimiento exige stream activo |
| LiDAR paquetes=0 | Amarillo→S3 GPIO1, verde→GPIO2, alimentación 5 V, GND, CH340 retirado, D6 girando |
| Hay paquetes pero LiDAR PENDIENTE | Sectores, retornos, checksum, plano despejado y entorno; no basta que gire |
| Muchos errores LiDAR | Nivel lógico, cables, GND, alimentación/ruido de motores; no cambiar baudios al azar |
| Mínimo <250 mm | Objetos, manos o partes del propio montaje en el barrido; no reducir umbral solo para habilitar |
| UART PENDIENTE / SIN_ENLACE_S3 | TX/RX cruzados, GND, ambos firmwares nuevos; el antiguo HOLA/RECIBIDO no sirve con este protocolo |
| MPU PENDIENTE / MPU_SIN_DATOS | VCC3V3, contacto GND, SDA21/SCL22; corregir y reiniciar |
| S3_SIN_PERMISO | Ver indicadores de cámara, operador, LiDAR, UART y MPU en la página |
| ENCODER_SIN_PULSOS | Identificar contador inmóvil, DO y alimentación compatible, disco/rueda, motor trabado |
| LIMITE_TIEMPO | Carga de motores, energía, pulsos y distancia; la prueba no terminó 50 cm |
| Reinicios / flash read err | Alimentación y conexiones de arranque antes de atribuirlo al programa |
| Una rueda gira al revés | Apagar; cotejar mapa y montaje aceptado. No probar repetidamente en suelo |

## 13. Protocolo y decisiones técnicas

Entre ESP32: tramas ASCII `@contenido*CRC16\n`, CRC16-CCITT-FALSE. Mensajes S de estado/permiso, GO con identificador de sesión y orden, STOP y D de telemetría. Las órdenes repetidas se descartan y el cambio de sesión cancela el movimiento. El CRC detecta corrupción accidental; no es autenticación criptográfica.

El D6 usa UART 230400 y paquetes AA55; orden de inicio AA55F00F. Se valida longitud, ángulos, checksum XOR y cobertura antes de permitir marcha. Se dibujan mínimos por 72 sectores de 5°, no una nube de puntos completa ni un mapa SLAM.

Se separan servidor HTTP de control (80), servidor MJPEG (81) y bucle de sensores/UART. El stream no ocupa el bucle que vigila el LiDAR. Se conserva resolución pequeña para reducir tráfico y evitar el refresco antiguo de una foto cada segundo.

Referencias originales consultadas:

- [Ficha/pinout COIN-D6](https://drive.google.com/file/d/1j-oi7kPCEAPHREYggjbOy9C-KsARW4Vk/view).
- [Protocolo COIN-D6](https://drive.google.com/file/d/11bBGx5RCkZnwQlYUY3u3yagJFsP3wJo0/view).
- [Ejemplo CameraWebServer de Espressif 3.3.12](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/ESP32/examples/Camera/CameraWebServer/app_httpd.cpp).

## 14. Verificación realizada y límites

- Compilación real de los dos sketches con Arduino-ESP32 3.3.12, placas y opciones indicadas arriba.
- Prueba C++ de los encabezados realmente usados: vector CRC conocido, rechazo de trama corrupta, resincronización, exceso de longitud, checksum D6, recuperación tras corrupción, cobertura, retornos cero, caducidad y cruce de ángulo 360°.
- Archivo reproducible: `pruebas/protocolo_test.cpp`, compilable con g++ C++17; no es un tercer firmware.
- Pendiente en hardware: ejecutar estos dos programas juntos, medir recorrido real, verificar sentido de giro, caídas de tensión, ruido con motores, funcionamiento del bloqueo LiDAR y respuesta efectiva a STOP/pérdidas.
- En la conversación anterior el usuario confirmó LiDAR funcionando con CH340/CSPC (~9,9 Hz), montaje del sensor, alimentación XL4016 a 5 V y pruebas anteriores. Eso no equivale a haber validado este nuevo parser en el S3 ni este firmware integral.

Los documentos antiguos con «D6 sin identificar», «VM pendiente» o «MPU no detectada» reflejan etapas anteriores. Este paquete recoge la evolución del chat, sin declarar completadas las comprobaciones nuevas.
````

### `primera_prueba_exitosa_2026-11-14/pruebas/protocolo_test.cpp`

```cpp
#include "../s3_integral/LidarD6.h"
#include "../s3_integral/EnlaceRobot.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

std::vector<uint8_t> paquete(bool inicio, unsigned n, float a, float b, unsigned mm) {
  std::vector<uint8_t> p(10 + 3*n);
  auto poner=[&](unsigned i,unsigned v){p[i]=v&255;p[i+1]=v>>8;};
  p[0]=0xAA;p[1]=0x55;p[2]=inicio?21:20;p[3]=n;
  poner(4,unsigned(a*128)+1);poner(6,unsigned(b*128)+1);
  for(unsigned i=0;i<n;++i){p[10+3*i]=0;p[11+3*i]=(mm&63)<<2;p[12+3*i]=mm>>6;}
  unsigned cs=0;
  // SDK: cabecera 16 bits; cada muestra usa 8 bits y luego 16 bits LE.
  for(unsigned i=0;i<8;i+=2)cs^=p[i]|(p[i+1]<<8);
  for(unsigned i=10;i<p.size();i+=3)cs^=p[i]^(p[i+1]|(p[i+2]<<8));
  poner(8,cs);return p;
}
void dar(LidarD6 &l,const std::vector<uint8_t>&p,unsigned t){for(auto b:p)l.recibir(b,t);}
int main(){
  // Vector fijo independiente del generador: XOR SDK=0x5AFC, distancia=946 mm.
  // El agrupamiento anterior produce 0x9F39 y debe rechazar la trama.
  const std::vector<uint8_t> vectorSDK = {
    0xAA,0x55,0x15,0x01,0x01,0x00,0x01,0x00,0xFC,0x5A,0x88,0xCB,0x0E
  };
  LidarD6 referencia;
  dar(referencia,vectorSDK,0);
  dar(referencia,vectorSDK,100);
  assert(referencia.paquetes==2 && referencia.inicios==2 && referencia.errores==0);
  assert(referencia.distancias[0]==946);
  auto checksumAnterior=vectorSDK;
  checksumAnterior[8]=0x39;checksumAnterior[9]=0x9F;
  dar(referencia,checksumAnterior,120);
  assert(referencia.paquetes==2 && referencia.erroresChecksum==1);
  assert(crcRobot("123456789",9)==0x29B1);
  ReceptorRobot r;
  auto recibir=[&](std::string s){std::string salida;for(char c:s){auto p=r.recibir(c);if(p)salida=p;}return salida;};
  assert(recibir("ruido@123456789*29B1\n")=="123456789");
  assert(recibir("@123456788*29B1\n").empty());
  assert(recibir("@incompleto@123456789*29B1\r\n")=="123456789");
  assert(recibir("@"+std::string(500,'x')+"\n").empty());
  assert(recibir("@123456789*29B1\n")=="123456789");
  LidarD6 l;
  dar(l,paquete(true,1,0,0,800),0);
  dar(l,paquete(false,144,0,357.5,800),20);
  dar(l,paquete(true,1,0,0,800),100);
  assert(l.completo&&l.cobertura==72&&l.minimo==800&&l.vigente(600)&&!l.vigente(601));
  auto malo=paquete(false,144,0,357.5,100);malo[10]^=4;
  dar(l,malo,120);dar(l,paquete(true,1,0,0,800),200);
  assert(!l.completo&&l.errores>0);
  dar(l,paquete(false,144,0,357.5,900),220);dar(l,paquete(true,1,0,0,900),300);
  assert(l.completo&&l.minimo==800); // muestra de inicio del barrido anterior.
  dar(l,paquete(false,144,0,357.5,0),320);dar(l,paquete(true,1,0,0,900),400);
  assert(!l.completo); // ceros no prueban que el entorno esta libre.
  LidarD6 wrap;
  dar(wrap,paquete(true,1,0,0,1000),0);
  dar(wrap,paquete(false,5,350,10,200),10);
  dar(wrap,paquete(true,1,0,0,1000),100);
  assert(!wrap.completo&&wrap.distancias[70]==200&&wrap.distancias[0]==200&&wrap.distancias[2]==200);
  std::cout<<"OK: CRC, tramas corruptas, resincronizacion, barridos, caducidad, ceros y cruce de 360 grados\n";
}
```

### `primera_prueba_exitosa_2026-11-14/RESPALDO.md`

```markdown
# Primera prueba exitosa — 14 de noviembre de 2026

Copia exacta de los ocho archivos de `prueba_integral_50cm`, creada antes de modificar la navegación. `SHA256.txt` permite comprobar su integridad. La verificación comparó cada copia con su original.

El usuario confirmó que estos códigos funcionaron. Las capturas muestran cámara, UART, MPU y LiDAR activos, con checksum corregido y cero errores de LiDAR. Esto no certifica navegación autónoma ni una prueba física de 50 cm completada: en las últimas capturas el botón seguía bloqueado y había ecos a 54–71 mm.

Conservar esta carpeta. Para volver a esta versión cargar ambos sketches de esta carpeta, con todos sus encabezados. No mezclar con los sketches de `navegacion_autonoma`, que usan otro protocolo de control.
```

### `primera_prueba_exitosa_2026-11-14/s3_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `primera_prueba_exitosa_2026-11-14/s3_integral/EstadoIntegral.h`

```cpp
#pragma once
#include <stdint.h>

struct EstadoIntegral {
  bool permiso = false;
  bool camara = false;
  bool lidar = false;
  bool enlace = false;
  bool imu = false;
  bool moviendo = false;
  bool operador = false;
  uint16_t minimo = 0;
  uint8_t sectores = 0;
  uint32_t paquetes = 0;
  uint32_t bytesLidar = 0;
  uint32_t erroresCabecera = 0;
  uint32_t erroresChecksum = 0;
  uint32_t iniciosLidar = 0;
  char diagnosticoLidar[180] = "Esperando bytes del LiDAR";
  uint32_t errores = 0;
  uint32_t barridos = 0;
  float fps = 0;
  float distancia = 0;
  uint16_t mapa[72]{};
  char devkit[300] = "Esperando DevKitC";
  char evento[96] = "Esperando video, LiDAR y DevKitC";
};
```

### `primera_prueba_exitosa_2026-11-14/s3_integral/LidarD6.h`

```cpp
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
        completo = !corrupto && cobertura >= 60 && puntos >= 100 &&
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
```

### `primera_prueba_exitosa_2026-11-14/s3_integral/s3_integral.ino`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "EnlaceRobot.h"
#include "LidarD6.h"
#include "EstadoIntegral.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Selecciona ESP32S3 Dev Module."
#endif

constexpr uint16_t OBSTACULO_MM = 250;
HardwareSerial enlace(1);
HardwareSerial puertoLidar(2);
ReceptorRobot receptor;
LidarD6 lidar;
httpd_handle_t servidorWeb = nullptr, servidorVideo = nullptr;
QueueHandle_t solicitudesInicio = nullptr;
portMUX_TYPE muxEstado = portMUX_INITIALIZER_UNLOCKED;
EstadoIntegral estado;
std::atomic<bool> solicitudStop{false};
std::atomic<bool> videoActivo{false};
std::atomic<uint32_t> ultimoFrame{0}, ultimoNavegador{0};
std::atomic<uint32_t> fpsDecimas{0};
bool camaraIniciada = false, servidoresListos = false;
bool devkitIMU = false, devkitMoviendo = false, devkitRecibido = false;
float distanciaDevkit = 0;
uint32_t sesion = 0, secuencia = 0, orden = 0;
uint32_t ultimoDevkit = 0, ultimoLatido = 0, ultimoLog = 0;
uint32_t ultimoArranqueLidar = 0;
char mensajeDevkit[300] = "Esperando DevKitC";
char evento[96] = "Abre la pagina y espera los indicadores";
char lineaUSB[32]{};
size_t longitudUSB = 0;
bool descartarUSB = false;

const char PAGINA[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Prueba integral 50 cm · diagnostico D6</title>
<style>
body{font:16px Arial;background:#142b43;color:#fff;margin:20px auto;max-width:1100px;padding:0 16px}
h1{font-size:25px}.paneles{display:flex;gap:16px;flex-wrap:wrap}.panel{flex:1;min-width:280px}
img,canvas{width:100%;max-width:520px;background:#203c55;border-radius:12px}
button{font-size:19px;padding:14px 22px;border:0;border-radius:8px;margin:8px 10px 8px 0;cursor:pointer}
#avanzar{background:#77dec4}#parar{background:#ff6666;font-weight:bold}button:disabled{opacity:.4}
pre{white-space:pre-wrap;background:#203c55;padding:14px;border-radius:8px;overflow-wrap:anywhere}
</style></head><body>
<h1>Robot 4WD · prueba integral de 50 cm</h1>
<p>La orden mueve las cuatro ruedas. El LiDAR detiene ante un punto a menos de 25 cm en cualquier direccion.</p>
<div class="paneles"><div class="panel"><h2>Camara OV5640</h2>
<img id="video" alt="Video de la camara"><br>
<button onclick="reconectar()">Reconectar video</button>
<a style="color:#8deadb" href="/captura" target="_blank">Captura</a></div>
<div class="panel"><h2>LiDAR · vista de 72 sectores</h2>
<canvas id="radar" width="420" height="420"></canvas>
<p>0 grados es la referencia del LiDAR. El dibujo no supone el frente del carro. Escala 3 m.</p></div></div>
<button id="avanzar" disabled onclick="mandar('avanzar')">AVANZA 50 cm</button>
<button id="parar" onclick="mandar('stop')">STOP</button>
<pre id="estado">Esperando datos...</pre><pre id="diagnostico"></pre><pre id="devkit"></pre>
<p>Mantener esta pagina abierta. Al perder el enlace con la pagina se cancela el movimiento.</p>
<script>
const boton=document.getElementById('avanzar');
const radar=document.getElementById('radar');const ctx=radar.getContext('2d');
function reconectar(){document.getElementById('video').src='http://'+location.hostname+':81/stream?t='+Date.now()}
async function mandar(accion){
 boton.disabled=true;
 try{const r=await fetch('/'+accion,{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('estado').textContent=await r.text()}catch(e){}
}
function dibujar(mapa){
 ctx.clearRect(0,0,420,420);ctx.strokeStyle='#56738b';ctx.fillStyle='#8deadb';
 for(let r=42;r<=168;r+=42){ctx.beginPath();ctx.arc(210,210,r,0,2*Math.PI);ctx.stroke()}
 ctx.beginPath();ctx.moveTo(210,20);ctx.lineTo(210,400);ctx.moveTo(20,210);ctx.lineTo(400,210);ctx.stroke();
 ctx.font='14px Arial';ctx.fillText('0 grados',220,20);
 mapa.forEach((d,i)=>{if(d>0&&d<=3000){const a=i*5*Math.PI/180;
 const r=d/3000*180;ctx.beginPath();ctx.arc(210+Math.sin(a)*r,210-Math.cos(a)*r,3,0,2*Math.PI);ctx.fill()}});
 ctx.fillStyle='#ffcc55';ctx.fillRect(205,205,10,10);
}
async function actualizar(){
 try{
  const r=await fetch('/estado',{cache:'no-store'});if(!r.ok)throw Error();const d=await r.json();
  const si=v=>v?'OK':'PENDIENTE';
  document.getElementById('estado').textContent=
   'Camara: '+si(d.camara)+' | FPS enviados: '+d.fps+'\n'+
   'LiDAR: '+si(d.lidar)+' | sectores: '+d.sectores+'/72 | minimo: '+d.minimo+' mm\n'+
   'UART: '+si(d.enlace)+' | MPU: '+si(d.imu)+' | operador: '+si(d.operador)+'\n'+
   'Distancia estimada: '+d.distancia+' cm | movimiento: '+(d.moviendo?'SI':'NO')+'\n'+
   'LiDAR paquetes: '+d.paquetes+' | errores: '+d.errores+' | barridos: '+d.barridos+'\n'+d.evento;
  document.getElementById('devkit').textContent='DevKitC\n'+d.devkit;
  document.getElementById('diagnostico').textContent=
   'DIAGNOSTICO D6 V3 SDK | bytes: '+d.bytesLidar+' | cabeceras invalidas: '+d.erroresCabecera+
   ' | checksum invalidos: '+d.erroresChecksum+' | inicios validos: '+d.iniciosLidar+'\n'+d.diagnosticoLidar;
  boton.disabled=!(d.permiso&&d.enlace&&d.imu&&!d.moviendo);dibujar(d.mapa);
 }catch(e){boton.disabled=true;document.getElementById('estado').textContent='Sin conexion al S3. Reconecta RobotCam.'}
 setTimeout(actualizar,300);
}
reconectar();actualizar();
</script></body></html>
)HTML";

bool peticionLocal(httpd_req_t *req) {
  char valor[4];
  return httpd_req_get_hdr_value_str(req, "X-Robot", valor, sizeof(valor)) == ESP_OK &&
         strcmp(valor, "1") == 0;
}

esp_err_t paginaWeb(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, PAGINA, HTTPD_RESP_USE_STRLEN);
}

esp_err_t estadoWeb(httpd_req_t *req) {
  ultimoNavegador.store(millis());
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  char mapa[512];
  size_t usados = 0;
  for (size_t i = 0; i < 72; ++i) {
    usados += snprintf(mapa + usados, sizeof(mapa) - usados, "%s%u", i ? "," : "", copia.mapa[i]);
  }
  char json[2200];
  snprintf(json, sizeof(json),
    "{\"permiso\":%u,\"camara\":%u,\"lidar\":%u,\"enlace\":%u,\"imu\":%u,"
    "\"moviendo\":%u,\"operador\":%u,\"minimo\":%u,\"sectores\":%u,\"paquetes\":%lu,"
    "\"errores\":%lu,\"barridos\":%lu,\"fps\":%.1f,\"distancia\":%.1f,"
    "\"bytesLidar\":%lu,\"erroresCabecera\":%lu,\"erroresChecksum\":%lu,\"iniciosLidar\":%lu,"
    "\"diagnosticoLidar\":\"%s\","
    "\"devkit\":\"%s\",\"evento\":\"%s\",\"mapa\":[%s]}",
    copia.permiso, copia.camara, copia.lidar, copia.enlace, copia.imu, copia.moviendo,
    copia.operador, copia.minimo, copia.sectores, (unsigned long)copia.paquetes,
    (unsigned long)copia.errores, (unsigned long)copia.barridos, copia.fps, copia.distancia,
    (unsigned long)copia.bytesLidar, (unsigned long)copia.erroresCabecera,
    (unsigned long)copia.erroresChecksum, (unsigned long)copia.iniciosLidar, copia.diagnosticoLidar,
    copia.devkit, copia.evento, mapa);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

esp_err_t avanzarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  if (!copia.permiso || !copia.enlace || !copia.imu || copia.moviendo) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan condiciones. Revisa los indicadores.");
  }
  const uint32_t ahora = millis();
  if (xQueueSend(solicitudesInicio, &ahora, 0) != pdPASS) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ya hay una orden pendiente.");
  }
  return httpd_resp_send(req, "Orden recibida. Esperando respuesta del DevKitC.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t pararWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  solicitudStop.store(true);
  return httpd_resp_send(req, "STOP solicitado", HTTPD_RESP_USE_STRLEN);
}

esp_err_t capturaWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  camera_fb_t *imagen = esp_camera_fb_get();
  if (!imagen) return httpd_resp_send_500(req);
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  const esp_err_t resultado = httpd_resp_send(req, (const char *)imagen->buf, imagen->len);
  esp_camera_fb_return(imagen);
  return resultado;
}

esp_err_t videoWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  esp_err_t resultado = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=robotframe");
  if (resultado != ESP_OK) return resultado;
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  videoActivo.store(true);
  uint32_t cuadros = 0, inicioFPS = millis();
  while (true) {
    camera_fb_t *imagen = esp_camera_fb_get();
    if (!imagen) { resultado = ESP_FAIL; break; }
    char cabecera[100];
    const int n = snprintf(cabecera, sizeof(cabecera),
      "\r\n--robotframe\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)imagen->len);
    resultado = httpd_resp_send_chunk(req, cabecera, n);
    if (resultado == ESP_OK) resultado = httpd_resp_send_chunk(req, (const char *)imagen->buf, imagen->len);
    esp_camera_fb_return(imagen);
    if (resultado != ESP_OK) break;
    ultimoFrame.store(millis());
    ++cuadros;
    const uint32_t transcurrido = millis() - inicioFPS;
    if (transcurrido >= 3000) {
      fpsDecimas.store(cuadros * 10000UL / transcurrido);
      inicioFPS = millis();
      cuadros = 0;
    }
    delay(1);
  }
  videoActivo.store(false);
  fpsDecimas.store(0);
  return resultado;
}

bool registrarRuta(httpd_handle_t servidor, const char *uri, httpd_method_t metodo,
                   esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t ruta = {};
  ruta.uri = uri;
  ruta.method = metodo;
  ruta.handler = handler;
  return httpd_register_uri_handler(servidor, &ruta) == ESP_OK;
}

bool iniciarServidores() {
  httpd_config_t web = HTTPD_DEFAULT_CONFIG();
  web.server_port = 80;
  web.stack_size = 8192;
  web.send_wait_timeout = 2;
  if (httpd_start(&servidorWeb, &web) != ESP_OK) return false;
  bool bien = registrarRuta(servidorWeb, "/", HTTP_GET, paginaWeb) &&
    registrarRuta(servidorWeb, "/estado", HTTP_GET, estadoWeb) &&
    registrarRuta(servidorWeb, "/avanzar", HTTP_POST, avanzarWeb) &&
    registrarRuta(servidorWeb, "/stop", HTTP_POST, pararWeb) &&
    registrarRuta(servidorWeb, "/captura", HTTP_GET, capturaWeb);
  httpd_config_t video = HTTPD_DEFAULT_CONFIG();
  video.server_port = 81;
  video.ctrl_port = web.ctrl_port + 1;
  video.stack_size = 8192;
  video.send_wait_timeout = 2;
  bien = bien && httpd_start(&servidorVideo, &video) == ESP_OK &&
         registrarRuta(servidorVideo, "/stream", HTTP_GET, videoWeb);
  if (!bien) {
    if (servidorVideo) httpd_stop(servidorVideo);
    httpd_stop(servidorWeb);
  }
  return bien;
}

bool iniciarCamara() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11; config.pin_d1 = 9; config.pin_d2 = 8; config.pin_d3 = 10;
  config.pin_d4 = 12; config.pin_d5 = 18; config.pin_d6 = 17; config.pin_d7 = 16;
  config.pin_xclk = 15; config.pin_pclk = 13; config.pin_vsync = 6; config.pin_href = 7;
  config.pin_sccb_sda = 4; config.pin_sccb_scl = 5;
  config.pin_pwdn = -1; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 16;
  const bool psram = psramFound();
  config.fb_count = psram ? 2 : 1;
  config.fb_location = psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = psram ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  Serial.printf("PSRAM=%s | buferes=%d\n", psram ? "SI" : "NO", config.fb_count);
  const esp_err_t resultado = esp_camera_init(&config);
  Serial.printf("CAMARA init=0x%X\n", (unsigned)resultado);
  return resultado == ESP_OK;
}

void procesarDevkit(const char *texto) {
  unsigned long recibida;
  unsigned int moviendo, imu;
  float distancia;
  if (sscanf(texto, "D %lu %u %u %f", &recibida, &moviendo, &imu, &distancia) != 4 ||
      recibida != sesion || moviendo > 1 || imu > 1 || !isfinite(distancia)) return;
  devkitRecibido = true;
  ultimoDevkit = millis();
  devkitMoviendo = moviendo;
  devkitIMU = imu;
  distanciaDevkit = distancia;
  // Solo caracteres generados por el firmware para conservar JSON valido.
  for (const char *p = texto; *p; ++p) {
    if (*p == '"' || *p == '\\' || static_cast<unsigned char>(*p) < 32) return;
  }
  snprintf(mensajeDevkit, sizeof(mensajeDevkit), "%s", texto);
}

void leerUSB() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineaUSB[longitudUSB] = '\0';
      if (!descartarUSB) {
        if (strcmp(lineaUSB, "STOP") == 0) solicitudStop.store(true);
        else if (strcmp(lineaUSB, "AVANZA 50") == 0) {
          const uint32_t ahora = millis();
          xQueueSend(solicitudesInicio, &ahora, 0);
        } else Serial.println("Comandos: AVANZA 50 | STOP. Mantener pagina con video abierta.");
      }
      longitudUSB = 0;
      descartarUSB = false;
    } else if (longitudUSB < sizeof(lineaUSB) - 1) lineaUSB[longitudUSB++] = c;
    else descartarUSB = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  sesion = esp_random();
  if (!sesion) sesion = 1;
  solicitudesInicio = xQueueCreate(1, sizeof(uint32_t));
  if (!solicitudesInicio) { Serial.println("ERROR memoria para cola de ordenes"); return; }
  enlace.setRxBufferSize(2048);
  enlace.begin(115200, SERIAL_8N1, 42, 41);
  enviarRobot(enlace, "STOP");
  puertoLidar.setRxBufferSize(8192);
  puertoLidar.begin(230400, SERIAL_8N1, 1, 2);
  camaraIniciada = iniciarCamara();
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("RobotCam", "CAMBIA_CLAVE_ROBOTCAM", 1, false, 1)) {
    Serial.println("ERROR WIFI");
    return;
  }
  servidoresListos = iniciarServidores();
  Serial.printf("INTEGRAL S3 V3 CHECKSUM SDK | camara=%u web=%u | LiDAR RX1 TX2\n", camaraIniciada, servidoresListos);
  Serial.print("Pagina http://"); Serial.println(WiFi.softAPIP());
  Serial.println("Wi-Fi RobotCam | clave CAMBIA_CLAVE_ROBOTCAM | motores parados hasta nueva orden");
  const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
  puertoLidar.write(iniciar, sizeof(iniciar));
  ultimoArranqueLidar = millis();
}

void loop() {
  if (!solicitudesInicio) { delay(100); return; }
  for (size_t i = 0; i < 4096 && puertoLidar.available(); ++i) {
    lidar.recibir(puertoLidar.read(), millis());
  }
  for (size_t i = 0; i < 1024 && enlace.available(); ++i) {
    const char *texto = receptor.recibir(enlace.read());
    if (texto) procesarDevkit(texto);
  }
  leerUSB();
  const uint32_t ahora = millis();
  if (lidar.paquetes == 0 && ahora - ultimoArranqueLidar > 4000) {
    const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
    puertoLidar.write(iniciar, sizeof(iniciar));
    ultimoArranqueLidar = ahora;
  }
  const bool video = camaraIniciada && videoActivo.load() && ahora - ultimoFrame.load() < 1000;
  const bool operador = ultimoNavegador.load() != 0 && ahora - ultimoNavegador.load() < 1200;
  const bool sensor = lidar.vigente(ahora);
  const bool devkit = devkitRecibido && ahora - ultimoDevkit <= 600;
  const bool permiso = servidoresListos && video && operador && sensor && devkit && devkitIMU && lidar.minimo >= OBSTACULO_MM;
  if (ahora - ultimoLatido >= 100) {
    ultimoLatido = ahora;
    char mensaje[100];
    snprintf(mensaje, sizeof(mensaje), "S %lu %lu %u %u",
      (unsigned long)sesion, (unsigned long)++secuencia, permiso, lidar.minimo);
    enviarRobot(enlace, mensaje);
  }
  if (solicitudStop.exchange(false)) {
    xQueueReset(solicitudesInicio);
    enviarRobot(enlace, "STOP");
    snprintf(evento, sizeof(evento), "STOP enviado al DevKitC");
  } else {
    uint32_t solicitada;
    if (xQueueReceive(solicitudesInicio, &solicitada, 0) == pdPASS) {
      if (ahora - solicitada <= 500 && permiso && devkit && devkitIMU && !devkitMoviendo) {
        char mensaje[80];
        snprintf(mensaje, sizeof(mensaje), "GO %lu %lu", (unsigned long)sesion, (unsigned long)++orden);
        enviarRobot(enlace, mensaje);
        snprintf(evento, sizeof(evento), "Orden %lu enviada. Ver resultado en DevKitC.", (unsigned long)orden);
      } else snprintf(evento, sizeof(evento), "Orden rechazada: revisar camara, LiDAR, MPU, enlace y distancia libre");
    }
  }
  portENTER_CRITICAL(&muxEstado);
  estado.permiso = permiso;
  estado.camara = video;
  estado.lidar = sensor;
  estado.enlace = devkit;
  estado.imu = devkit && devkitIMU;
  estado.moviendo = devkit && devkitMoviendo;
  estado.operador = operador;
  estado.minimo = lidar.minimo;
  estado.sectores = lidar.cobertura;
  estado.paquetes = lidar.paquetes;
  estado.bytesLidar = lidar.bytesRecibidos;
  estado.erroresCabecera = lidar.erroresCabecera;
  estado.erroresChecksum = lidar.erroresChecksum;
  estado.iniciosLidar = lidar.inicios;
  memcpy(estado.diagnosticoLidar, lidar.diagnostico, sizeof(estado.diagnosticoLidar));
  estado.errores = lidar.errores;
  estado.barridos = lidar.barridos;
  estado.fps = fpsDecimas.load() / 10.0f;
  estado.distancia = distanciaDevkit;
  memcpy(estado.mapa, lidar.distancias, sizeof(estado.mapa));
  memcpy(estado.devkit, mensajeDevkit, sizeof(estado.devkit));
  memcpy(estado.evento, evento, sizeof(estado.evento));
  portEXIT_CRITICAL(&muxEstado);
  if (ahora - ultimoLog >= 2000) {
    ultimoLog = ahora;
    Serial.printf("ESTADO | cam=%u lidar=%u uart=%u imu=%u permiso=%u | min=%u mm | sectores=%u/72 | paquetes=%lu errores=%lu\n",
      video, sensor, devkit, devkitIMU, permiso, lidar.minimo, lidar.cobertura,
      (unsigned long)lidar.paquetes, (unsigned long)lidar.errores);
  }
  delay(1);
}
```

### `primera_prueba_exitosa_2026-11-14/SHA256.txt`

```text
0C9FCD65A04746FFE81D78D46C02EE3114DA6FF0FD3D441619CB981C14764444  LEEME.md
E231CD0BE32C7B50444E8E5A4147B2FD36B027C98D77D4C6FFBF58BF5E4A5DF0  devkit_integral\devkit_integral.ino
C90B940B9E9032B232D8A4212F3786C72321A66CD426A6A954CE8C87AC178A24  devkit_integral\EnlaceRobot.h
68190E0970F0A2D3E64CAE069C257E8F1019FF9CC0B5554F7F5C9508176B6FA4  pruebas\protocolo_test.cpp
C90B940B9E9032B232D8A4212F3786C72321A66CD426A6A954CE8C87AC178A24  s3_integral\EnlaceRobot.h
ADE95B0B280654408A3029BD40B54CAE544FCB59DD3F3517648E648356357BFD  s3_integral\EstadoIntegral.h
F173D33189FEEECF20F6527CE96A81CB8B31768BBD2F57F5708101021B51B541  s3_integral\LidarD6.h
494E7B82F17915DB6CDCE5D957A32C0D123EF22A123BD1404712D875AEE08B6A  s3_integral\s3_integral.ino
```

### `prueba_integral_50cm/devkit_integral/devkit_integral.ino`

```cpp
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
```

### `prueba_integral_50cm/devkit_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `prueba_integral_50cm/LEEME.md`

````markdown
# Prueba integral: cámara + COIN-D6 + dos ESP32 + avance de 50 cm

Preparado el 14 de noviembre de 2026, revisando este chat, su conversación anterior «Analiza video y material de clase (2)» y los programas del proyecto. **Programas compilados; la integración completa de este paquete todavía debe probarse en el robot.** No se cargó firmware automáticamente.

## 1. Qué hace cada componente

| Componente | Función en esta prueba |
|---|---|
| ESP32-S3 | Video MJPEG, lectura UART del D6, página de control, órdenes y recepción de telemetría del DevKitC |
| Cámara OV5640 | Video en directo; si deja de enviarse video, se cancela el permiso de movimiento |
| COIN-D6 | Barrido y distancia mínima: bloquea/cancela movimiento si detecta menos de 250 mm en cualquier dirección |
| ESP32 DevKitC V4 / WROOM-32D | Cuatro motores, cuatro encoders, MPU6050 y parada local por pérdida de órdenes |
| Dos TB6612FNG | Un canal por motor, STBY común |
| Cuatro LM393 | Conteo de pulsos y estimación de distancia por rueda |
| MPU6050 | Lectura de aceleración y giro Z; si falla, se bloquea/cancela el avance |
| XL4016 | Salida regulada de 5,0 V, decisión recuperada del montaje anterior |

La cámara aporta supervisión visual; no se ejecuta reconocimiento de objetos. La MPU aporta telemetría y comprobación de comunicación, no corrección de trayectoria. No se implementan PID, SLAM, Nav2 ni navegación autónoma. Los encoders de un canal no determinan por sí mismos la dirección real. Los condensadores, interruptor y fusible son elementos eléctricos, no dispositivos programables. No es necesario introducir el LM2596 en este montaje si ya se adoptó el XL4016 único.

## 2. Archivos que debes cargar

1. DevKitC: abrir `devkit_integral/devkit_integral.ino`. Mantener `EnlaceRobot.h` en la misma carpeta.
2. S3: abrir `s3_integral/s3_integral.ino`. Mantener `EnlaceRobot.h`, `LidarD6.h` y `EstadoIntegral.h` en la misma carpeta.

Extraer TODO el ZIP antes de abrir los `.ino`. No pegar ambos programas en el mismo sketch. No hace falta instalar bibliotecas adicionales: se usan las incluidas en Arduino-ESP32.

## 3. Conexiones del LiDAR al S3: nombres y colores

Hacer los cambios con la alimentación apagada. El adaptador USB CH340 se retira del circuito del LiDAR para esta prueba; no conectar simultáneamente sus señales a las del S3.

| Cable del D6 mostrado en las fotos | Nombre / dirección | Conectar a |
|---|---|---|
| **Amarillo** | TXD: datos que salen del LiDAR | **GPIO1 del S3**, RX del puerto del LiDAR |
| **Verde** | RXD: órdenes que entran al LiDAR | **GPIO2 del S3**, TX del puerto del LiDAR |
| **Negro** | GND | Distribución GND común |
| **Rojo** | VCC | **5,0 V regulados del XL4016** |

Esta correspondencia se recuperó del pinout y el montaje del chat anterior; los colores no son una norma universal para otros cables. UART D6: **230400 baudios, 8N1, lógica de 3,3 V**. Los 5 V son alimentación, nunca una señal GPIO. COM9 era el CH340 del PC: no es un puerto que se configure dentro del firmware del S3.

## 4. UART entre los dos ESP32

| Origen | Destino | Función |
|---|---|---|
| S3 GPIO41 TX | DevKitC GPIO16 RX | Permiso, avance y STOP |
| DevKitC GPIO17 TX | S3 GPIO42 RX | Distancia, pulsos, MPU y estado |
| GND S3 | GND común DevKitC | Referencia eléctrica |

**115200 baudios, 8N1.** No unir los pines 3V3 de ambas placas. Los colores de estos tres puentes no quedaron confirmados en la evidencia disponible: identificarlos por sus extremos, no inventar un color.

## 5. Motores y encoders: mapa conservado

FL = delantero izquierdo; RL = trasero izquierdo; FR = delantero derecho; RR = trasero derecho.

| Rueda / canal | IN1 DevKitC | IN2 DevKitC | PWM DevKitC | Encoder DO/OUT DevKitC | Inversión en programa |
|---|---:|---:|---:|---:|---|
| FL / driver izquierdo A | 13 (AIN1) | 14 (AIN2) | 25 (PWMA) | 34 | Sí |
| RL / driver izquierdo B | 26 (BIN1) | 27 (BIN2) | 32 (PWMB) | 35 | No |
| FR / driver derecho A | 18 (AIN1) | 19 (AIN2) | 23 (PWMA) | 36 / VP | Sí |
| RR / driver derecho B | 4 (BIN1) | 15 (BIN2) | 33 (PWMB) | 39 / VN | No |

- STBY de ambos TB6612FNG → GPIO2 **del DevKitC**. Es otra placa distinta del GPIO2 del S3 usado para LiDAR.
- VCC lógico de ambos drivers → 3V3 del DevKitC.
- VM de ambos drivers → rama de motores a 5,0 V adoptada en el montaje; GND a distribución común.
- Canal A: AO1/AO2 al motor delantero; canal B: BO1/BO2 al trasero de ese lado. En el mapa previo aparecen pares rojo/negro de motor; la inversión de software conserva el sentido ya probado, no cambiar polaridad por el color.
- Cada encoder: DO/OUT a la entrada de la tabla; GND común. Conservar la alimentación del montaje ya probado y comprobar que DO no exceda 3,3 V. No alimentar a 5 V por deducción del nombre LM393. AO no se usa.
- GPIO34, 35, 36 y 39 no tienen resistencias pull-up internas; el firmware usa INPUT y depende de la salida/pull-up físico ya instalado.
- No hay registro fiable del color de cada puente IN/PWM/OUT: la tabla de GPIO es la referencia.

## 6. MPU6050 en el DevKitC

| Pin MPU | Conectar a |
|---|---|
| VCC | 3V3, según montaje que funcionó en la sesión |
| GND | GND común verificado; anteriormente un contacto de tierra falló |
| SDA | GPIO21 DevKitC |
| SCL | GPIO22 DevKitC |
| AD0 | Conservar montaje; el programa busca 0x68 y 0x69 |
| INT, XDA, XCL | No usados por este programa |

I2C a 100 kHz, lectura cada 50 ms. Se valida WHO_AM_I. Se reportan ax, ay, az en g y gz en grados/s. No se afirma que exista calibración de sesgo ni orientación absoluta. Si no detecta la MPU al arrancar, corregir su conexión y reiniciar; no cambiarla a 5 V como solución automática.

## 7. Cámara en el S3

| Señal | GPIO S3 |
|---|---:|
| D0, D1, D2, D3 | 11, 9, 8, 10 |
| D4, D5, D6, D7 | 12, 18, 17, 16 |
| XCLK / PCLK | 15 / 13 |
| VSYNC / HREF | 6 / 7 |
| SCCB SDA / SCL | 4 / 5 |
| PWDN / RESET | -1 / -1 |

Son conexiones del módulo de cámara existente. No se asignan estos GPIO también a sensores del S3. Video QVGA JPEG, calidad 16 y XCLK 20 MHz. Si detecta PSRAM usa dos buffers; si no, usa un buffer en DRAM. Se elimina el bloqueo anterior «PSRAM no detectada»: este programa puede continuar sin PSRAM. Los FPS se muestran como imágenes enviadas por segundo, sin prometer una tasa determinada.

## 8. Alimentación y carga de los programas

Decisión del chat anterior: XL4016 ajustado con multímetro a **5,0 V**, con ramas en paralelo hacia VM de ambos drivers, entradas **5V** de ambas placas y rojo VCC del D6. La lógica TB6612 permanece a **3V3 del DevKitC**. GND común con retornos de motores separados del cableado fino de lógica. No conectar batería directa a GPIO/3V3. Conservar interruptor, fusible y desacoplo del montaje; no hay evidencia suficiente para inventar valores de fusible o condensadores.

Para programar por USB, apagar la alimentación externa y **aislar sus positivos de 5 V de las placas** antes de enchufar USB, salvo que ya exista una protección de alimentación documentada. Evitar alimentar una placa apagada a través de TX/RX: al cargar cada placa por separado desconectar temporalmente el enlace UART entre placas. No mantener alimentación externa y USB unidos por sus positivos sin aislamiento verificado.

Una vez cargadas ambas placas: desconectar USB, apagar todo, restaurar UART y alimentación externa y ejecutar desde navegador. Así no necesitas USB durante el movimiento. Verificar que los 5 V no caigan durante el arranque simultáneo de motores y LiDAR; este software no puede corregir una caída de alimentación.

## 9. Carga paso a paso en Arduino IDE

### A. ESP32 DevKitC

1. Abrir `devkit_integral.ino`.
2. Placa: **ESP32 Dev Module**. Puerto correspondiente al DevKitC, históricamente COM5; comprobar el actual.
3. Flash Mode: DIO. El paquete se verificó con **Arduino-ESP32 3.3.12**.
4. Pulsar Verificar y Subir. Si aparece el antiguo error de modo de arranque, sostener BOOT, pulsar y soltar EN, iniciar/concluir conexión de carga y soltar BOOT cuando comience la escritura. No borrar flash como primer remedio.
5. Monitor a 115200 si lo necesitas durante programación. El arranque deja los motores deshabilitados.

### B. ESP32-S3

1. Abrir `s3_integral.ino`.
2. Placa **ESP32S3 Dev Module**; puerto del S3, históricamente COM6.
3. Flash Size **16MB**; PSRAM **OPI PSRAM**; USB CDC On Boot **Disabled** usando el USB-UART que se venía usando.
4. Partition Scheme: opción **16M con APP de 3MB y FATFS de 9MB** (`app3M_fat9M_16MB`).
5. Verificar y subir. Monitor 115200. Si PSRAM sigue sin detectarse, el firmware intenta cámara en DRAM y lo informa; no queda bloqueado por ese motivo.

Los puertos COM pueden cambiar al reconectar. No seleccionar COM9 del CH340 como si fuera una de estas placas.

## 10. Ejecutar la prueba completa

1. Con todo apagado, comprobar las conexiones anteriores y restaurar alimentación externa sin USB. Fijar la MPU y el LiDAR; que las ruedas, cables y chasis no tapen el plano de barrido.
2. Para el primer arranque de este firmware mantener las ruedas elevadas y despejadas, sin tocar la parte giratoria del LiDAR. Encender: los motores no deben arrancar automáticamente. Confirmar primero que STOP y estados responden. Apagar antes de bajar el robot.
3. En el suelo, preparar al menos 50 cm de recorrido más margen de parada, con espacio libre alrededor. La regla LiDAR es de **25 cm en los 360°**, no solo por delante; también una mano cerca por detrás puede bloquearlo. La distancia se mide desde el sensor, no desde el borde del chasis; no constituye una garantía anticolisión.
4. Conectar el teléfono/PC a Wi-Fi **RobotCam**, contraseña **CAMBIA_CLAVE_ROBOTCAM**. Aceptar permanecer en esa red aunque no tenga internet.
5. Abrir **http://192.168.4.1/**. Debe aparecer video y radar. Mantener esa página en primer plano; el AP admite un cliente Wi-Fi para esta prueba.
6. Esperar Cámara OK, LiDAR OK, UART OK y MPU OK. Si falta cualquiera, AVANZA permanece bloqueado. No anular el bloqueo para forzar la prueba.
7. Pulsar **AVANZA 50 cm una sola vez**. Los cuatro motores reciben PWM 80. Se muestra distancia estimada, pulsos y motivo del DevKitC. Mantener acceso al interruptor físico y no poner las manos delante del sensor durante el recorrido.
8. Debe detenerse con **FIN_50CM** cuando el promedio de las cuatro distancias alcance 50 cm. Medir con regla la distancia real. Un final por obstáculo, pérdida de enlace o tiempo no es una prueba de 50 cm completada.
9. Registrar distancia real, pulsos FL/RL/FR/RR, motivo final, estabilidad del video y mínimos LiDAR. No hay reanudación automática: tras una parada se requiere una orden nueva.

Los factores se reconstruyeron de los últimos registros aceptados de 30 cm: FL=13,2/27, RL=13,2/25, FR=13,2/26, RR=13,2/24 cm/pulso. **No son una nueva calibración validada de 50 cm.** No se reutilizaron los coeficientes antiguos incompatibles de `pruebas_distancia`. Deslizamiento, diferencias entre ruedas e inercia pueden cambiar el desplazamiento real; este programa no garantiza rectitud ni exactitud de 50,0 cm.

## 11. Paradas y condiciones de permiso

- Arranque y reinicio: motores parados; no arranque automático.
- STOP desde la página: orden UART al DevKitC, deshabilita STBY y PWM.
- LiDAR: checksum XOR, paquetes y barridos; un barrido válido requiere al menos 60 de 72 sectores con retorno válido, 100 puntos y duración de 40–300 ms. Se considera vencido tras 500 ms. Retornos cero no se interpretan como espacio libre. En entornos con pocos retornos puede bloquear aunque no haya un obstáculo: revisar cobertura, no forzar un permiso.
- Distancia mínima LiDAR inferior a 250 mm: se cancela el permiso. Se usa el mínimo de todo el barrido; no hace falta inventar cuál es el frente del sensor.
- DevKitC: si no recibe latido válido en 450 ms, se detiene. El S3 envía cada 100 ms.
- El S3 exige telemetría de retorno del DevKitC reciente (600 ms) y MPU válida: se comprueba también el sentido DevKitC→S3.
- MPU: lecturas válidas y recientes; fallo durante marcha causa parada local.
- Encoder: si cualquiera no cambia durante 1500 ms, parada. No detecta todos los casos de ruido o deslizamiento.
- Tiempo máximo de marcha: 8 s. Si vence antes de 50 cm, se detiene; no aumentar este límite sin identificar la causa.
- Video sin imágenes enviadas durante 1 s o página sin consultar estado durante 1,2 s: cancelación del permiso. Los plazos se suman al procesamiento y comunicación; no son latencias de parada física certificadas.

La orden manual no convierte este sistema en un dispositivo de seguridad. La desconexión del controlador web puede impedir entregar STOP; por eso existe además el vencimiento local de permiso y el interruptor físico.

## 12. Lectura de telemetría y problemas

### Corrección D6 V3 SDK — checksum y recepción en S3

La captura posterior del diagnóstico V2 mostró 483267 bytes, cero errores de
cabecera, 5985 errores de checksum y solo 25 paquetes aceptados. El SDK ROS 2
proporcionado en la carpeta del fabricante calcula el XOR de cada muestra como
`Si_L ^ (Si_2nd | (Si_H << 8))`. El dibujo del manual V1.0 muestra otra agrupación,
que fue la implementada inicialmente. Se corrigió `LidarD6.h` según el SDK,
archivo `m1ct_d2/src/lidar_data_processing.cpp`, líneas 571–580, del paquete
[m1ct_d4_ros2_v16.zip](https://drive.google.com/file/d/1j1spco3obOxyYVIh0IL4nm6542XacO9O/view).
Solo se acepta el checksum del SDK; no se deshabilita la validación.

La página muestra ahora `DIAGNOSTICO D6 V3 SDK`. Se agregó un vector fijo
con checksum 0x5AFC y distancia 946 mm, independiente del generador de paquetes,
y se comprueba el rechazo de 0x9F39 calculado con la agrupación anterior.
Falta comprobar esta corrección con el LiDAR físico tras cargar el S3.

Tras conectar amarillo a GPIO1 y verde a GPIO2, la captura del usuario mostró
125 paquetes aceptados, 30732 errores y cero barridos. Confirma recepción de
algunas tramas aceptadas, pero no validaba un barrido. El diagnóstico V2 permitió
localizar el rechazo en el checksum y contrastarlo posteriormente con el SDK.

La página ahora incluye `DIAGNOSTICO D6 V2`, con bytes recibidos, errores de
cabecera, errores de checksum, inicios válidos y una muestra hexadecimal de
la última trama rechazada muestreada (como máximo una actualización por segundo).
Solo hace falta volver a cargar el sketch del S3 con sus tres encabezados.
Tras restablecer alimentación y abrir la página, esperar diez segundos y
guardar una captura del diagnóstico. Los contadores son acumulados desde
el arranque. Esta actualización agrega evidencia para el diagnóstico y mantiene
las condiciones de permiso de movimiento. Tras la corrección V3 todavía hace
falta verificar barridos, cobertura y medidas físicas antes de aprobar la integración.

La línea del DevKitC tiene este orden:

```text
D sesion moviendo imu distancia_cm pulsos_FL pulsos_RL pulsos_FR pulsos_RR ax_g ay_g az_g gz_grados_s motivo
```

| Síntoma / motivo | Revisar |
|---|---|
| Cámara PENDIENTE | Video cargado, alimentación, conector cámara, botón Reconectar video; el movimiento exige stream activo |
| LiDAR paquetes=0 | Amarillo→S3 GPIO1, verde→GPIO2, alimentación 5 V, GND, CH340 retirado, D6 girando |
| Hay paquetes pero LiDAR PENDIENTE | Sectores, retornos, checksum, plano despejado y entorno; no basta que gire |
| Muchos errores LiDAR | Nivel lógico, cables, GND, alimentación/ruido de motores; no cambiar baudios al azar |
| Mínimo <250 mm | Objetos, manos o partes del propio montaje en el barrido; no reducir umbral solo para habilitar |
| UART PENDIENTE / SIN_ENLACE_S3 | TX/RX cruzados, GND, ambos firmwares nuevos; el antiguo HOLA/RECIBIDO no sirve con este protocolo |
| MPU PENDIENTE / MPU_SIN_DATOS | VCC3V3, contacto GND, SDA21/SCL22; corregir y reiniciar |
| S3_SIN_PERMISO | Ver indicadores de cámara, operador, LiDAR, UART y MPU en la página |
| ENCODER_SIN_PULSOS | Identificar contador inmóvil, DO y alimentación compatible, disco/rueda, motor trabado |
| LIMITE_TIEMPO | Carga de motores, energía, pulsos y distancia; la prueba no terminó 50 cm |
| Reinicios / flash read err | Alimentación y conexiones de arranque antes de atribuirlo al programa |
| Una rueda gira al revés | Apagar; cotejar mapa y montaje aceptado. No probar repetidamente en suelo |

## 13. Protocolo y decisiones técnicas

Entre ESP32: tramas ASCII `@contenido*CRC16\n`, CRC16-CCITT-FALSE. Mensajes S de estado/permiso, GO con identificador de sesión y orden, STOP y D de telemetría. Las órdenes repetidas se descartan y el cambio de sesión cancela el movimiento. El CRC detecta corrupción accidental; no es autenticación criptográfica.

El D6 usa UART 230400 y paquetes AA55; orden de inicio AA55F00F. Se valida longitud, ángulos, checksum XOR y cobertura antes de permitir marcha. Se dibujan mínimos por 72 sectores de 5°, no una nube de puntos completa ni un mapa SLAM.

Se separan servidor HTTP de control (80), servidor MJPEG (81) y bucle de sensores/UART. El stream no ocupa el bucle que vigila el LiDAR. Se conserva resolución pequeña para reducir tráfico y evitar el refresco antiguo de una foto cada segundo.

Referencias originales consultadas:

- [Ficha/pinout COIN-D6](https://drive.google.com/file/d/1j-oi7kPCEAPHREYggjbOy9C-KsARW4Vk/view).
- [Protocolo COIN-D6](https://drive.google.com/file/d/11bBGx5RCkZnwQlYUY3u3yagJFsP3wJo0/view).
- [Ejemplo CameraWebServer de Espressif 3.3.12](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/ESP32/examples/Camera/CameraWebServer/app_httpd.cpp).

## 14. Verificación realizada y límites

- Compilación real de los dos sketches con Arduino-ESP32 3.3.12, placas y opciones indicadas arriba.
- Prueba C++ de los encabezados realmente usados: vector CRC conocido, rechazo de trama corrupta, resincronización, exceso de longitud, checksum D6, recuperación tras corrupción, cobertura, retornos cero, caducidad y cruce de ángulo 360°.
- Archivo reproducible: `pruebas/protocolo_test.cpp`, compilable con g++ C++17; no es un tercer firmware.
- Pendiente en hardware: ejecutar estos dos programas juntos, medir recorrido real, verificar sentido de giro, caídas de tensión, ruido con motores, funcionamiento del bloqueo LiDAR y respuesta efectiva a STOP/pérdidas.
- En la conversación anterior el usuario confirmó LiDAR funcionando con CH340/CSPC (~9,9 Hz), montaje del sensor, alimentación XL4016 a 5 V y pruebas anteriores. Eso no equivale a haber validado este nuevo parser en el S3 ni este firmware integral.

Los documentos antiguos con «D6 sin identificar», «VM pendiente» o «MPU no detectada» reflejan etapas anteriores. Este paquete recoge la evolución del chat, sin declarar completadas las comprobaciones nuevas.
````

### `prueba_integral_50cm/pruebas/protocolo_test.cpp`

```cpp
#include "../s3_integral/LidarD6.h"
#include "../s3_integral/EnlaceRobot.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

std::vector<uint8_t> paquete(bool inicio, unsigned n, float a, float b, unsigned mm) {
  std::vector<uint8_t> p(10 + 3*n);
  auto poner=[&](unsigned i,unsigned v){p[i]=v&255;p[i+1]=v>>8;};
  p[0]=0xAA;p[1]=0x55;p[2]=inicio?21:20;p[3]=n;
  poner(4,unsigned(a*128)+1);poner(6,unsigned(b*128)+1);
  for(unsigned i=0;i<n;++i){p[10+3*i]=0;p[11+3*i]=(mm&63)<<2;p[12+3*i]=mm>>6;}
  unsigned cs=0;
  // SDK: cabecera 16 bits; cada muestra usa 8 bits y luego 16 bits LE.
  for(unsigned i=0;i<8;i+=2)cs^=p[i]|(p[i+1]<<8);
  for(unsigned i=10;i<p.size();i+=3)cs^=p[i]^(p[i+1]|(p[i+2]<<8));
  poner(8,cs);return p;
}
void dar(LidarD6 &l,const std::vector<uint8_t>&p,unsigned t){for(auto b:p)l.recibir(b,t);}
int main(){
  // Vector fijo independiente del generador: XOR SDK=0x5AFC, distancia=946 mm.
  // El agrupamiento anterior produce 0x9F39 y debe rechazar la trama.
  const std::vector<uint8_t> vectorSDK = {
    0xAA,0x55,0x15,0x01,0x01,0x00,0x01,0x00,0xFC,0x5A,0x88,0xCB,0x0E
  };
  LidarD6 referencia;
  dar(referencia,vectorSDK,0);
  dar(referencia,vectorSDK,100);
  assert(referencia.paquetes==2 && referencia.inicios==2 && referencia.errores==0);
  assert(referencia.distancias[0]==946);
  auto checksumAnterior=vectorSDK;
  checksumAnterior[8]=0x39;checksumAnterior[9]=0x9F;
  dar(referencia,checksumAnterior,120);
  assert(referencia.paquetes==2 && referencia.erroresChecksum==1);
  assert(crcRobot("123456789",9)==0x29B1);
  ReceptorRobot r;
  auto recibir=[&](std::string s){std::string salida;for(char c:s){auto p=r.recibir(c);if(p)salida=p;}return salida;};
  assert(recibir("ruido@123456789*29B1\n")=="123456789");
  assert(recibir("@123456788*29B1\n").empty());
  assert(recibir("@incompleto@123456789*29B1\r\n")=="123456789");
  assert(recibir("@"+std::string(500,'x')+"\n").empty());
  assert(recibir("@123456789*29B1\n")=="123456789");
  LidarD6 l;
  dar(l,paquete(true,1,0,0,800),0);
  dar(l,paquete(false,144,0,357.5,800),20);
  dar(l,paquete(true,1,0,0,800),100);
  assert(l.completo&&l.cobertura==72&&l.minimo==800&&l.vigente(600)&&!l.vigente(601));
  auto malo=paquete(false,144,0,357.5,100);malo[10]^=4;
  dar(l,malo,120);dar(l,paquete(true,1,0,0,800),200);
  assert(!l.completo&&l.errores>0);
  dar(l,paquete(false,144,0,357.5,900),220);dar(l,paquete(true,1,0,0,900),300);
  assert(l.completo&&l.minimo==800); // muestra de inicio del barrido anterior.
  dar(l,paquete(false,144,0,357.5,0),320);dar(l,paquete(true,1,0,0,900),400);
  assert(!l.completo); // ceros no prueban que el entorno esta libre.
  LidarD6 wrap;
  dar(wrap,paquete(true,1,0,0,1000),0);
  dar(wrap,paquete(false,5,350,10,200),10);
  dar(wrap,paquete(true,1,0,0,1000),100);
  assert(!wrap.completo&&wrap.distancias[70]==200&&wrap.distancias[0]==200&&wrap.distancias[2]==200);
  std::cout<<"OK: CRC, tramas corruptas, resincronizacion, barridos, caducidad, ceros y cruce de 360 grados\n";
}
```

### `prueba_integral_50cm/s3_integral/EnlaceRobot.h`

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// CRC16-CCITT-FALSE. Formato de UART: @contenido*CCCC\n.
inline uint16_t crcRobot(const char *datos, size_t longitud) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < longitud; ++i) {
    crc ^= static_cast<uint16_t>(static_cast<uint8_t>(datos[i])) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc;
}

template <typename Puerto>
void enviarRobot(Puerto &puerto, const char *contenido) {
  char fin[8];
  snprintf(fin, sizeof(fin), "*%04X\n", crcRobot(contenido, strlen(contenido)));
  puerto.print('@');
  puerto.print(contenido);
  puerto.print(fin);
}

class ReceptorRobot {
 public:
  // Devuelve un contenido terminado en cero solo al recibir una trama valida.
  const char *recibir(char c) {
    if (c == '@') {
      longitud = 0;
      activo = true;
      return nullptr;
    }
    if (!activo || c == '\r') return nullptr;
    if (c != '\n') {
      if (longitud >= sizeof(linea) - 1) {
        activo = false;
      } else {
        linea[longitud++] = c;
      }
      return nullptr;
    }
    activo = false;
    linea[longitud] = '\0';
    if (longitud < 6 || linea[longitud - 5] != '*') return nullptr;
    const size_t cuerpo = longitud - 5;
    uint16_t esperado = 0;
    for (size_t i = cuerpo + 1; i < longitud; ++i) {
      const char h = linea[i];
      int valor = -1;
      if (h >= '0' && h <= '9') valor = h - '0';
      if (h >= 'A' && h <= 'F') valor = h - 'A' + 10;
      if (h >= 'a' && h <= 'f') valor = h - 'a' + 10;
      if (valor < 0) return nullptr;
      esperado = (esperado << 4) | valor;
    }
    if (crcRobot(linea, cuerpo) != esperado) return nullptr;
    linea[cuerpo] = '\0';
    return linea;
  }

 private:
  char linea[384]{};
  size_t longitud = 0;
  bool activo = false;
};
```

### `prueba_integral_50cm/s3_integral/EstadoIntegral.h`

```cpp
#pragma once
#include <stdint.h>

struct EstadoIntegral {
  bool permiso = false;
  bool camara = false;
  bool lidar = false;
  bool enlace = false;
  bool imu = false;
  bool moviendo = false;
  bool operador = false;
  uint16_t minimo = 0;
  uint8_t sectores = 0;
  uint32_t paquetes = 0;
  uint32_t bytesLidar = 0;
  uint32_t erroresCabecera = 0;
  uint32_t erroresChecksum = 0;
  uint32_t iniciosLidar = 0;
  char diagnosticoLidar[180] = "Esperando bytes del LiDAR";
  uint32_t errores = 0;
  uint32_t barridos = 0;
  float fps = 0;
  float distancia = 0;
  uint16_t mapa[72]{};
  char devkit[300] = "Esperando DevKitC";
  char evento[96] = "Esperando video, LiDAR y DevKitC";
};
```

### `prueba_integral_50cm/s3_integral/LidarD6.h`

```cpp
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
        completo = !corrupto && cobertura >= 60 && puntos >= 100 &&
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
```

### `prueba_integral_50cm/s3_integral/s3_integral.ino`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "EnlaceRobot.h"
#include "LidarD6.h"
#include "EstadoIntegral.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Selecciona ESP32S3 Dev Module."
#endif

constexpr uint16_t OBSTACULO_MM = 250;
HardwareSerial enlace(1);
HardwareSerial puertoLidar(2);
ReceptorRobot receptor;
LidarD6 lidar;
httpd_handle_t servidorWeb = nullptr, servidorVideo = nullptr;
QueueHandle_t solicitudesInicio = nullptr;
portMUX_TYPE muxEstado = portMUX_INITIALIZER_UNLOCKED;
EstadoIntegral estado;
std::atomic<bool> solicitudStop{false};
std::atomic<bool> videoActivo{false};
std::atomic<uint32_t> ultimoFrame{0}, ultimoNavegador{0};
std::atomic<uint32_t> fpsDecimas{0};
bool camaraIniciada = false, servidoresListos = false;
bool devkitIMU = false, devkitMoviendo = false, devkitRecibido = false;
float distanciaDevkit = 0;
uint32_t sesion = 0, secuencia = 0, orden = 0;
uint32_t ultimoDevkit = 0, ultimoLatido = 0, ultimoLog = 0;
uint32_t ultimoArranqueLidar = 0;
char mensajeDevkit[300] = "Esperando DevKitC";
char evento[96] = "Abre la pagina y espera los indicadores";
char lineaUSB[32]{};
size_t longitudUSB = 0;
bool descartarUSB = false;

const char PAGINA[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Prueba integral 50 cm · diagnostico D6</title>
<style>
body{font:16px Arial;background:#142b43;color:#fff;margin:20px auto;max-width:1100px;padding:0 16px}
h1{font-size:25px}.paneles{display:flex;gap:16px;flex-wrap:wrap}.panel{flex:1;min-width:280px}
img,canvas{width:100%;max-width:520px;background:#203c55;border-radius:12px}
button{font-size:19px;padding:14px 22px;border:0;border-radius:8px;margin:8px 10px 8px 0;cursor:pointer}
#avanzar{background:#77dec4}#parar{background:#ff6666;font-weight:bold}button:disabled{opacity:.4}
pre{white-space:pre-wrap;background:#203c55;padding:14px;border-radius:8px;overflow-wrap:anywhere}
</style></head><body>
<h1>Robot 4WD · prueba integral de 50 cm</h1>
<p>La orden mueve las cuatro ruedas. El LiDAR detiene ante un punto a menos de 25 cm en cualquier direccion.</p>
<div class="paneles"><div class="panel"><h2>Camara OV5640</h2>
<img id="video" alt="Video de la camara"><br>
<button onclick="reconectar()">Reconectar video</button>
<a style="color:#8deadb" href="/captura" target="_blank">Captura</a></div>
<div class="panel"><h2>LiDAR · vista de 72 sectores</h2>
<canvas id="radar" width="420" height="420"></canvas>
<p>0 grados es la referencia del LiDAR. El dibujo no supone el frente del carro. Escala 3 m.</p></div></div>
<button id="avanzar" disabled onclick="mandar('avanzar')">AVANZA 50 cm</button>
<button id="parar" onclick="mandar('stop')">STOP</button>
<pre id="estado">Esperando datos...</pre><pre id="diagnostico"></pre><pre id="devkit"></pre>
<p>Mantener esta pagina abierta. Al perder el enlace con la pagina se cancela el movimiento.</p>
<script>
const boton=document.getElementById('avanzar');
const radar=document.getElementById('radar');const ctx=radar.getContext('2d');
function reconectar(){document.getElementById('video').src='http://'+location.hostname+':81/stream?t='+Date.now()}
async function mandar(accion){
 boton.disabled=true;
 try{const r=await fetch('/'+accion,{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('estado').textContent=await r.text()}catch(e){}
}
function dibujar(mapa){
 ctx.clearRect(0,0,420,420);ctx.strokeStyle='#56738b';ctx.fillStyle='#8deadb';
 for(let r=42;r<=168;r+=42){ctx.beginPath();ctx.arc(210,210,r,0,2*Math.PI);ctx.stroke()}
 ctx.beginPath();ctx.moveTo(210,20);ctx.lineTo(210,400);ctx.moveTo(20,210);ctx.lineTo(400,210);ctx.stroke();
 ctx.font='14px Arial';ctx.fillText('0 grados',220,20);
 mapa.forEach((d,i)=>{if(d>0&&d<=3000){const a=i*5*Math.PI/180;
 const r=d/3000*180;ctx.beginPath();ctx.arc(210+Math.sin(a)*r,210-Math.cos(a)*r,3,0,2*Math.PI);ctx.fill()}});
 ctx.fillStyle='#ffcc55';ctx.fillRect(205,205,10,10);
}
async function actualizar(){
 try{
  const r=await fetch('/estado',{cache:'no-store'});if(!r.ok)throw Error();const d=await r.json();
  const si=v=>v?'OK':'PENDIENTE';
  document.getElementById('estado').textContent=
   'Camara: '+si(d.camara)+' | FPS enviados: '+d.fps+'\n'+
   'LiDAR: '+si(d.lidar)+' | sectores: '+d.sectores+'/72 | minimo: '+d.minimo+' mm\n'+
   'UART: '+si(d.enlace)+' | MPU: '+si(d.imu)+' | operador: '+si(d.operador)+'\n'+
   'Distancia estimada: '+d.distancia+' cm | movimiento: '+(d.moviendo?'SI':'NO')+'\n'+
   'LiDAR paquetes: '+d.paquetes+' | errores: '+d.errores+' | barridos: '+d.barridos+'\n'+d.evento;
  document.getElementById('devkit').textContent='DevKitC\n'+d.devkit;
  document.getElementById('diagnostico').textContent=
   'DIAGNOSTICO D6 V3 SDK | bytes: '+d.bytesLidar+' | cabeceras invalidas: '+d.erroresCabecera+
   ' | checksum invalidos: '+d.erroresChecksum+' | inicios validos: '+d.iniciosLidar+'\n'+d.diagnosticoLidar;
  boton.disabled=!(d.permiso&&d.enlace&&d.imu&&!d.moviendo);dibujar(d.mapa);
 }catch(e){boton.disabled=true;document.getElementById('estado').textContent='Sin conexion al S3. Reconecta RobotCam.'}
 setTimeout(actualizar,300);
}
reconectar();actualizar();
</script></body></html>
)HTML";

bool peticionLocal(httpd_req_t *req) {
  char valor[4];
  return httpd_req_get_hdr_value_str(req, "X-Robot", valor, sizeof(valor)) == ESP_OK &&
         strcmp(valor, "1") == 0;
}

esp_err_t paginaWeb(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, PAGINA, HTTPD_RESP_USE_STRLEN);
}

esp_err_t estadoWeb(httpd_req_t *req) {
  ultimoNavegador.store(millis());
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  char mapa[512];
  size_t usados = 0;
  for (size_t i = 0; i < 72; ++i) {
    usados += snprintf(mapa + usados, sizeof(mapa) - usados, "%s%u", i ? "," : "", copia.mapa[i]);
  }
  char json[2200];
  snprintf(json, sizeof(json),
    "{\"permiso\":%u,\"camara\":%u,\"lidar\":%u,\"enlace\":%u,\"imu\":%u,"
    "\"moviendo\":%u,\"operador\":%u,\"minimo\":%u,\"sectores\":%u,\"paquetes\":%lu,"
    "\"errores\":%lu,\"barridos\":%lu,\"fps\":%.1f,\"distancia\":%.1f,"
    "\"bytesLidar\":%lu,\"erroresCabecera\":%lu,\"erroresChecksum\":%lu,\"iniciosLidar\":%lu,"
    "\"diagnosticoLidar\":\"%s\","
    "\"devkit\":\"%s\",\"evento\":\"%s\",\"mapa\":[%s]}",
    copia.permiso, copia.camara, copia.lidar, copia.enlace, copia.imu, copia.moviendo,
    copia.operador, copia.minimo, copia.sectores, (unsigned long)copia.paquetes,
    (unsigned long)copia.errores, (unsigned long)copia.barridos, copia.fps, copia.distancia,
    (unsigned long)copia.bytesLidar, (unsigned long)copia.erroresCabecera,
    (unsigned long)copia.erroresChecksum, (unsigned long)copia.iniciosLidar, copia.diagnosticoLidar,
    copia.devkit, copia.evento, mapa);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

esp_err_t avanzarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  if (!copia.permiso || !copia.enlace || !copia.imu || copia.moviendo) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan condiciones. Revisa los indicadores.");
  }
  const uint32_t ahora = millis();
  if (xQueueSend(solicitudesInicio, &ahora, 0) != pdPASS) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ya hay una orden pendiente.");
  }
  return httpd_resp_send(req, "Orden recibida. Esperando respuesta del DevKitC.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t pararWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  solicitudStop.store(true);
  return httpd_resp_send(req, "STOP solicitado", HTTPD_RESP_USE_STRLEN);
}

esp_err_t capturaWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  camera_fb_t *imagen = esp_camera_fb_get();
  if (!imagen) return httpd_resp_send_500(req);
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  const esp_err_t resultado = httpd_resp_send(req, (const char *)imagen->buf, imagen->len);
  esp_camera_fb_return(imagen);
  return resultado;
}

esp_err_t videoWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  esp_err_t resultado = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=robotframe");
  if (resultado != ESP_OK) return resultado;
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  videoActivo.store(true);
  uint32_t cuadros = 0, inicioFPS = millis();
  while (true) {
    camera_fb_t *imagen = esp_camera_fb_get();
    if (!imagen) { resultado = ESP_FAIL; break; }
    char cabecera[100];
    const int n = snprintf(cabecera, sizeof(cabecera),
      "\r\n--robotframe\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)imagen->len);
    resultado = httpd_resp_send_chunk(req, cabecera, n);
    if (resultado == ESP_OK) resultado = httpd_resp_send_chunk(req, (const char *)imagen->buf, imagen->len);
    esp_camera_fb_return(imagen);
    if (resultado != ESP_OK) break;
    ultimoFrame.store(millis());
    ++cuadros;
    const uint32_t transcurrido = millis() - inicioFPS;
    if (transcurrido >= 3000) {
      fpsDecimas.store(cuadros * 10000UL / transcurrido);
      inicioFPS = millis();
      cuadros = 0;
    }
    delay(1);
  }
  videoActivo.store(false);
  fpsDecimas.store(0);
  return resultado;
}

bool registrarRuta(httpd_handle_t servidor, const char *uri, httpd_method_t metodo,
                   esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t ruta = {};
  ruta.uri = uri;
  ruta.method = metodo;
  ruta.handler = handler;
  return httpd_register_uri_handler(servidor, &ruta) == ESP_OK;
}

bool iniciarServidores() {
  httpd_config_t web = HTTPD_DEFAULT_CONFIG();
  web.server_port = 80;
  web.stack_size = 8192;
  web.send_wait_timeout = 2;
  if (httpd_start(&servidorWeb, &web) != ESP_OK) return false;
  bool bien = registrarRuta(servidorWeb, "/", HTTP_GET, paginaWeb) &&
    registrarRuta(servidorWeb, "/estado", HTTP_GET, estadoWeb) &&
    registrarRuta(servidorWeb, "/avanzar", HTTP_POST, avanzarWeb) &&
    registrarRuta(servidorWeb, "/stop", HTTP_POST, pararWeb) &&
    registrarRuta(servidorWeb, "/captura", HTTP_GET, capturaWeb);
  httpd_config_t video = HTTPD_DEFAULT_CONFIG();
  video.server_port = 81;
  video.ctrl_port = web.ctrl_port + 1;
  video.stack_size = 8192;
  video.send_wait_timeout = 2;
  bien = bien && httpd_start(&servidorVideo, &video) == ESP_OK &&
         registrarRuta(servidorVideo, "/stream", HTTP_GET, videoWeb);
  if (!bien) {
    if (servidorVideo) httpd_stop(servidorVideo);
    httpd_stop(servidorWeb);
  }
  return bien;
}

bool iniciarCamara() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11; config.pin_d1 = 9; config.pin_d2 = 8; config.pin_d3 = 10;
  config.pin_d4 = 12; config.pin_d5 = 18; config.pin_d6 = 17; config.pin_d7 = 16;
  config.pin_xclk = 15; config.pin_pclk = 13; config.pin_vsync = 6; config.pin_href = 7;
  config.pin_sccb_sda = 4; config.pin_sccb_scl = 5;
  config.pin_pwdn = -1; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 16;
  const bool psram = psramFound();
  config.fb_count = psram ? 2 : 1;
  config.fb_location = psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = psram ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  Serial.printf("PSRAM=%s | buferes=%d\n", psram ? "SI" : "NO", config.fb_count);
  const esp_err_t resultado = esp_camera_init(&config);
  Serial.printf("CAMARA init=0x%X\n", (unsigned)resultado);
  return resultado == ESP_OK;
}

void procesarDevkit(const char *texto) {
  unsigned long recibida;
  unsigned int moviendo, imu;
  float distancia;
  if (sscanf(texto, "D %lu %u %u %f", &recibida, &moviendo, &imu, &distancia) != 4 ||
      recibida != sesion || moviendo > 1 || imu > 1 || !isfinite(distancia)) return;
  devkitRecibido = true;
  ultimoDevkit = millis();
  devkitMoviendo = moviendo;
  devkitIMU = imu;
  distanciaDevkit = distancia;
  // Solo caracteres generados por el firmware para conservar JSON valido.
  for (const char *p = texto; *p; ++p) {
    if (*p == '"' || *p == '\\' || static_cast<unsigned char>(*p) < 32) return;
  }
  snprintf(mensajeDevkit, sizeof(mensajeDevkit), "%s", texto);
}

void leerUSB() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineaUSB[longitudUSB] = '\0';
      if (!descartarUSB) {
        if (strcmp(lineaUSB, "STOP") == 0) solicitudStop.store(true);
        else if (strcmp(lineaUSB, "AVANZA 50") == 0) {
          const uint32_t ahora = millis();
          xQueueSend(solicitudesInicio, &ahora, 0);
        } else Serial.println("Comandos: AVANZA 50 | STOP. Mantener pagina con video abierta.");
      }
      longitudUSB = 0;
      descartarUSB = false;
    } else if (longitudUSB < sizeof(lineaUSB) - 1) lineaUSB[longitudUSB++] = c;
    else descartarUSB = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  sesion = esp_random();
  if (!sesion) sesion = 1;
  solicitudesInicio = xQueueCreate(1, sizeof(uint32_t));
  if (!solicitudesInicio) { Serial.println("ERROR memoria para cola de ordenes"); return; }
  enlace.setRxBufferSize(2048);
  enlace.begin(115200, SERIAL_8N1, 42, 41);
  enviarRobot(enlace, "STOP");
  puertoLidar.setRxBufferSize(8192);
  puertoLidar.begin(230400, SERIAL_8N1, 1, 2);
  camaraIniciada = iniciarCamara();
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("RobotCam", "CAMBIA_CLAVE_ROBOTCAM", 1, false, 1)) {
    Serial.println("ERROR WIFI");
    return;
  }
  servidoresListos = iniciarServidores();
  Serial.printf("INTEGRAL S3 V3 CHECKSUM SDK | camara=%u web=%u | LiDAR RX1 TX2\n", camaraIniciada, servidoresListos);
  Serial.print("Pagina http://"); Serial.println(WiFi.softAPIP());
  Serial.println("Wi-Fi RobotCam | clave CAMBIA_CLAVE_ROBOTCAM | motores parados hasta nueva orden");
  const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
  puertoLidar.write(iniciar, sizeof(iniciar));
  ultimoArranqueLidar = millis();
}

void loop() {
  if (!solicitudesInicio) { delay(100); return; }
  for (size_t i = 0; i < 4096 && puertoLidar.available(); ++i) {
    lidar.recibir(puertoLidar.read(), millis());
  }
  for (size_t i = 0; i < 1024 && enlace.available(); ++i) {
    const char *texto = receptor.recibir(enlace.read());
    if (texto) procesarDevkit(texto);
  }
  leerUSB();
  const uint32_t ahora = millis();
  if (lidar.paquetes == 0 && ahora - ultimoArranqueLidar > 4000) {
    const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
    puertoLidar.write(iniciar, sizeof(iniciar));
    ultimoArranqueLidar = ahora;
  }
  const bool video = camaraIniciada && videoActivo.load() && ahora - ultimoFrame.load() < 1000;
  const bool operador = ultimoNavegador.load() != 0 && ahora - ultimoNavegador.load() < 1200;
  const bool sensor = lidar.vigente(ahora);
  const bool devkit = devkitRecibido && ahora - ultimoDevkit <= 600;
  const bool permiso = servidoresListos && video && operador && sensor && devkit && devkitIMU && lidar.minimo >= OBSTACULO_MM;
  if (ahora - ultimoLatido >= 100) {
    ultimoLatido = ahora;
    char mensaje[100];
    snprintf(mensaje, sizeof(mensaje), "S %lu %lu %u %u",
      (unsigned long)sesion, (unsigned long)++secuencia, permiso, lidar.minimo);
    enviarRobot(enlace, mensaje);
  }
  if (solicitudStop.exchange(false)) {
    xQueueReset(solicitudesInicio);
    enviarRobot(enlace, "STOP");
    snprintf(evento, sizeof(evento), "STOP enviado al DevKitC");
  } else {
    uint32_t solicitada;
    if (xQueueReceive(solicitudesInicio, &solicitada, 0) == pdPASS) {
      if (ahora - solicitada <= 500 && permiso && devkit && devkitIMU && !devkitMoviendo) {
        char mensaje[80];
        snprintf(mensaje, sizeof(mensaje), "GO %lu %lu", (unsigned long)sesion, (unsigned long)++orden);
        enviarRobot(enlace, mensaje);
        snprintf(evento, sizeof(evento), "Orden %lu enviada. Ver resultado en DevKitC.", (unsigned long)orden);
      } else snprintf(evento, sizeof(evento), "Orden rechazada: revisar camara, LiDAR, MPU, enlace y distancia libre");
    }
  }
  portENTER_CRITICAL(&muxEstado);
  estado.permiso = permiso;
  estado.camara = video;
  estado.lidar = sensor;
  estado.enlace = devkit;
  estado.imu = devkit && devkitIMU;
  estado.moviendo = devkit && devkitMoviendo;
  estado.operador = operador;
  estado.minimo = lidar.minimo;
  estado.sectores = lidar.cobertura;
  estado.paquetes = lidar.paquetes;
  estado.bytesLidar = lidar.bytesRecibidos;
  estado.erroresCabecera = lidar.erroresCabecera;
  estado.erroresChecksum = lidar.erroresChecksum;
  estado.iniciosLidar = lidar.inicios;
  memcpy(estado.diagnosticoLidar, lidar.diagnostico, sizeof(estado.diagnosticoLidar));
  estado.errores = lidar.errores;
  estado.barridos = lidar.barridos;
  estado.fps = fpsDecimas.load() / 10.0f;
  estado.distancia = distanciaDevkit;
  memcpy(estado.mapa, lidar.distancias, sizeof(estado.mapa));
  memcpy(estado.devkit, mensajeDevkit, sizeof(estado.devkit));
  memcpy(estado.evento, evento, sizeof(estado.evento));
  portEXIT_CRITICAL(&muxEstado);
  if (ahora - ultimoLog >= 2000) {
    ultimoLog = ahora;
    Serial.printf("ESTADO | cam=%u lidar=%u uart=%u imu=%u permiso=%u | min=%u mm | sectores=%u/72 | paquetes=%lu errores=%lu\n",
      video, sensor, devkit, devkitIMU, permiso, lidar.minimo, lidar.cobertura,
      (unsigned long)lidar.paquetes, (unsigned long)lidar.errores);
  }
  delay(1);
}
```

## C. Cableado, reglas del proyecto y pruebas de componentes

### `MAPA_CONEXIONES.md`

```markdown
# Mapa de conexiones — robot 4WD con dos ESP32

Estado: revisión de documentación 2026-10-22; diseño de cableado NO probado físicamente. Sustituye el mapa anterior que compartía dirección y PWM por lado. Los números son GPIO, no posiciones del conector. El DevKitC de las fotos es una variante con USB-C; la guía oficial es referencia de GPIO, no certifica su circuito de alimentación exacto.

## Láminas para seguir el montaje

1. `esquema-01-motores.svg`: doce señales independientes, salidas de los motores y conexiones compartidas.
2. `esquema-02-sensores-uart.svg`: cuatro OUT, bus I2C y enlace entre placas.
3. `esquema-03-alimentacion.svg`: ramas, retornos y condensadores.

Las láminas son esquemas funcionales: los pines de cada bloque están ordenados para leer cables, NO como aparecen físicamente en el módulo. Seguir siempre la etiqueta del pin real. El dibujo anterior `diseno-montaje-dos-esp32.svg` es únicamente una propuesta de ubicación sobre el chasis. El plano Draw.io antiguo no debe usarse para cablear.

## Reparto

### Auditoría de alimentación tras fallos de arranque (2026-10-29)

- **Se mantiene 3V3 DevKitC → VCC de ambos TB6612FNG.** Alimenta la lógica, no los motores. El TB6612 especifica VCC entre 2,7 y 5,5 V; escoger 3,3 V mantiene compatibilidad con los GPIO. No mover VCC a 5 V como corrección automática: con VCC=5 V, el mínimo VIH=0,7×VCC exige 3,5 V y un GPIO de 3,3 V no garantiza alto válido.
- **Se mantiene fuente de motores verificada → VM**, con GND común y retorno de potencia independiente. El puente H no aísla galvánicamente ni regula tensión.
- El usuario indica un multiconector desde 3V3 y una unión que comparte alimentación entre driver y MPU. La conexión compartida no demuestra por sí sola un error. No se confirmó por fotografía el pin real de cada extremo, la continuidad ni el consumo disponible. Colores no identifican voltajes.
- No se aprobó todavía alimentar el módulo MPU desconocido a un voltaje concreto: hay que identificar su regulador/pull-ups o documentación. No trasladar VCC a 5 V por ensayo; las líneas I2C deben permanecer compatibles con 3,3 V. No conectar VCC a GND.
- Se confirmó conteo manual de encoders, pero no consumo total/pull-ups/niveles medidos ni ruido con motores. No declarar que encender LED valida un módulo.
- Capturas posteriores muestran `flash read err, 988`, reinicios y fallo de carga/interrupción serial. No hay detección MPU exitosa documentada. No está demostrada la causa: revisar primero distribución/voltajes/cambios de cableado. No borrar memoria ni cambiar GPIO a ciegas.
- Si se aísla la MPU para diagnóstico, apagar todo y retirar sus cuatro conexiones eléctricas (VCC/GND/SDA/SCL), o desconectar su conector completo, para evitar alimentación por SDA/SCL. No hace falta desarmar el chasis ni retirar todo el robot. No dejar una placa apagada recibiendo UART de otra alimentada.
- El mapa utiliza GPIO2 y GPIO15 con restricciones de arranque; no fue una asignación libre de restricciones. Comprobar niveles al reset antes de reubicar señales. No se cambian esos pines automáticamente en este documento.

Orden de diagnóstico actual: apagar fuentes → foto/trazado del multiconector y etiquetas VCC/VM → continuidad de distribución sin energía → medida de 3V3 respecto a GND (solo tras descartar corto/conexión incorrecta y evitando puentear pines con puntas) → aislamiento local de MPU si el fallo persiste → nueva carga solo con alimentación y arranque estables. Si se mide continuidad/resistencia, hacerlo siempre sin USB/baterías/fuentes.

- ESP32 DevKitC V4 / ESP-WROOM-32D: cuatro motores independientes, cuatro entradas de encoder, MPU6050, control local y parada por pérdida de órdenes.
- ESP32-S3 N16R8 con conector de cámara: cámara, comunicación con computador y puente de órdenes/telemetría por UART.
- D6: sin asignación hasta identificar modelo, alimentación e interfaz.

## Driver izquierdo → DevKitC V4

| TB6612FNG | GPIO | Función |
|---|---:|---|
| AIN1 | 13 | Dirección FL |
| AIN2 | 14 | Dirección FL |
| PWMA | 25 | Velocidad FL |
| BIN1 | 26 | Dirección RL |
| BIN2 | 27 | Dirección RL |
| PWMB | 32 | Velocidad RL |
| STBY | 2 | Habilitación común de ambos drivers |
| VCC | 3V3 | Lógica |
| GND | GND | Tierra común |
| VM | pendiente | Fuente de motores con tensión verificada |

AO1/AO2: rojo/negro FL. BO1/BO2: rojo/negro RL. Colores no garantizan sentido de avance.

## Driver derecho → DevKitC V4

| TB6612FNG | GPIO | Función |
|---|---:|---|
| AIN1 | 18 | Dirección FR |
| AIN2 | 19 | Dirección FR |
| PWMA | 23 | Velocidad FR |
| BIN1 | 4 | Dirección RR |
| BIN2 | 15 | Dirección RR; pin de arranque, ver restricciones |
| PWMB | 33 | Velocidad RR |
| STBY | 2 | Habilitación común de ambos drivers |
| VCC | 3V3 | Lógica |
| GND | GND | Tierra común |
| VM | pendiente | Misma fuente regulada de motores |

AO1/AO2: rojo/negro FR. BO1/BO2: rojo/negro RR.

No unir AIN con BIN ni PWMA con PWMB. STBY, VCC y GND sí se distribuyen en paralelo. Compartir STBY permite detener todos los motores sin compartir velocidad ni dirección.

## Encoders → DevKitC V4

| Encoder | DO/OUT a GPIO |
|---|---:|
| FL | 34 |
| RL | 35 |
| FR | 36 (VP) |
| RR | 39 (VN) |

GND a tierra común. VCC depende del módulo exacto: no asumir alimentación compatible por el nombre LM393. Verificar que DO no supere 3,3 V. Si el módulo funciona correctamente a 3,3 V y su salida es compatible, usar 3V3; si necesita 5 V, adaptar la salida antes del GPIO. AO no se usa. GPIO34–39 son solo entradas y no tienen pull-up/pull-down internos; comprobar el pull-up del módulo. No conectar OUT a drivers ni a motores.

Un encoder óptico de un canal cuenta pulsos, no mide dirección por sí solo. La dirección inferida de la orden no detecta movimiento contrario, deslizamiento o rueda empujada.

GPIO36/39: no usar ADC ni ahorro de energía Wi-Fi/Bluetooth en este controlador durante el conteo. Se propone radio desactivada y UART cableada.

## MPU6050 → DevKitC V4

SDA → GPIO21; SCL → GPIO22; GND → GND. VCC según módulo verificado; mantener SDA/SCL y pull-ups a 3,3 V. AD0 a GND si el módulo lo permite para dirección 0x68; no alimentar módulos desconocidos sin revisar su placa. INT, XDA y XCL sin conectar en esta etapa. IMU fija y ejes documentados.

## UART entre placas

| Origen | Destino |
|---|---|
| S3 GPIO41 (TX propuesto) | DevKitC GPIO16 (RX) |
| DevKitC GPIO17 (TX) | S3 GPIO42 (RX propuesto) |
| GND S3 | GND DevKitC |

GPIO16/17 disponibles en WROOM-32D, no extrapolar a WROVER. GPIO41/42 del S3 son propuesta basada en placa de cámara de referencia; fabricante/esquema exacto del S3 no confirmado. Validar UART con motores sin alimentación antes de adoptar este mapa como probado. No conectar 3V3 de una placa con 3V3 de la otra. Señales UART de 3,3 V. Cables cortos, alejados de potencia; ambos equipos encendidos durante comunicación para evitar alimentación por señales.

No usar TX/RX impresos del DevKitC (GPIO1/3): se reservan para USB/programación. UART de aplicación se configura en 16/17. El computador puede controlar y recibir telemetría mediante el S3; la conexión de cámara/ROS aún requiere software.

## Arranque y restricciones

- GPIO2 → STBY: mantener bajo durante reset y programación; nunca puente a 3V3. El TB6612 tiene pull-down interno, pero verificar que el breakout no incorpore pull-up o puente STBY-VCC. Recomendada resistencia externa 10 kΩ de STBY a GND si se dispone de ella; no figura en inventario. No activar VM hasta comprobar STBY bajo durante arranque.
- GPIO15 → BIN2 RR: su nivel durante reset puede suprimir mensajes ROM. No añadir pull-up/pull-down sin revisar; el pull-down de la entrada TB6612 puede cambiar esos mensajes y no implica por sí solo un fallo de firmware. Motors deshabilitados mediante STBY mientras arranca.
- No usar GPIO0, 5, 12 ni D0/D1/D2/D3/CMD/CLK para este mapa. GPIO6–11 son memoria. EN es reset.
- Si el breakout modifica niveles de arranque, revisar el mapa; no eliminar protección ni forzar encendido.

## Alimentación pendiente de medidas

Batería (+) → fusible → interruptor → distribución → entradas de reguladores.

- Rama motores: LM2596/XL4016 elegido según tensión de batería, corriente real y motor; salida positiva → VM de ambos drivers; negativa → tierra común.
- Rama lógica: regulador con salida 5 V verificada → entradas 5V de ambas placas, solo cuando se compruebe compatibilidad de las placas reales. No usar alimentación externa simultánea con USB hasta verificar protección frente a retorno de corriente.
- 3V3 DevKitC → VCC de ambos drivers por distribución; sensores únicamente tras comprobar compatibilidad y presupuesto de corriente del regulador.
- Tierra común para placas, drivers, sensores y fuente, sin hacer circular retorno de corriente de motores por cables finos del ESP32/protoboard.
- Desacoplo de referencia Toshiba, por CADA driver: 10 µF en paralelo con 100 nF entre VM y GND, y 10 µF en paralelo con 100 nF entre VCC y GND, próximos al módulo. Revisar los condensadores ya incorporados antes de añadir otros. Electrolítico positivo a VM/VCC y negativo a GND; cerámico sin polaridad. Comprobar tensión nominal superior a la máxima prevista, incluidos transitorios. Capacidad adicional por determinar según ruido y pruebas. El inventario no confirma los valores disponibles.
- Revisar corriente del motor al arrancar y bloquearse, y capacidad continua y térmica del driver: el pico breve de la ficha no autoriza esa corriente continua. Tampoco elegir regulador por el amperaje publicitario sin considerar disipación, cableado y carga simultánea de cuatro motores. No dimensionar fusible o tensión de motores por su color o apariencia.
- No poner batería directamente en 3V3, VCC de drivers ni GPIO.

## Secuencia física

1. Batería y USB desconectados: fijar DevKitC junto a drivers; S3 cerca de cámara; IMU centrada. Ordenar cables y conservar acceso USB.
2. Verificar salidas de motores ya montadas: canal A delantero, B trasero, un motor por canal.
3. Distribuir GND y VCC lógico. Cablear las doce señales según tablas y STBY común.
4. Preparar OUT de encoders etiquetados; conectar solo después de medir/verificar su alimentación y salida.
5. Conectar IMU después de revisar niveles I2C.
6. Preparar UART cruzada; comprobar señales/telemetría con VM desconectado y firmware de arranque seguro.
7. Medir batería y reguladores sin cargas; completar fusible, tensión y alimentación.
8. Con ruedas elevadas, probar un motor por vez y parada por pérdida de comunicación. Corregir sentido en software o intercambiando cables con todo apagado.
9. Probar encoders manualmente y luego junto a cada motor; calibrar pulsos por vuelta antes del PID.

El plano Draw.io anterior no representa este reparto y debe tratarse como esquema conceptual antiguo.

## Fuentes

- https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html
- https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG

Inventario actualizado por usuario: se añadió DevKitC V4 / ESP-32D. No se considera comprado ningún otro controlador mencionado antes.

## Orientación para localizar los GPIO del DevKitC

Vista de componentes, módulo metálico/antena arriba y USB abajo. Comparar primero con la serigrafía de la placa real; no contar pines a ciegas. En las fotografías coincide con esta distribución de referencia:

| De arriba hacia abajo | Lado izquierdo | Lado derecho |
|---:|---|---|
| 1 | 3V3 | GND |
| 2 | EN | 23 |
| 3 | VP (36) | 22 |
| 4 | VN (39) | TX (1), reservado USB |
| 5 | 34 | RX (3), reservado USB |
| 6 | 35 | 21 |
| 7 | 32 | GND |
| 8 | 33 | 19 |
| 9 | 25 | 18 |
| 10 | 26 | 5, sin usar |
| 11 | 27 | 17 |
| 12 | 14 | 16 |
| 13 | 12, sin usar | 4 |
| 14 | GND | 0, sin usar |
| 15 | 13 | 2 |
| 16 | D2, memoria | 15 |
| 17 | D3, memoria | D1, memoria |
| 18 | CMD, memoria | D0, memoria |
| 19 | 5V | CLK, memoria |

La numeración de la primera columna es solo la fila de esta tabla. GPIO2 NO significa la fila 2.

## Lista de cables etiquetados

Usar conectores firmes y aislados; en señales con headers macho puede usarse Dupont hembra-hembra. En distribución, una bornera/protoboard para lógica o una unión soldada aislada; no meter dos conectores en un solo pin. Corriente de motores por conexiones adecuadas a la corriente medida, no por rieles de protoboard.

| Etiqueta | Desde | Hasta | Motivo |
|---|---|---|---|
| I1 | DevKitC 13 | IZQ AIN1 | Dirección motor FL, entrada 1 |
| I2 | DevKitC 14 | IZQ AIN2 | Dirección motor FL, entrada 2 |
| I3 | DevKitC 25 | IZQ PWMA | PWM exclusivo FL |
| I4 | DevKitC 26 | IZQ BIN1 | Dirección RL, entrada 1 |
| I5 | DevKitC 27 | IZQ BIN2 | Dirección RL, entrada 2 |
| I6 | DevKitC 32 | IZQ PWMB | PWM exclusivo RL |
| D1 | DevKitC 18 | DER AIN1 | Dirección FR, entrada 1 |
| D2 | DevKitC 19 | DER AIN2 | Dirección FR, entrada 2 |
| D3 | DevKitC 23 | DER PWMA | PWM exclusivo FR |
| D4 | DevKitC 4 | DER BIN1 | Dirección RR, entrada 1 |
| D5 | DevKitC 15 | DER BIN2 | Dirección RR, entrada 2; restricción de arranque |
| D6 | DevKitC 33 | DER PWMB | PWM exclusivo RR; etiqueta de cable, no sensor D6 |
| S1 | DevKitC 2 | STBY IZQ y DER en Y | Deshabilitar los cuatro motores |
| L1 | Distribución 3V3 DevKitC | VCC IZQ | Lógica compatible con GPIO |
| L2 | Misma distribución | VCC DER | Alimentación lógica compartida |
| G1 | Distribución GND | GND IZQ | Referencia y retorno del driver |
| G2 | Distribución GND | GND DER | Referencia y retorno del driver |
| E1 | LM393 FL OUT | DevKitC 34 | Conteo FL |
| E2 | LM393 RL OUT | DevKitC 35 | Conteo RL |
| E3 | LM393 FR OUT | DevKitC VP/36 | Conteo FR |
| E4 | LM393 RR OUT | DevKitC VN/39 | Conteo RR |
| EG1–EG4 | Cada LM393 GND | GND lógica | Referencia de los pulsos |
| EV1–EV4 | Cada LM393 VCC | Rama compatible por confirmar | No conectar hasta verificar salida |
| U1 | DevKitC 21 | MPU6050 SDA | Datos I2C |
| U2 | DevKitC 22 | MPU6050 SCL | Reloj I2C |
| UG | MPU6050 GND | GND lógica | Referencia I2C |
| UV | MPU6050 VCC | Rama compatible por confirmar | Evitar pull-ups I2C a 5 V |
| UA | MPU6050 AD0 | GND si está disponible y permitido | Dirección I2C 0x68 |
| C1 | S3 41 TX propuesto | DevKitC 16 RX | Órdenes de movimiento |
| C2 | DevKitC 17 TX | S3 42 RX propuesto | Telemetría y estado |
| C3 | S3 GND | GND lógica común | Referencia UART |
| P1 | Regulador motores OUT+ | VM IZQ | Potencia de FL/RL |
| P2 | Mismo OUT+ | VM DER | Potencia de FR/RR |

Las etiquetas D1/D2/D3 de cables NO son los pines D1/D2/D3 de memoria de la placa: esos pines físicos no se usan. Para evitar confusión, anotar siempre también la pareja GPIO → pin driver en la etiqueta.

## Cómo funciona y qué significa cada grupo

- IN1/IN2 seleccionan giro, parada o frenado. PWM regula la activación del canal; no garantiza RPM por sí solo. El encoder aporta la medición para ajustar después con control de velocidad.
- AO1/AO2 y BO1/BO2 son pares de potencia con polaridad reversible. Ningún cable de motor va permanentemente a GND. Un canal, un motor.
- VCC del TB6612 alimenta lógica, VM alimenta motores. Son redes distintas. Poner 5 V en VCC no es la elección de este diseño: con VCC 3,3 V los niveles de control quedan coherentes con el ESP32.
- GND común no significa usar un mismo cable fino para toda la corriente. Separar los retornos de drivers de los de lógica hasta la distribución común.
- El computador procesa visión/navegación; el S3 transporta órdenes; DevKitC controla motores y cuenta pulsos. La cámara no tiene que enviar imágenes por la UART entre ESP32.
- Parada por pérdida de órdenes, STBY bajo antes de habilitar y cuatro PWM inicialmente a cero son requisitos del firmware futuro; el cableado solo no los implementa.

## Completitud y límites

| Parte | Estado |
|---|---|
| Doce señales independientes de motores | Asignadas sin duplicar GPIO |
| STBY compartido | Asignado; comprobar nivel de reset y breakout |
| Cuatro OUT de encoder | Entradas asignadas; nivel/alimentación pendientes |
| I2C MPU6050 | Señales asignadas; módulo y pull-ups pendientes |
| UART DevKitC | GPIO16/17 asignados para WROOM-32D |
| UART del S3 | GPIO41/42 propuestos; verificar con cámara funcionando |
| Cámara | Conector propio del S3; modelo/pinout exactos pendientes |
| Sensor D6 | Sin conectar: falta identificarlo |
| Alimentación y protecciones | Topología definida; valores/capacidad sin cerrar |
| Simulación/prueba real | No realizadas |

No se puede llamar listo para energizar al conjunto hasta resolver estas celdas pendientes. No se requieren compras automáticas: primero revisar los módulos y componentes disponibles.
```

### `REVISION_CABLEADO.md`

```markdown
# Revisión de indicaciones y cableado

Revisión 2026-10-29. Referencia activa: MAPA_CONEXIONES.md. No certifica el montaje físico: faltan fotos de los extremos y medidas.

## Lo que sí se mantiene

| Conexión | Evaluación |
|---|---|
| 3V3 DevKitC → VCC de dos TB6612 | Correcta para lógica compatible; verificar breakout y corriente total del regulador |
| Multiconector divide 3V3 en ramas | Válido si contactos firmes, aislados, con continuidad correcta y sin sobrecarga |
| XL4016/fuente motores → VM de drivers | Topología adecuada; tensión y corriente deben verificarse antes de conectar |
| AO1/AO2 y BO1/BO2 → un motor por canal | Correcto; ningún cable motor directamente a 3V3/GPIO |
| GND común | Necesario; retorno motor no debe pasar por cable fino de ESP/IMU |
| Encoders D0→34,35,VP36,VN39 | Adecuado para entradas; usuario confirmó prueba manual |
| MPU SDA21/SCL22 | Asignación adecuada; no identifica alimentación del módulo |
| UART16/17 del DevKitC | Adecuado para WROOM; llegada de mensajes confirmada, no ruido con motores/cámara |

## Qué estuvo mal, fue incompleto o necesitó corrección

1. Se presentó demasiado pronto un mapa como completo mientras faltaban valores de alimentación, características de breakouts y pines reales de cámara. Las láminas son propuestas, no un esquema verificado listo para energizar.
2. Se sugirió repartir 3V3 entre drivers y sensores sin cerrar presupuesto de corriente y fichas de módulos. Compartir no es el error por sí mismo; faltó verificar carga y niveles antes de energizarlo todo. La MPU sigue sin identificar físicamente.
3. La primera propuesta unía PWM/dirección por lado; no satisfacía el objetivo posterior de independencia por motor. Ya fue sustituida por doce señales separadas. No conservar puentes PWMA/PWMB o AIN/BIN del diseño agrupado.
4. Se usaron GPIO2 y GPIO15, que son de arranque: necesitan condiciones documentadas. GPIO2/STBY no debe estar forzado alto para programar; GPIO15 bajo puede silenciar ROM. No atribuirles todos los fallos sin medirlos.
5. Se describió inicialmente que el encoder indicaría sentido y se mencionaron imanes como si LM393 definiera tecnología. LM393 es un comparador presente en distintos módulos; el sensor óptico de una salida con disco regular no determina sentido por sí solo. Contar pulsos no equivale a odometría validada.
6. Se planteó retirar solo VCC de la MPU como prueba. Para aislamiento eléctrico fiable, desconectar también SDA/SCL, evitando alimentación por señales. GND puede mantenerse como referencia, pero retirar conector completo es la comprobación más clara.
7. Se asoció el código abierto en IDE con el grabado en placa. No son lo mismo. Los mensajes de ejecución y la carga verificada son la evidencia.
8. Un archivo copiado quedó truncado. Se reparó y se comprobó compilación; ese error de llaves era independiente de los fallos de arranque/carga.

## Solución de alimentación prevista

Durante diagnóstico: USB como única entrada de alimentación de cada placa; batería, fuentes externas y VM apagados. Mantener ambas placas alimentadas si sigue unida UART. El rail 3V3 del DevKitC puede alimentar VCC de drivers; no conectar otra fuente activa a ese mismo rail.

Montaje final: batería protegida alimenta dos ramas reguladas, con negativos de reguladores no aislados en distribución común. Rama motores a VM; rama lógica verificada a entradas adecuadas de ambas placas. Dejar al regulador de cada placa generar su 3V3, salvo rediseño explícito. No unir 3V3 S3 con 3V3 DevKitC. No alimentar por USB y fuente externa simultáneamente sin resolver retorno de corriente.

Si la carga total excede el regulador, se debe rediseñar la rama de sensores a una fuente apropiada y verificar niveles. No se propone comprar ni agregar reguladores sin medir ni se autoriza unir salidas de reguladores.

Para sensor con alimentación 5 V, verificar regulación propia, salida D0 o pull-ups I2C antes de conectarlo a GPIO. Un resistor en serie por sí solo no garantiza conversión de 5 a 3,3 V.

## Paso actual

Apagar y comprobar físicamente el multiconector: el cable 3V3 debe ir a VCC de drivers, no VM, GND, EN, GPIO12 o pines de memoria. Verificar la conexión de la MPU y la tensión real de la rama antes de nuevas cargas. No se ha demostrado que el rail compartido sea la causa ni que ninguna placa esté dañada.

## Fuentes

- Toshiba TB6612FNG: https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG
- ESP32 DevKitC V4: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html
- Arranque ESP32: https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html
```

### `AGENTS.md`

````markdown
# AGENTS.md

## 1. Contexto del proyecto

Este repositorio corresponde al proyecto de la asignatura **Programación Distribuida y Paralela**.

El objetivo general es desarrollar un **robot móvil 4WD con percepción visual, sensores y capacidades de navegación**, usando una arquitectura distribuida basada principalmente en **ROS 2**.

La idea final del proyecto es que el robot pueda interpretar instrucciones de alto nivel como:

> "Ve a la izquierda de las gafas."

y combinar varias capacidades para ejecutarlas:

- detección visual de objetos;
- percepción del entorno;
- localización y/o construcción de mapas;
- navegación;
- control de motores;
- lectura de sensores;
- comunicación entre distintos procesos o nodos.

El proyecto comenzó con una propuesta mucho más simple basada en un ESP32, una cámara y un chasis móvil. Con el avance del diseño se añadieron sensores y componentes orientados a lograr una plataforma robótica más completa.

Este documento sirve como **contexto y guía para agentes de IA que trabajen sobre el repositorio**.  
No debe interpretarse como una especificación rígida ni como una arquitectura inmodificable.

Los archivos sincronizados bajo `sources/` son material de referencia y deben conservarse sin cambios.

---

## 2. Principio general para trabajar en este repositorio

Antes de realizar cambios importantes:

1. Revisar la estructura actual del repositorio.
2. Identificar qué partes ya están implementadas.
3. No asumir que toda la arquitectura descrita aquí ya existe.
4. No introducir dependencias, frameworks o componentes nuevos sin una razón técnica clara.
5. Preferir soluciones simples, modulares y verificables.
6. Mantener separadas las responsabilidades de percepción, navegación, control y hardware.
7. Cuando sea posible, permitir que el mismo software de alto nivel pueda funcionar tanto con simulación como con el robot real.

La arquitectura descrita más adelante es una **referencia deseada**, pero puede cambiar conforme avance el proyecto.

---

## 3. Hardware actual o previsto

### Prueba confirmada y corrección bidireccional — 2026-11-24

- El usuario confirmó éxito de la prueba autónoma V1.1 con objetos cercanos, con límites: solo izquierda y paradas que exigían ARRANCAR. Respaldados los15 archivos anteriores en `prueba_autonoma_exitosa_2026-11-24`, manifiestoSHA256 y nota de evidencia. No modificar ese respaldo ni las pruebas exitosas anteriores.
- Desarrollo actual en `navegacion_autonoma` V1.2: comparación de ambos lados, sentido angular físico configurable y obligatorio en la página, giros izquierda/derecha con cuatro ruedas. El tiempo6,5s ya no cancela la corrida: reevalúa otro lado con pausa. Recupera automáticamente espera de espacio o barridoLiDAR inválido/caducado tras tres barridos nuevos confirmados, con motores a cero mientras no haya datos/salida. Sin bajar distancias ni convertir retornos desconocidos en espacio libre.
- Video interrumpido ya no cancela una navegación con LiDAR y operador sanos. Navegador, UART, MPU y encoder siguen teniendo protección de parada enclavada, sin forzar rearme automático. La página conserva Última parada. DevKit identifica cuál encoder FL/RL/FR/RR dejó de contar. Cargar ambas placas para diagnóstico completo; protocoloV1/A1 compatible con el DevKit autónomo.
- Anticipación150mm adicional al radio150 y margen100. No significa permitir un obstáculo a150mm del sensor. Sigue siendo algoritmo reactivo, no aprendizajeRL ni garantía de ausencia de choque.
- Nuevas pruebas:28 combinaciones de frente/sentido para ambos lados, escenas variables,32 combinaciones de salud, integración planificador→CRC/UART→sketchDevKit con dobles y GPIO reales simulados, cuatro encoders e interfaz. `python navegacion_autonoma/pruebas/verificar.py`. Ver LEEME para tabla de recuperación. V1.2 requiere validación física; no describir las pruebas dePC como prueba del vehículo.

### Corrección de reanudación — 2026-11-24

- Usuario informa parada al corregir trayectoria. Falta motivo del panel para atribuir la causa real; consulta HTTP a /estado no accesible durante la revisión.
- `navegacion_autonoma` S3 V1.1: salida de giro envía avance directamente; obstáculo temporal o celda necesaria sin observar pasa a `ESPERANDO_ESPACIO`, conserva corrida y reanuda tras tres barridos nuevos que confirmen el mismo tipo de salida. STOP y fallos de sensores, comunicación, MPU, encoders o giro sin salida siguen cancelando. Pausas breves por inversión de motores permanecen.
- Anticipación V1.1 ajustada por petición del usuario a150mm (más radio150 y margen100, no umbral de150mm desde el sensor). La corrección usa las cuatro ruedas. Protocolo y DevKit sin cambios: para V1.1 recargar solo S3 con sus encabezados. Respaldo previo verificado en `respaldo_autonomia_antes_reanudacion_2026-11-24`. No confundir tests de software con validación física.
- Los 50 cm sugeridos para identificar la caja frontal no son obligatorios ni determinan el ángulo. Radio real informado150mm; captura anterior mostraba radio500 aplicado aunque formulario150: verificar configuración confirmada.

### Actualización de montaje y firmware — 2026-11-14

- Reparto actual: DevKitC controla cuatro motores, encoders y MPU6050; S3 controla cámara OV5640, COIN-D6 y página Wi-Fi. Las referencias posteriores a un único S3 son conceptuales o históricas.
- Capturas recientes confirman cámara, UART, MPU y LiDAR activos simultáneamente. D6 V3 con checksum del SDK: más de 31000 paquetes, cero errores, 68/72 sectores. No confundir esto con una prueba de evasión física completada.
- LiDAR: amarillo TX a S3 RX GPIO1; verde RX a S3 TX GPIO2; rojo a 5V regulados; negro a GND común. UART a 230400. UART entre placas permanece S3 TX41/RX42 y DevKit RX16/TX17 a 115200.
- Usuario confirmó XL4016 regulado a 5V y decidió usarlo como único regulador. Conservar niveles lógicos y cableado ya probado; no volver a proponer LM2596 ni repetir mediciones de tensión sin un fallo nuevo.
- Antes de modificar se copiaron y verificaron por SHA256 los ocho archivos de `prueba_integral_50cm` a `primera_prueba_exitosa_2026-11-14`. Conservar ambos sin cambios. Respaldo solicitado por el usuario como primera prueba exitosa; no afirmar que el desplazamiento físico de 50 cm fue certificado.
- Firmware nuevo en `navegacion_autonoma`: ARRANCAR/STOP, avance sin límite de distancia, evasión reactiva girando a la izquierda con cuatro salidas firmadas. Protocolo UART V1/A1: cargar AMBAS placas, no mezclar con firmware integral anterior. Mantiene paradas por fallos y ausencia de maniobra; no hay OTA, SLAM ni Nav2.
- Geometría: usuario declara LiDAR centrado y 15 cm hasta cada punta. Formulario propone radio de 150 mm; ángulo frontal pendiente de identificar en radar y confirmar antes de arrancar. Se pierde la confirmación al reiniciar S3. No asumir frente=0 grados.
- Planificador usa 36 celdas de 10 grados, mínimo real de cada par de sectores, sin borrar ecos cercanos. Desconocidos bloquean la maniobra que requiere esa zona. Radio + 100 mm de margen inicial y 500 mm de anticipación, pendientes de validar físicamente. Capturas anteriores mostraban ecos a 54–71 mm: no ocultarlos para habilitar marcha.
- Estado del nuevo firmware: compilación y pruebas automatizadas de protocolo, planificación, GPIO/UART/I2C simulados e interfaz. Pendiente carga y validación física de los sentidos de giro, margen de parada y evasión.

### Actualización de montaje — 2026-10-29

- Hay dos placas físicas: ESP32 DevKitC V4 con módulo ESP-32D/WROOM-32D para motores, encoders y MPU6050; ESP32-S3 anterior para cámara y comunicación. Este reparto sustituye las asignaciones antiguas al S3 que aparecen como arquitectura conceptual más abajo.
- Mapa de cableado actual: `MAPA_CONEXIONES.md`. Es diseño parcialmente probado; no asumir alimentación, sensores ni motores ya validados.
- Prueba UART `pruebas_uart/`: en capturas se confirmó DevKitC ejecutando receptor y recibiendo HOLA del S3 de manera repetida (contador al menos 12). Falta evidencia del monitor S3 confirmando RECIBIDO; no declarar validado ida y vuelta, cámara simultánea o ruido con motores.
- Puertos vistos: DevKitC COM5; S3 COM6. Pueden cambiar al reconectar.
- Diagnóstico actual: prueba MPU aún sin éxito; inicialmente no respondió en I2C y, después de cambios de alimentación, hubo capturas con `flash read err, 988` y reinicios/carga interrumpida. El usuario informa multiconector desde 3V3 compartido por driver/MPU. Esto no demuestra conexión incorrecta ni sobrecarga: revisar físicamente VCC frente a VM, continuidad y tensiones antes de reintentar cargas. Auditoría/correcciones en `REVISION_CABLEADO.md`; no cambiar GPIO ni mover sensores a 5 V sin verificar módulo y niveles.
- Prueba `pruebas_encoders/devkitc_encoders/devkitc_encoders.ino`: compilada y cargada por el usuario. Inicialmente falló carga (Wrong boot mode 0x13 / interrupción serial); BOOT sostenido + pulso EN permitió DOWNLOAD_BOOT y luego escritura verificada. El usuario confirmó que la prueba manual funciona; captura con TOTAL FL=80, RL=55, FR=140, RR=60 y NUEVOS=0 en reposo al final. Confirma conteo básico, no calibración, ausencia de ruido con motores, dirección ni distancia. VM/batería sin alimentar en esta fase. Siguiente etapa: prueba MPU6050 antes de energizar motores.
- UART cableada propuesta: S3 TX41→DevKitC RX16, DevKitC TX17→S3 RX42 y GND común. No unir salidas 3V3. Niveles de encoders/IMU y alimentación de motores siguen pendientes de evidencia técnica.

### Computación y control

- **ESP32-S3**
  - Microcontrolador principal del robot.
  - Se encargará principalmente de tareas de bajo nivel.
  - Control de motores.
  - Lectura de sensores.
  - Lectura de encoders.
  - Comunicación con el computador y/o ROS 2.

### Cámara

- **Módulo de cámara OV56** asociado al ESP32-S3. El modelo exacto no está confirmado en este documento.
- Su función principal será proporcionar imágenes para visión artificial.

La inferencia pesada de visión artificial no necesariamente debe ejecutarse en el ESP32.  
Puede ejecutarse en un computador externo y utilizar al ESP32 como nodo/controlador de hardware.

### Chasis

- Chasis móvil **4WD**.
- Cuatro ruedas.
- Cuatro motores DC.
- Movimiento principalmente mediante control diferencial.

Conceptualmente:

```text
lado izquierdo                  lado derecho

Motor FL ─┐                     ┌─ Motor FR
          ├── mismo sentido ────┤
Motor RL ─┘                     └─ Motor RR
```

El giro se realiza modificando la velocidad y/o sentido relativo entre ambos lados.

### Drivers de motores

- **2 × TB6612FNG**, cada uno con dos canales.

Cada driver controla dos motores.

Distribución orientativa:

```text
TB6612FNG #1
 ├── Motor delantero izquierdo
 └── Motor trasero izquierdo

TB6612FNG #2
 ├── Motor delantero derecho
 └── Motor trasero derecho
```

Esta distribución puede cambiar si el diseño físico o eléctrico lo requiere.

### Encoders

- **4 × encoders LM393** asociados a las ruedas/motores.

Objetivos:

- medir movimiento de las ruedas;
- estimar velocidad;
- apoyar la odometría;
- eventualmente implementar control cerrado de velocidad.

### IMU

- **MPU6050**

Objetivos principales:

- aceleración;
- velocidad angular;
- apoyo a estimación de orientación;
- posible fusión con odometría de ruedas.

### Sensor de distancia

- **Sensor D6** proporcionado por el profesor. Verificar sus características e interfaz antes de integrarlo.

Objetivos previstos:

- obtener distancias al entorno;
- detección geométrica de obstáculos;
- alimentar SLAM si sus datos e interfaz lo permiten;
- apoyar navegación autónoma.

### Alimentación

El sistema utiliza baterías y regulación de tensión.

Entre los componentes disponibles se encuentran:

- reguladores DC-DC **LM2596** y **XL4016**;
- baterías **18650** con protección integrada;
- condensadores **cerámicos** y **electrolíticos**;
- interruptor de **balancín**;
- **fusibles y portafusible en línea**;
- distribución de alimentación separando adecuadamente lógica y motores.

No modificar configuraciones eléctricas únicamente basándose en este documento.  
Consultar siempre el esquema eléctrico vigente del proyecto.

---

## 4. Arquitectura conceptual

La arquitectura de referencia es aproximadamente:

```text
                      ┌──────────────────────────────┐
                      │          COMPUTADOR          │
                      │                              │
Cámara ──────────────►│ OpenCV / VLM                │
Instrucción ─────────►│ percepción e interpretación│
                      │                              │
Sensor D6 ───────────►│ ROS 2                       │
                      │ SLAM / localización          │
                      │ Nav2 / navegación            │
                      │ lógica de alto nivel         │
                      │ objetivos validados         │
                      └──────────────┬───────────────┘
                                     │
                                 /cmd_vel
                              comunicación
                                     │
                                     ▼
                      ┌──────────────────────────────┐
                      │          ESP32-S3            │
                      │                              │
                      │ control de motores           │
                      │ lectura de encoders          │
                      │ lectura de IMU               │
                      │ comunicación con ROS 2       │
                      └──────────────┬───────────────┘
                                     │
                         ┌───────────┴───────────┐
                         ▼                       ▼
                    TB6612FNG                TB6612FNG
                         │                       │
                  motores izquierdos      motores derechos
```

Este diagrama representa una **separación conceptual de responsabilidades**.

No significa que todos los procesos deban ejecutarse exactamente en esos dispositivos ni que todos los nodos deban tener esos nombres.

---

## 5. Separación de responsabilidades

La arquitectura ideal intenta dividir el sistema aproximadamente en cuatro capas:

```text
PERCEPCIÓN        LOCALIZACIÓN       DECISIÓN        CONTROL
    │                  │                │               │
 cámara/VLM         SLAM/odom       Nav2/lógica       ESP32
    │                  │                │               │
    └──────────────────┴────────────────┴───────────────┘
                              ROS 2
```

### Percepción

Puede incluir:

- cámara;
- OpenCV;
- VLM para percepción visual e interpretación de instrucciones;
- sensor D6;
- procesamiento de sensores.

Responsabilidad:

> Convertir datos del mundo físico en información útil para el resto del sistema.

---

### Localización y representación del entorno

Puede incluir:

- odometría;
- encoders;
- IMU;
- sensor D6;
- SLAM;
- filtros de estimación de estado.

Responsabilidad:

> Determinar dónde está el robot y cómo es el entorno que lo rodea.

---

### Decisión y navegación

Puede incluir:

- ROS 2;
- Nav2;
- planificador de rutas;
- lógica propia;
- objetivos estructurados y validados a partir de la salida del VLM.

Responsabilidad:

> Transformar objetivos de alto nivel en acciones de navegación.

---

### Control

Principalmente asociado al ESP32.

Responsabilidades:

- recibir comandos de movimiento;
- controlar motores;
- leer encoders;
- leer sensores de bajo nivel;
- ejecutar lazos de control;
- informar estado al sistema superior.

---

## 6. ROS 2

ROS 2 es la capa de integración principal prevista para el sistema.

Siempre que tenga sentido, los componentes deben comunicarse mediante interfaces bien definidas.

Ejemplos orientativos:

```text
/cmd_vel
/odom
/scan
/imu
/camera/image_raw
```

Los nombres concretos pueden variar.

### Idea importante

El software de alto nivel debería depender de interfaces y no directamente del hardware.

Por ejemplo:

```text
Nav2
 │
 ▼
/cmd_vel
 │
 ├── Gazebo / robot simulado
 │
 └── ESP32 / robot real
```

Esto permite reutilizar una mayor parte del sistema entre simulación y hardware real.

---

## 7. Gazebo

Gazebo se utilizará, si resulta útil para el desarrollo, como simulador del robot.

La idea no es escribir "código para Gazebo" y después copiarlo al ESP32.

La estrategia deseada es:

1. Crear un modelo aproximado del robot.
2. Simular sensores y movimiento.
3. Conectar el robot simulado con ROS 2.
4. Desarrollar y probar nodos de alto nivel.
5. Reemplazar posteriormente el hardware simulado por interfaces del robot real.

Ejemplo:

```text
                ROS 2
                  │
          ┌───────┴────────┐
          │                │
       Gazebo          Robot real
          │                │
 sensores simulados    sensores físicos
 motores simulados     ESP32 + motores
```

La simulación debe considerarse una herramienta de desarrollo, no una obligación si introduce una complejidad innecesaria para los objetivos académicos.

---

## 8. Visión artificial

### OpenCV

Puede utilizarse para:

- captura y manipulación de imágenes;
- conversiones de color;
- filtros;
- geometría;
- calibración;
- preprocesamiento;
- procesamiento posterior a una detección.

### VLM

Un **modelo de visión y lenguaje (VLM)** combina la imagen de la cámara con una instrucción para reconocer objetos relevantes e interpretar la intención. Por ejemplo, ante "Ve a la izquierda de las gafas", puede producir una salida estructurada como:

```text
acción: navegar
objeto_referencia: gafas
relación_espacial: izquierda
evidencia_visual: gafas identificadas en la imagen
```

La presencia de un objeto en la imagen no determina por sí sola una posición navegable. Otros componentes deben relacionar la observación con la localización del robot y el entorno, calcular un objetivo y ejecutar la navegación.

---

## 9. Instrucciones y percepción con VLM

Flujo conceptual:

```text
imagen de cámara + "Ve a la izquierda de las gafas"
                         │
                         ▼
                        VLM
             objeto e intención estructurados
                         │
                         ▼
          validación y posición objetivo
             con localización y sensores
                         │
                         ▼
                   Nav2 / lógica
                         │
                         ▼
                      /cmd_vel
                         │
                         ▼
                    ESP32-S3
                         │
                         ▼
                      motores
```

El VLM **no debe controlar directamente los motores**. Una capa intermedia debe validar su salida y traducirla a objetivos seguros y concretos. Si el objeto o la relación espacial no se pueden determinar con suficiente claridad, no se debe iniciar el movimiento basado en esa instrucción.

---

## 10. Programación distribuida y paralela

El proyecto debe aprovechar su naturaleza distribuida.

Ejemplos de procesos que podrían ejecutarse de forma independiente:

```text
Nodo de cámara
Nodo de VLM (visión e instrucciones)
Nodo del sensor D6
Nodo de IMU
Nodo de odometría
Nodo de SLAM
Nodo de navegación
Nodo puente con ESP32
```

No es obligatorio crear un nodo separado para cada función.

La división debe responder a una necesidad real de:

- modularidad;
- procesamiento paralelo;
- comunicación;
- independencia;
- reutilización;
- mantenimiento.

Evitar fragmentar el sistema artificialmente solo para aumentar el número de nodos.

---

## 11. Prioridades del proyecto

Al trabajar sobre el código, priorizar aproximadamente este orden:

```text
1. Robot puede moverse de forma controlada
          ↓
2. Comunicación estable computador ↔ ESP32
          ↓
3. Encoders / odometría
          ↓
4. IMU
          ↓
5. Sensor D6
          ↓
6. Integración ROS 2
          ↓
7. Simulación
          ↓
8. SLAM / localización
          ↓
9. Navegación autónoma
          ↓
10. Percepción visual e interpretación de instrucciones con VLM
          ↓
11. Relación objeto ↔ posición
```

Este orden es una recomendación y puede cambiar según las necesidades de la asignatura.

No intentar implementar todo simultáneamente.

---

## 12. Reglas para agentes de IA

Cuando un agente trabaje sobre este repositorio:

### Debe

- leer antes de modificar;
- explicar cambios arquitectónicos importantes;
- mantener modularidad;
- evitar acoplar visión, navegación y control de motores innecesariamente;
- reutilizar interfaces estándar de ROS 2 cuando tenga sentido;
- documentar supuestos;
- distinguir claramente código experimental de código funcional;
- priorizar implementaciones que puedan probarse por etapas;
- conservar compatibilidad entre simulación y robot real cuando sea razonable.

### No debe

- asumir que todo lo descrito en este archivo ya está implementado;
- reemplazar una arquitectura funcional solo porque exista una alternativa más sofisticada;
- instalar dependencias pesadas sin justificación;
- modificar configuración eléctrica o pines sin revisar el código/esquema vigente;
- inventar características de sensores o componentes;
- integrar el VLM con el movimiento antes de validar las capas básicas de percepción, navegación y control;
- introducir abstracciones complejas innecesarias.

---

## 13. Filosofía del proyecto

La meta no es construir el robot más complejo posible.

La meta es construir un sistema que:

- funcione;
- pueda explicarse;
- pueda demostrarse;
- esté dividido en componentes comprensibles;
- permita observar programación distribuida y paralela;
- pueda ampliarse progresivamente.

Preferir:

```text
simple + funcional + demostrable
```

sobre:

```text
sofisticado + frágil + difícil de explicar
```

---

## 14. Guías del profesor y adaptación al hardware real

Durante el semestre el profesor puede entregar guías, ejemplos, prácticas o procedimientos.

Estas guías deben utilizarse como **referencia funcional y académica**, pero no debe asumirse que el hardware utilizado en ellas coincide con el hardware de este proyecto.

Una guía puede estar diseñada para compañeros que tengan menos hardware, otro hardware o una configuración diferente.

Por tanto, el objetivo no es copiar ciegamente la guía, sino:

> entender qué se pretende lograr y determinar cómo implementarlo correctamente con el hardware que realmente tenemos.

---

### Regla principal

Ante una nueva guía:

```text
Guía del profesor
       │
       ▼
¿Qué funcionalidad o aprendizaje busca?
       │
       ▼
¿Qué hardware tenemos realmente?
       │
       ▼
¿Podemos implementar esa funcionalidad
con nuestro hardware actual?
       │
       ├── Sí → adaptar la implementación
       │        al hardware disponible
       │
       └── No → señalar la limitación
                y NO comprar, sustituir
                ni inventar hardware
                automáticamente
```

---

### Las guías NO redefinen automáticamente nuestro hardware

Si una guía utiliza, por ejemplo:

```text
ESP32
+
cámara
+
motores
```

pero nuestro robot además dispone de:

```text
sensor D6
IMU
encoders
mejores drivers
u otros componentes
```

no se deben ignorar automáticamente esos componentes adicionales.

Se debe evaluar si pueden utilizarse de manera útil para implementar mejor, con mayor precisión o de forma más robusta lo que pide la guía.

Ejemplo conceptual:

```text
La guía pide:
"mover el robot cierta distancia"

Compañeros:
tiempo de activación de motores

Nuestro proyecto:
encoders + odometría
```

Si los encoders permiten cumplir el mismo objetivo de una forma más apropiada para nuestro hardware, puede utilizarse esa solución siempre que no contradiga explícitamente la actividad.

---

### Tampoco se debe usar todo el hardware porque esté disponible

Disponer de un componente no significa que deba utilizarse en todas las prácticas.

Si una actividad puede resolverse de forma simple con:

```text
ESP32 + motores
```

no es obligatorio introducir:

```text
sensor D6 + VLM + SLAM + Nav2
```

solo porque estén contemplados en el proyecto.

Usar únicamente los componentes que aporten valor real a la tarea actual.

---

### Cuando una guía utiliza hardware que NO tenemos

Si una guía menciona un módulo, sensor, placa o componente que no poseemos:

**NO asumir automáticamente que hay que comprarlo.**

**NO buscar automáticamente un sustituto.**

**NO modificar la arquitectura para introducir un equivalente.**

Primero se debe determinar qué papel cumple ese componente dentro de la guía.

Después comprobar si:

1. la función puede realizarse con hardware que ya tenemos;
2. otra parte de nuestra arquitectura ya proporciona una capacidad equivalente;
3. esa sección puede omitirse sin impedir el objetivo principal de la práctica;
4. la actividad depende necesariamente de ese componente.

El resultado debe comunicarse claramente.

Ejemplo:

```text
La guía utiliza: sensor X

Nuestro robot: no tiene sensor X
       │
       ▼
Analizar qué hace sensor X
       │
       ├── La función puede cubrirla nuestro sensor D6
       │       → verificar sus capacidades y adaptar
       │
       ├── Esa parte es opcional o irrelevante
       │       → omitirla
       │
       └── Es indispensable y no tenemos equivalente
               → informar la limitación
                  y esperar decisión del usuario
```

---

### Prohibición de adquirir o sustituir hardware por iniciativa del agente

Un agente de IA que trabaje en este repositorio **no debe decidir que hace falta comprar hardware nuevo**.

Tampoco debe asumir que el usuario desea:

- comprar un módulo mencionado en una guía;
- reemplazarlo por otro componente;
- buscar un equivalente comercial;
- rediseñar el robot para adaptarlo a ese módulo;
- cambiar componentes ya comprados.

Eso solamente debe hacerse cuando **el usuario lo decida explícitamente**.

Hasta entonces, trabajar con:

```text
hardware disponible
+
software disponible
+
objetivo funcional de la guía
```

---

### Tratamiento recomendado de cada guía nueva

Cuando se reciba una guía nueva:

1. Identificar el objetivo de la práctica.
2. Separar los requisitos funcionales de los componentes concretos utilizados en el ejemplo.
3. Comparar esos requisitos con nuestro hardware actual.
4. Clasificar cada parte como:

```text
A. Aplicable directamente
B. Aplicable con adaptación
C. Ya cubierta por hardware mejor o diferente
D. No aplicable por falta de hardware
E. Prescindible para nuestro caso
```

5. Implementar A, B y C cuando corresponda.
6. Omitir D o E cuando sea razonable.
7. Si una parte D es indispensable para cumplir el objetivo, informarlo antes de proponer compras o sustituciones.
8. No buscar sustitutos de hardware salvo petición expresa del usuario.
9. Mantener el resto del robot funcional y evitar romper capacidades existentes por seguir literalmente una guía.

---

### Prioridad práctica

Cuando una guía y nuestra configuración difieran, pensar en este orden:

```text
1. ¿Qué quiere enseñar o conseguir la guía?
                 ↓
2. ¿Qué necesita realmente la actividad?
                 ↓
3. ¿Qué hardware tenemos?
                 ↓
4. ¿Cómo logramos el mismo objetivo con ese hardware?
                 ↓
5. ¿Hay partes de la guía que no aplican a nuestro robot?
                 ↓
6. Omitirlas o marcarlas como no aplicables.
```

La prioridad es cumplir correctamente el objetivo académico **con nuestro robot**, no reproducir exactamente el montaje de otro grupo.

---

### Hardware conocido actualmente

Mantener como referencia el inventario conocido del proyecto:

```text
ESP32-S3 con módulo de cámara OV56
chasis 4WD
ruedas
4 motores DC
4 encoders LM393
2 × TB6612FNG de dos canales
MPU6050
sensor D6 proporcionado por el profesor
LM2596
XL4016
condensadores cerámicos y electrolíticos
interruptor de balancín
baterías 18650 con protección integrada
fusibles y portafusible en línea
```

Este inventario puede cambiar durante el proyecto.

Cuando cambie, actualizar esta sección o la sección de hardware principal del documento.

## 15. Estado cambiante del proyecto

Este archivo describe el contexto acumulado, el hardware conocido y la dirección general del proyecto, pero **no es una fuente definitiva para cada detalle técnico**.

Debe conservarse como memoria del proyecto para evitar perder decisiones previas.

Las guías nuevas deben interpretarse junto con este contexto, no reemplazarlo automáticamente.

Ante discrepancias, considerar en conjunto:

```text
objetivo de la actividad
+
hardware realmente disponible
+
estado actual del código
+
decisiones recientes del equipo
```

Si el repositorio, el hardware o las decisiones del proyecto evolucionan significativamente, actualizar este documento.

---

## 16. Resumen rápido para un agente nuevo

Si acabas de entrar al repositorio, entiende esto primero:

> Estamos construyendo un robot móvil 4WD para Programación Distribuida y Paralela. El ESP32-S3 controla el hardware de bajo nivel. El robot tiene un módulo de cámara OV56, cuatro motores con encoders, dos TB6612FNG, MPU6050, sensor D6 y los componentes de alimentación indicados en el inventario. Un computador puede ejecutar ROS 2, percepción con VLM, localización y navegación. El VLM combina la imagen con instrucciones como "ve a la izquierda de las gafas" y entrega información para calcular un objetivo validado; Nav2 o la lógica de navegación y el ESP32-S3 se encargan del movimiento. Gazebo puede utilizarse para simular el robot. Esta arquitectura es una guía: revisar siempre el estado actual del repositorio antes de realizar cambios.
````

### `pruebas_uart/devkitc_receptor/devkitc_receptor.ino`

```cpp
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
```

### `pruebas_uart/LEEME.md`

```markdown
# Prueba UART entre los dos ESP32

Son dos programas separados. No pegarlos juntos. No requieren bibliotecas externas: solo el paquete ESP32 de Espressif para Arduino IDE.

Estado: fuentes revisadas; todavía sin compilación para las placas ni prueba física. No cuentan encoders, leen IMU ni inician cámara. El programa del DevKitC mantiene STBY y las doce señales de motor en bajo mientras se ejecuta; no protege el intervalo anterior a setup ni reemplaza la verificación de hardware. Mantener VM desconectado.

## Ahora: DevKitC V4 conectado

1. Batería/fuentes externas desconectadas, VM de ambos drivers sin alimentación. No usar USB y entrada 5V simultáneamente.
2. Con ambas placas apagadas, retirar temporalmente las dos señales UART (S3 41 a DevKitC 16 y S3 42 a DevKitC 17). GND puede permanecer.
3. Conectar únicamente DevKitC a USB. No conectar S3 todavía.
4. Abrir `devkitc_receptor/devkitc_receptor.ino` en Arduino IDE.
5. Gestor de placas: debe estar instalado `esp32 by Espressif Systems`. Placa `ESP32 Dev Module`. Puerto: el COM que aparece al enchufar el DevKitC, sin inventar su número. Upload Speed 115200 si se quiere una primera carga conservadora. Resto de ajustes predeterminados de ESP32 Dev Module.
6. Pulsar Verificar y luego Subir. Si queda en Connecting, mantener BOOT durante inicio de conexión y soltar al comenzar escritura. No cambiar cables con USB conectado. Si falla, guardar error exacto antes de otro intento.
7. Abrir Monitor Serie a 115200. Cada 3 s: `DEVKITC ACTIVO | HOLA recibidos: 0 | motores deshabilitados`. El cero es normal hasta conectar S3 programado. Esto confirma ejecución local, todavía no enlace entre placas.

## Después: S3 por separado

1. Desconectar USB DevKitC. Señales UART aún retiradas.
2. Conectar S3 por USB-UART; abrir `s3_emisor/s3_emisor.ino`.
3. Placa `ESP32S3 Dev Module`, puerto COM del S3. USB CDC On Boot: Disabled, para Monitor Serie por puente USB-UART. Estos sketches mínimos no necesitan activar cámara ni PSRAM.
4. Verificar/Subir. Monitor Serie a 115200 mostrará HOLA y SIN RESPUESTA: normal estando solo.
5. Desconectar USB S3.

## Unir y probar

Con todo apagado: S3 41 (TX) a DevKitC 16 (RX), DevKitC 17 (TX) a S3 42 (RX), GND común. No unir salidas 3V3. Mantener cables cortos. Alimentar ambas por USB del mismo computador, sin alimentación externa, y evitar dejar una sola encendida durante la transmisión. VM desconectado.

En Monitor del DevKitC: `S3 -> HOLA | respuesta -> RECIBIDO | total: ...` incrementando. En S3: `DevKitC -> RECIBIDO | ida y vuelta OK`. Para observar ambos a la vez, usar dos ventanas IDE, cada una con su sketch, placa y COM; también se pueden revisar por separado sin apagar placas.

La recepción es una línea ASCII terminada en salto de línea; 115200, 8N1. No enviar órdenes de motor: se descartan. Una respuesta no es un protocolo robusto de movimiento: esta prueba no mide errores sostenidos ni valida funcionamiento con ruido, cámara o corriente de motores.

GPIO41/42 del S3 siguen basados en placa de referencia: probar en hardware sin cámara activa no certifica compatibilidad simultánea con cámara. Si faltan respuestas, guardar salidas de ambos monitores, apagar ambas placas y revisar continuidad/etiquetas antes de modificar GPIO. No cambiar aleatoriamente pines.

Fuente API: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/serial.html
```

### `pruebas_uart/s3_emisor/s3_emisor.ino`

```cpp
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
```

### `pruebas_mpu/devkitc_mpu/devkitc_mpu.ino`

```cpp
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
```

### `pruebas_mpu/LEEME.md`

```markdown
# Prueba MPU6050 — DevKitC V4

Archivo: `devkitc_mpu/devkitc_mpu.ino`. No requiere librerias adicionales: Wire viene con Arduino ESP32. Estado: preparado y revisado, sin compilacion ni prueba local en la placa. Usar Verificar en el IDE.

## Cableado y carga

- MPU SDA → DevKitC 21; SCL → 22; GND → tierra comun.
- VCC segun modulo real verificado; SDA/SCL deben tener pull-ups a 3,3 V. No inferir compatibilidad a 5 V de la etiqueta MPU6050. AD0 determina 0x68/0x69; se prueban ambas direcciones. No dejar AD0 flotante si el modulo no fija su nivel. INT, XDA, XCL sin conectar.
- Bateria y fuentes externas apagadas, VM sin alimentar. Solo logica USB. Si S3 apagado, retirar las dos senales UART con todo apagado; si se mantiene enlace, alimentar ambas placas. No unir salidas 3V3.
- Abrir sketch separado. Placa ESP32 Dev Module, puerto DevKitC (antes COM5). No cargar al S3. Conservar ajustes de carga que funcionaron en prueba encoders.
- Verificar/Subir. Si hace falta modo descarga manual: mantener BOOT, pulsar/soltar EN; confirmar DOWNLOAD_BOOT, cerrar monitor antes de subir. No pulsar EN durante escritura. Al finalizar EN sin BOOT si queda esperando descarga.
- Monitor Serie 115200. Si se perdio el mensaje de deteccion inicial, EN una vez y leer desde inicio.

## Resultado esperado

`MPU6050 LISTO` seguido de aceleracion X/Y/Z en g y velocidades angulares en grados/s cada 500 ms. En reposo el modulo de aceleracion debe ser aproximadamente 1 g (gravedad). Si Z esta vertical, su lectura sera aproximadamente +1 o -1 g y X/Y proximos a cero; depende del montaje. El giroscopio quieto cerca de cero, con sesgo: no exigir cero exacto. La temperatura es interna del chip, no temperatura ambiente certificada.

Inclinar suavemente el chasis cambia la distribucion de gravedad entre ejes; al girarlo cambian las velocidades angulares durante el movimiento. Mover despacio y sujetar sin tensar cables. No desmontar la IMU para probarla ni forzar sus soportes.

Si WHO_AM_I no es 0x68 no configura el dispositivo; guardar salida y revisar identidad del sensor. Si no detecta o falla lectura, no interpretar ceros como datos validos. Revisar cables y niveles con alimentacion desconectada antes de reiniciar.

Esta prueba NO mide orientacion absoluta/yaw, no calibra sesgos ni fusiona sensores. Los sketches anteriores se conservan en disco; al subir este, se reemplaza el programa activo del DevKitC: encoders quedan temporalmente sin contar. Conserva respuesta RECIBIDO al HOLA del S3, pero no envia IMU por UART ni cambia sketch S3. Motores permanecen deshabilitados.

## Referencias

- https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html
- https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf
```

### `pruebas_encoders/devkitc_encoders/devkitc_encoders.ino`

```cpp
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
```

### `pruebas_encoders/LEEME.md`

```markdown
# Prueba manual de los cuatro encoders

Programa separado para DevKitC V4. No sobrescribe los sketches originales de UART. El usuario compiló, cargó y confirmó funcionamiento en hardware el 2026-10-29: captura de los cuatro contadores con FL=80, RL=55, FR=140, RR=60; al detenerse, NUEVOS=0. Es evidencia de conteo básico, no de calibración ni de prueba con motores energizados.

La carga inicial falló por modo de arranque; entrada manual con BOOT + EN produjo DOWNLOAD_BOOT y la carga posterior terminó con verificación de hash. No se confirmó la causa física exacta de los fallos previos.

## Antes de cargar

- Batería, reguladores externos y VM de ambos drivers sin alimentación. Solo lógica por USB.
- D0: FL→34, RL→35, FR→VP/36, RR→VN/39. A0 sin conectar; GND común.
- VCC del encoder debe corresponder a su módulo real; si usan el riel 3V3, comprobar que opera ahí y que D0 cambia entre bajo y un alto compatible con GPIO. Nunca aplicar D0 de 5 V al ESP32. No activar esta prueba sobre alimentación no verificada. No alimentar motores desde USB.
- Placas apagadas al cambiar cables. Si solo DevKitC queda alimentado, retirar las dos señales UART hacia el S3 apagado (GND puede permanecer). Para mantener UART, alimentar ambas placas sin fuentes externas y no cambiar el sketch del S3.

## Arduino IDE

1. Abrir `devkitc_encoders/devkitc_encoders.ino` en una ventana propia.
2. Elegir `ESP32 Dev Module` y COM del DevKitC (última sesión: COM5). No subir al S3/COM6.
3. Conservar los ajustes con los que ya arrancó el receptor. Verificar y Subir.
4. Monitor USB a 115200. Al iniciar contadores quedan en cero; también escribir `r` y Enter para reiniciarlos.

## Qué observar

Cada 500 ms imprime TOTAL, NUEVOS (pulsos desde el reporte anterior), niveles instantáneos D0 y número de HOLA recibidos.

1. Sin tocar ruedas: NUEVOS debería quedar en 0 tras estabilizar; TOTAL debe permanecer fijo.
2. Girar solo FL una vuelta lenta: aumenta FL; las otras columnas deben quedar fijas.
3. Repetir RL, FR y RR, una rueda a la vez. No forzar engranajes ni dejar que el disco toque el sensor.
4. Para calibración preliminar, poner cero y dar varias vueltas completas de la rueda elegida en el mismo sentido; anotar pulsos/vueltas. No asumir 20 pulsos por vuelta sin medir.
5. El conteo crece en ambos sentidos: son sensores de un canal. Esta prueba no calcula direccion, RPM ni distancia.
6. Si el S3 sigue con emisor, UART_HOLA puede aumentar y el emisor recibe RECIBIDO como antes. No cambia el protocolo ni envía pulsos por UART.

El programa cuenta flancos descendentes sin filtro de rebote. No confundir conteo bruto con lectura calibrada: bordes ruidosos pueden sumar de más. Si cuenta sin movimiento o los resultados varían, revisar estabilidad mecánica, GND, pull-up del módulo, ruido y alineación antes de añadir filtrado. GPIO34–39 no admiten pull-up interno. Se deja radio y ADC sin iniciar, especialmente por las restricciones de GPIO36/39.

Si un contador no cambia: revisar la pareja D0/GPIO con ambas placas apagadas; comprobar alineación disco/sensor y medir D0 al bloquear/liberar óptica. El nivel D0 impreso es una muestra lenta: no siempre cambia visiblemente aun con pulsos. Un LED del módulo no prueba por sí solo que el ESP32 reciba una señal correcta.

Siguiente etapa tras validar los cuatro: leer IMU, terminar medidas de alimentación y hacer prueba individual de motores, todavía sin navegación.

Fuente API GPIO: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/gpio.html
```

### `pruebas_distancia/devkitc_distancia/devkitc_distancia.ino`

```cpp
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
```

## D. Memoria histórica íntegra

### `MEMORIA_SUPREMA.md`

# Memoria suprema del robot 4WD

**Corte de información — 14 de noviembre de 2026, America/Bogota.**

Documento de continuidad para entregar a otra IA o integrante del equipo. Reúne la conversación disponible, los resultados aportados por el usuario y la revisión de los archivos de esta carpeta. Incluye decisiones vigentes, propuestas históricas, errores y asuntos abiertos. Las fechas de muchas pruebas individuales no quedaron registradas y por eso se conserva su orden relativo.

> **Actualización posterior del mismo 14 de noviembre:** se recuperó la conversación padre «Analiza video y material de clase (2)». Allí el D6 ya había funcionado con CH340 COM9/CSPC (~9,9 Hz), el usuario confirmó montaje en el carro y XL4016 ajustado a 5,0 V. Mapa adoptado: amarillo TXD D6 → S3 RX GPIO1; verde RXD D6 ← S3 TX GPIO2; rojo → 5 V regulados; negro → GND. El CH340 se retira al conectarlo al S3. Alimentación adoptada: un XL4016 de 5 V para ramas VM, entradas 5V de placas y D6; lógica TB6612 a 3V3 del DevKitC, retornos comunes separados de potencia. Estas evidencias actualizan las menciones históricas de «D6 pendiente» y dos reguladores del cuerpo del documento.
>
> Se creó `prueba_integral_50cm/LEEME.md` y dos sketches separados, compilados con Arduino-ESP32 3.3.12. Incluyen video MJPEG con alternativa DRAM sin PSRAM, parser COIN-D6, UART con CRC, MPU, cuatro encoders y avance estimado de 50 cm. Se verificó el parser con pruebas C++ de corrupción, cobertura y caducidad. **No se cargó ni validó físicamente este nuevo firmware integral.** La página habilita avance solo con todas las condiciones y cancela si el mínimo LiDAR de 360° es menor de 25 cm. Factores reconstruidos de registros de 30 cm: 13,2/27, 13,2/25, 13,2/26 y 13,2/24 cm/pulso. La distancia física de 50 cm y la integración simultánea siguen por comprobar. Leer el LEEME del paquete antes de cargarlo.
>
> El usuario considera completadas las pruebas anteriores de movimiento de 30 cm, comunicación entre placas y cámara con UART. No pedir repetirlas como requisito inicial sin un motivo nuevo y concreto. Las secciones siguientes conservan el historial y pueden describir etapas anteriores a esta actualización.

## Índice

1. Punto exacto de continuidad
2. Cómo interpretar la evidencia y resolver contradicciones
3. Objetivo académico y arquitectura
4. Inventario y evolución de placas
5. Mapa eléctrico del DevKitC y motores
6. Alimentación, distribución y GND
7. Encoders y montaje mecánico
8. MPU6050 y diagnóstico de alimentación
9. UART entre los dos ESP32
10. Movimiento y calibración de 30 cm
11. Arduino IDE, puertos y errores de carga
12. Cámara OV5640, montaje y pruebas
13. Video MJPEG y PSRAM
14. LiDAR D6 y adaptador CH340
15. Registro de decisiones y errores de análisis
16. Archivos disponibles y diferencias con lo cargado
17. Pendientes y ruta de integración
18. Documentación de clase y fuentes
19. Evidencias fotográficas
20. Contrato de relevo para otra IA
21. Anexos de código y documentos existentes

## 1. Punto exacto de continuidad

El equipo está construyendo físicamente un robot 4WD. Se montaron motores, ruedas, discos/lectores de encoder, dos drivers, un DevKitC V4 y un ESP32-S3. Se hicieron pruebas por etapas.

| Parte | Estado al corte | Evidencia y límite |
|---|---|---|
| Cuatro motores | Funcionales para avance | Usuario confirmó que todas las ruedas avanzan después de corregir sentidos |
| Cuatro encoders | Cuentan movimiento | Pruebas manuales y conteos con motores durante desplazamientos |
| Avance de 30 cm | Aceptado por el equipo | Tres últimas corridas físicas de aproximadamente 30 cm |
| MPU6050 | Lecturas obtenidas | Captura con aceleración, giro y temperatura después de cambiar de punto GND |
| UART entre placas | Completada según el usuario | Capturas previas de HOLA en DevKitC y reafirmaciones posteriores de comunicación funcionando |
| Cámara | Captura e imagen web comprobadas | Sensor PID 0x5640, JPEG 320×240, página con imagen visible |
| Video continuo | Última prueba declarada completada | Sketch MJPEG con alternativa DRAM tras fallo de detección PSRAM |
| PSRAM S3 | Problema abierto | `psramFound()` siguió dando falso tras seleccionar OPI según el usuario |
| Cámara + UART | Usuario afirma prueba ya realizada | No hay en la carpeta un sketch combinado inspeccionado que la reproduzca |
| Adaptador USB del LiDAR | Reconocido | Windows mostró `USB-SERIAL CH340 (COM9)` |
| LiDAR | Conexión de sus cuatro cables pendiente | No hay lectura, giro ni nube de puntos confirmados |
| Alimentación autónoma completa | No cerrada documentalmente | Se usó batería para motores y USB en pruebas de placas; faltan tensiones/capacidades registradas |
| ROS 2 del robot físico | Pendiente | Arquitectura prevista y material previo de clase, sin puente real validado |
| SLAM/Nav2/VLM sobre el robot | Pendiente | Objetivo final, no implementación demostrada |

La última solicitud de montaje fue indicar por nombre y color los cables del LiDAR. Antes de concluir esa investigación, el usuario solicitó esta memoria. **No se entregó una tabla definitiva de conexiones del LiDAR y no se debe dar por conectado.**

## 2. Cómo interpretar la evidencia y resolver contradicciones

### 2.1 Categorías

- **Observado** — se ve en una captura o en un archivo inspeccionado.
- **Reportado** — el usuario declara que hizo una prueba y funcionó.
- **Diseño vigente** — mapa adoptado para construir y programar, aunque no todos sus parámetros estén medidos.
- **Propuesto** — siguiente implementación o alternativa todavía sin evidencia de funcionamiento.
- **Pendiente** — dato que no se logró confirmar.

No convertir un LED encendido en prueba de funcionamiento completo, una propuesta de esquema en montaje verificado, o una descarga de documentación en instalación de software.

### 2.2 Precedencia práctica

Combinar resultados recientes del usuario, código realmente cargado, esquema vigente y archivos del repositorio. `AGENTS.md` y otros documentos contienen estados de octubre que quedaron atrás. Por ejemplo, dicen que el MPU no ha funcionado, pero después sí se recibieron datos. La sección conceptual que pone todos los motores en el S3 también quedó superada por el reparto entre dos placas.

El sketch abierto en Arduino IDE puede diferir del cargado en el microcontrolador. Usar banner de ejecución, carga verificada y copia real del sketch para identificar la versión.

### 2.3 Límites de esta memoria

Se conserva todo lo relevante recuperable de la conversación y de los archivos revisados. Algunas respuestas antiguas del asistente no están disponibles literalmente y no se ha leído el firmware desde las placas. Se recuperó el texto adjunto antiguo de distancia y el sketch actual guardado en la carpeta Arduino, que ya contiene la prueba de cámara. Se indican esas lagunas en vez de inventar constantes, mediciones o versiones.

## 3. Objetivo académico y arquitectura

Asignatura **Programación Distribuida y Paralela**, semestre VIII. Meta de robot móvil capaz de percepción, sensores y navegación, con una instrucción final como «Ve a la izquierda de las gafas».

Reparto vigente

```text
COMPUTADOR
  ROS 2, percepción/OpenCV/VLM, localización, navegación
  recibe imágenes y datos, produce objetivos y órdenes
       │ Wi-Fi para video y futura comunicación con PC
       ▼
ESP32-S3 N16R8 + cámara OV5640
  cámara y comunicación
       │ UART de aplicación 115200, 8N1
       ▼
ESP32 DevKitC V4 / ESP-32D
  motores, encoders, MPU6050, control local
       ├── TB6612 izquierdo → FL y RL
       └── TB6612 derecho   → FR y RR

LiDAR D6 → adaptador/interfaz por identificar → PC o controlador
           La integración definitiva no está elegida ni validada.
```

El VLM interpreta imágenes e instrucciones y debe pasar por una capa que valide objeto, relación, posición y objetivo navegable. La presencia de unas gafas en una imagen no determina por sí sola su distancia ni una posición libre a su izquierda.

Interfaces ROS orientativas `/cmd_vel`, `/odom`, `/imu/data_raw`, `/camera/image_raw`, `/scan`, `/tf` y `/tf_static`. No existen necesariamente en el robot actual. Separar percepción, decisión, localización y control sin crear nodos artificialmente por cada función.

## 4. Inventario y evolución de placas

### 4.1 Componentes físicos conocidos

| Componente | Cantidad / detalle |
|---|---|
| Chasis | Acrílico 4WD, cuatro motores DC amarillos, cuatro ruedas negras/amarillas |
| Drivers | 2 × TB6612FNG, dos canales cada uno |
| Encoders | 4 módulos con LM393 y discos ópticos en ejes |
| IMU | Módulo azul MPU-6050 tipo GY-521 |
| Control de movimiento | ESP32 DevKitC V4, módulo rotulado ESP-32D, código visible XX5R69 |
| Cámara/comunicación | ESP32-S3 con módulo N16R8, dos USB-C, conector FPC de cámara y microSD |
| Cámara | Inicialmente «OV56»; lectura posterior PID 0x5640 identifica OV5640 |
| Sensor de alcance | Llamado LiDAR D6 por usuario/profesor; identificación visual probable COIN-D6 |
| Adaptador del sensor | USB azul CH340C con UART TTL, RS-232 y RS-485 rotulados |
| Reguladores | LM2596 y XL4016 |
| Alimentación | Celdas 18650 con protección integrada reportada, portabaterías, fusible/portafusible, interruptor de balancín |
| Otros | Condensadores cerámicos y electrolíticos, cables y multiconector de distribución |

No se conocen capacidad/configuración exacta del paquete de batería, referencia/corriente de bloqueo de motores, calibre de cables, valor de fusible ni inventario preciso de condensadores.

### 4.2 Alternativas consideradas históricamente

Se evaluaron ESP32UE ECO V3 con antena, ESP32-C3 y ESP32-S3 DevKitC-1 N16R8 de 44 pines. Se compartieron enlaces de MercadoLibre y AliExpress. **La placa de motores efectivamente incorporada fue el DevKitC V4 / ESP-32D de las fotos.** No tratar las alternativas como compras realizadas.

Una ESP32-CAM se usó como fuente de 3V3 para comprobar si el MPU encendía. Eso no sustituye al S3 vigente ni demuestra que el proyecto ahora tenga tres controladores integrados. El usuario a veces llama «ESP32-CAM» al S3 con cámara.

### 4.3 Identificación física del S3

En las fotos recientes, una cara tiene blindaje metálico, antena PCB, conector blanco/negro largo para cinta y dos USB-C. La otra tiene microSD, CH343P y regulador marcado 1117C 33. Los USB se identifican por serigrafía **USB-UART** y **USB-OTG**. El chip USB del S3 no debe confundirse con el CH340C del adaptador del LiDAR.

## 5. Mapa eléctrico del DevKitC y motores

Los números siguientes son **GPIO**, no posiciones consecutivas del conector.

```text
             FRENTE DEL CARRO
       FL                       FR
       RL                       RR
```

FL = front left / delantera izquierda. RL = rear left / trasera izquierda. FR = front right / delantera derecha. RR = rear right / trasera derecha.

### 5.1 Doce señales independientes

| Driver | Canal / rueda | Pin driver | GPIO DevKitC | Función |
|---|---|---|---|---|
| Izquierdo | A / FL | AIN1 | 13 | Dirección entrada 1 |
| Izquierdo | A / FL | AIN2 | 14 | Dirección entrada 2 |
| Izquierdo | A / FL | PWMA | 25 | PWM FL |
| Izquierdo | B / RL | BIN1 | 26 | Dirección entrada 1 |
| Izquierdo | B / RL | BIN2 | 27 | Dirección entrada 2 |
| Izquierdo | B / RL | PWMB | 32 | PWM RL |
| Derecho | A / FR | AIN1 | 18 | Dirección entrada 1 |
| Derecho | A / FR | AIN2 | 19 | Dirección entrada 2 |
| Derecho | A / FR | PWMA | 23 | PWM FR |
| Derecho | B / RR | BIN1 | 4 | Dirección entrada 1 |
| Derecho | B / RR | BIN2 | 15 | Dirección entrada 2 |
| Derecho | B / RR | PWMB | 33 | PWM RR |

| Señal compartida | Conexión |
|---|---|
| STBY izquierdo y derecho | GPIO2 mediante distribución en Y |
| VCC izquierdo y derecho | Distribución 3V3 del DevKitC |
| GND izquierdo y derecho | GND común, con retorno de potencia apropiado |
| VM izquierdo y derecho | OUT+ del XL4016 / rama de motores verificada |

Se descartó el primer diseño que compartía señales por lado. El objetivo adoptado fue controlar cada motor por separado. Compartir dirección limita sentidos independientes, pero **la independencia de velocidad depende de PWM separados**. No confundir AIN1/BIN1 con entradas PWM. STBY común sí permite deshabilitar todos sin agrupar velocidades.

### 5.2 Salidas hacia motores y colores reportados

| Driver | Par de salida | Motor | Cable reportado |
|---|---|---|---|
| Izquierdo | AO1 / AO2 | FL | Rojo a AO1 y negro a AO2 |
| Izquierdo | BO1 / BO2 | RL | Rojo a BO1 y negro a BO2 |
| Derecho | AO1 / AO2 | FR | Rojo a AO1 y negro a AO2 |
| Derecho | BO1 / BO2 | RR | Rojo a BO1 y negro a BO2 |

El usuario escribe a veces A01/A02/B01/B02. Se refiere a AO1/AO2/BO1/BO2. Estos pares entregan potencia reversible al motor. La corriente procede de VM, conmutada por el puente H. Los GPIO solo mandan señales.

Cada motor lleva dos cables al driver. Las ruedas se acoplan mecánicamente al motor. Los encoders leen el movimiento por separado.

### 5.3 Sentidos corregidos en software

| Rueda | `invertir` adoptado en el archivo local | Avance |
|---|---|---|
| FL | true | IN1 LOW, IN2 HIGH |
| RL | false | IN1 HIGH, IN2 LOW |
| FR | true | IN1 LOW, IN2 HIGH |
| RR | false | IN1 HIGH, IN2 LOW |

El usuario aclaró «FR hacia atrás y RR hacia adelante» antes de corregir. Posteriormente confirmó que todas avanzaban. Conservar esa referencia al recuperar el firmware, sin intercambiar cables por iniciativa propia.

### 5.4 GPIO con restricciones

- GPIO2, conectado a STBY, interviene en entrada al cargador. No forzarlo alto durante programación. Revisar puentes/pull-ups del breakout.
- GPIO15 también es de arranque y un nivel bajo puede silenciar mensajes ROM. No atribuir todos los fallos de carga a ese pin sin comprobar.
- VP es GPIO36 y VN es GPIO39. El usuario creyó que no existían porque la placa usa esas etiquetas.
- GPIO34–39 son entradas sin pull-up/pull-down internos.
- GPIO1/3 TX/RX del DevKitC se reservan para USB/programación en este diseño.
- GPIO6–11 / D0/D1/D2/D3/CMD/CLK de memoria no se usan para el robot.
- GPIO16/17 están disponibles en la referencia WROOM/ESP-32D usada aquí. No extrapolar a placas con otra memoria/módulo.

Los colores de las doce señales, UART y cables del MPU no quedaron asignados ni fotografiados extremo a extremo. No inventarlos. Las etiquetas I1–I6, D1–D6, L1/L2, etc., son nombres de cables del mapa, no necesariamente nombres físicos de pines.

## 6. Alimentación, distribución y GND

### 6.1 Topología adoptada

```text
BATERÍA (+)
  └── FUSIBLE + INTERRUPTOR
        ├── XL4016 IN+
        │     OUT+ ─── VM driver izquierdo
        │          └── VM driver derecho
        │     OUT− ─── distribución GND de potencia
        │
        └── LM2596 IN+
              OUT+ ─── rama lógica prevista de 5 V verificados
                        ├── entrada 5V adecuada del DevKitC
                        └── entrada 5V adecuada del S3
              OUT− ─── GND común

BATERÍA (−) ─── retornos IN− según convertidores reales
GND común incluye placas, drivers y sensores.
Cada convertidor debe verificarse en su placa real.
```

Esta es la topología de proyecto. No consta una cifra medida para VM ni una puesta en servicio completa del LM2596 alimentando ambas placas sin USB. El usuario dijo que no tenían multímetro en una de las etapas. No presentar el voltaje como medido ni el dimensionamiento como cerrado.

### 6.2 VCC y VM del TB6612

- **VCC** alimenta lógica. La conexión vigente es 3V3 del DevKitC a VCC de ambos módulos.
- **VM** alimenta motores. Recibe la rama XL4016 ajustada a motores y driver.
- El puente H conmuta potencia, no regula la tensión de VM ni proporciona aislamiento galvánico.
- AO1/AO2/BO1/BO2 son las salidas de esa potencia.
- GND común proporciona referencia a señales. La corriente de motor debe tener su propio retorno hacia distribución.

Se produjo mucha confusión porque el usuario interpretaba que alimentar VCC desde el ESP significaba alimentar el motor desde el ESP. Se aclaró la separación. **No corregir VCC llevándolo al XL4016 de motores.** Tampoco cambiarlo automáticamente a 5 V. La ficha del TB6612 exige un alto de entrada proporcional a VCC, por lo que 3,3 V lógico mantiene coherencia con GPIO de 3,3 V.

### 6.3 Multiconector 3V3

El usuario tenía un multiconector desde 3V3 del DevKitC y una unión de cables para driver/MPU. Repartir una salida en paralelo puede ser válido si hay continuidad, aislamiento y capacidad de corriente. El hecho de compartir no prueba sobrecarga.

L1 y L2 del esquema eran los ramales a VCC de cada driver. G1/G2 eran los ramales GND. No meter dos terminales flojos en un solo pin. Usar distribución firme y aislada. La protección/impedancia exacta del multiconector no se comprobó.

### 6.4 GND y diagnóstico real

El usuario propuso usar el GND del MPU como paso hacia el otro ESP por falta de pines libres. El concepto necesario es una red común. La corriente de motores no debe pasar por el cable del MPU.

Más adelante el MPU no encendía con cierto GND del DevKitC. Cambiar a otro punto GND hizo que encendiera y luego entregara datos. Esto indica un problema en ese trayecto/contacto/pin. No se verificó si era header, soldadura, cable, distribución o daño interno. No declarar una placa entera defectuosa ni reparada por esa observación.

### 6.5 USB y alimentación final

- En banco, USB alimenta cada placa. La batería puede alimentar únicamente VM en pruebas controladas, con GND común y rama positiva lógica externa aislada.
- Si las placas se alimentan por entradas externas, retirar USB previamente salvo que se haya verificado la protección frente a retorno de corriente de las placas reales.
- La pregunta «retirar ambas USB» se refería a sacar físicamente los cables USB de las dos placas, al cambiar de modo de alimentación. No es una instrucción de desconectar todos los cables de sensores.
- No unir las salidas 3V3 del S3 y DevKitC.
- No alimentar una placa apagada indirectamente por UART/I2C desde otra encendida. Al aislar una placa o módulo, aislar señales según el caso.
- La tensión exacta del XL4016 depende de los motores. El máximo del TB6612 no es una consigna para los motores.
- Fusible, corriente de bloqueo, temperatura, batería y condensadores requieren medición/fichas reales.

El mapa menciona desacoplo de referencia por driver de 10 µF y 100 nF en VM/GND y VCC/GND, sujeto a comprobar lo ya incorporado y los valores disponibles. No consta instalación física de todos ellos.

## 7. Encoders y montaje mecánico

### 7.1 Montaje

Cuatro motores y ruedas ya estaban en el chasis acrílico cuando se enviaron las primeras fotos. Se observaban discos negros en ejes internos. El módulo óptico debe quedar fijo y el disco alineado en su ranura sin rozar. No hay conexión eléctrica del disco con la llanta.

El usuario pidió aclarar si los motores iban a LM393 o TB6612. Se estableció motor → salidas del TB6612, y encoder → entrada del ESP32.

### 7.2 Cableado

| Módulo | Salida usada | Destino |
|---|---|---|
| Encoder FL | DO / D0 | GPIO34 |
| Encoder RL | DO / D0 | GPIO35 |
| Encoder FR | DO / D0 | VP / GPIO36 |
| Encoder RR | DO / D0 | VN / GPIO39 |
| Cada encoder | GND | Distribución GND de lógica |
| Cada encoder | VCC | Alimentación compatible del módulo, referencia de montaje 3V3 |
| Cada encoder | AO / A0 | Sin uso en estas pruebas |

Los módulos reales tienen VCC, GND, D0 y A0. «OUT» fue un nombre genérico usado en esquemas y equivale aquí a la salida digital DO. No existe necesariamente un pin rotulado OUT.

El usuario reportó que los LED de encoders encendían al alimentar ESP32 durante UART. Eso es compatible con recibir alimentación lógica. No prueba independencia de fuentes, niveles ni conteo correcto.

La alimentación de módulos LM393 no se define por el nombre del comparador. Verificar que DO no exceda 3,3 V y tenga pull-up compatible. No se tienen mediciones de nivel alto archivadas. No usar INPUT_PULLUP como solución en GPIO34–39.

### 7.3 Conteo y límites

Se contaron flancos FALLING. En una versión local se usan `volatile` y sección crítica; en otras versiones de conversación `std::atomic<uint32_t>` y `fetch_add` relajado. No son dos pruebas de hardware distintas.

Un canal óptico con disco regular cuenta pulsos y no determina el sentido real por sí mismo. La odometría futura deberá inferir signo de la orden y considerar deslizamiento/movimiento externo. No está validada dirección por encoder ni control PID.

Conteos manuales observados en una captura temprana `TOTAL FL=80 RL=55 FR=140 RR=60`, con NUEVOS=0 al quedar quieto.

### 7.4 Holgura de ruedas

El equipo observó ruedas que se doblaban o se movían lateralmente, consideró cinta u otra sujeción y luego reportó haberlo arreglado. Más tarde volvió a indicar pequeñas cosas por ajustar en los lados, pero aceptó el resultado de distancia. No se documentó la reparación definitiva ni el material usado.

No diagnosticar «desfase de encoder» comparando conteos obtenidos al dar giros manuales distintos. El usuario aclaró que 1 y 7 pulsos correspondían a cuánto había girado la rueda. Una sola rueda accionada brevemente tampoco tiene que desplazar todo el vehículo.

## 8. MPU6050 y diagnóstico de alimentación

| MPU | DevKitC |
|---|---|
| VCC | 3V3 usado en prueba exitosa |
| GND | Punto GND comprobado / distribución común |
| SDA | GPIO21 |
| SCL | GPIO22 |
| AD0 | Define 0x68/0x69, conexión física final sin captura concluyente |
| INT, XDA, XCL | Sin uso en prueba básica |

Módulo azul rotulado MPU-6050 con regulador visible. No extrapolar que cualquier GY-521 permite conectar a 5 V sin revisar los pull-ups I2C.

Secuencia de incidencias

1. Sketch inicial copiado quedó cortado dentro de `leerUART()` en la línea 96, con errores `expected '}' at end of input` que referían llaves abiertas en líneas 84, 85 y 88.
2. Se entregó programa completo. Luego aparecía `MPU NO DETECTADO/VALIDADO` y `MPU no listo`.
3. El usuario preguntó si VCC podía ir a GND o al «cable rojo». Se aclaró que VCC es positivo de la tensión requerida y GND es retorno. Un color no determina voltaje.
4. En un punto el MPU estaba completamente desconectado. El error de detección era esperable en ese estado.
5. Conectar solo VCC/GND coincidió con `flash read err, 988` y reinicios. No se demostró causa única.
6. Alimentarlo desde otra ESP32-CAM encendió su LED. No bastaba para descartar fallo funcional del MPU.
7. Conexión directa al DevKitC, sin multiconector, tampoco encendía usando un GND determinado.
8. Cambiar a otra entrada GND del DevKitC hizo que encendiera. Después se obtuvieron datos por I2C.

Ejemplos de salida observada

```text
A[g] X=-0.026 Y=-0.999 Z=0.048 | modulo=1.000 |
GIRO[grados/s] X=-10.63 Y=-0.47 Z=-1.34 | Tchip=21.7 C | UART_HOLA=0
```

También se vieron temperaturas cercanas a 29,7–29,9 °C y vectores con módulo cercano a 1 g en otra orientación. Estos datos prueban lectura, no calibración de IMU. El sesgo de giro no se cerró. `UART_HOLA=0` en una prueba aislada no implica fallo del MPU.

El archivo local busca 0x68 y 0x69, comprueba WHO_AM_I 0x68, despierta el sensor, configura ±2 g y ±250 grados/s, filtro y muestreo interno. No hay fusión de orientación validada.

## 9. UART entre los dos ESP32

| Origen | Destino | Propósito |
|---|---|---|
| S3 GPIO41 TX | DevKitC GPIO16 RX | Órdenes / HOLA |
| DevKitC GPIO17 TX | S3 GPIO42 RX | Respuestas / RECIBIDO |
| GND de ambas | Red GND común | Referencia |

Configuración 115200 baudios, SERIAL_8N1. En `begin(115200, SERIAL_8N1, RX, TX)` el orden de argumentos es RX antes de TX.

- DevKitC usa `HardwareSerial enlace(2)`.
- S3 de la prueba UART usa `HardwareSerial enlace(1)`.
- USB/Serial de diagnóstico es independiente de esta UART.
- No se unieron salidas 3V3 para comunicar.
- Evitar cables largos o cercanos a potencia sin pruebas de ruido.

### 9.1 Flujo básico de prueba

El S3 enviaba HOLA cada 2 s. DevKitC respondía RECIBIDO e incrementaba contador. El S3 esperaba respuesta y mostraba SIN RESPUESTA tras 1 s. Los banners son específicos de cada sketch.

```text
DEVKITC LISTO | RX16 TX17 | MOTORES DESHABILITADOS
DEVKITC ACTIVO | HOLA recibidos: 0 | motores deshabilitados
S3 -> HOLA | respuesta -> RECIBIDO | total: 12
```

Se vieron primero ceros y mensajes SIN RESPUESTA. Después capturas con contador creciendo 1 y posteriormente 7–12. El DevKitC documentó recepción y envío de respuesta. El usuario más adelante confirmó comunicación realizada, y después confirmó que cámara + UART ya se habían probado. **Aceptar ese estado reportado.** La ausencia de una captura específica del S3 no justifica ignorar la confirmación posterior.

### 9.2 Alcance de lo que no queda archivado

No se dispone de un sketch combinado inspeccionado que contenga cámara + UART + control completo. El sketch actual encontrado en Documents/Arduino/sketch_oct22a es MJPEG de cámara y no inicializa HardwareSerial de aplicación. Los sketches de cámara entregados en esta conversación no inicializan UART de aplicación. Si se carga uno de ellos, no puede inferirse que ese firmware concreto siga reenviando HOLA u órdenes. Mantener separada la afirmación de prueba realizada de la funcionalidad exacta del último binario.

No hay evidencia archivada de comandos remotos de velocidad, ACK por identificador, watchdog de enlace ni STOP remoto simultáneo a video en piso. Son funciones posteriores.

## 10. Movimiento y calibración de 30 cm

### 10.1 Pruebas de pulso

PWM de prueba 80, pulso 350 ms. Comandos individuales FL, RL, FR, RR y STOP en los sketches de etapa. Después se añadió AVANZA para accionar los cuatro.

Ejemplos

```text
LISTO | motores deshabilitados | FL RL FR RR STOP
PULSO FL | PWM 80 | 350 ms
FIN | motores deshabilitados
```

Usuario ejecutó FL sin batería y obtuvo cero pulsos. Era coherente con VM sin alimentar. Con batería, se obtuvieron conteos, por ejemplo RR=9, RL=7 y luego FL 0, 1 y 7 dependiendo de giro/prueba.

Prueba inicial conjunta

```text
PULSO AVANZA | PWM 80 | 350 ms
FIN | pulsos FL=6 RL=6 FR=7 RR=8
```

Otras corridas del pulso dieron 16/1/2/1, 3/4/4/3, 4/4/5/5 y 1/0/3/0. Coincidieron con revisión de holgura y condiciones mecánicas. No se documentó que todos fueran el mismo recorrido físico ni una comparación formal de velocidades.

### 10.2 Confusión entre sketches

Se indicó enviar AVANZA 30 mientras el usuario tenía cargado un programa de conteo con motores permanentemente deshabilitados. Ese código tenía STBY LOW, cuatro interrupciones, reporte TOTAL cada 500 ms y aceptaba únicamente Z para reiniciar conteos. Se corrigió la interpretación. No volver a pedir movimiento a ese sketch.

El archivo local `pruebas_encoders` usa `r + Enter` para cero. La versión pegada de conversación usa Z. El comando depende de la versión.

### 10.3 Datos de calibración antiguos que no deben reutilizarse

Se aportaron acumulados FL=148, RL=36, FR=130, RR creciendo 79, 80, 81, 132, 143, 152, 172, 188, 201, 202 y un último texto cortado `RR=20`. Esos acumulados no demostraban cinco vueltas uniformes por rueda.

Pese a ello, el archivo local quedó con diámetro supuesto 6,5 cm y `PULSOS_CINCO_VUELTAS = {148,36,130,202}`. Es provisional y difiere del resultado final. Su fórmula es circunferencia × 5 / conteo por rueda. **No cargar ese archivo como firmware de calibración aceptada.**

### 10.4 Ajustes y mediciones reales

| Etapa | Estimación reportada | Medición con regla | Interpretación |
|---|---|---|---|
| Primera | 30,1 cm, izq 44,1 / der 16,1 | 13–15 cm | Escala y referencias por rueda incorrectas |
| Diagnóstico | 30,5 cm, izq 44,8 / der 16,3 | 13–15 cm | FL27=18,6 cm, RL25=70,9, FR26=20,4, RR24=12,1 |
| Tras corrección | 30,1 cm, izq29,6 / der30,6 | ~29 cm | FL56=29,0, RL54=30,2, FR54=29,1, RR55=32,1 |
| Repeticiones | ~30,1 cm | ~28 cm en dos ensayos | Se siguió ajustando escala |
| Posterior | 30,1 cm, izq29,9 / der30,3 | ~30 cm | Usuario aceptó mejora |
| Última serie | 30,0–30,1 cm | ~30 cm en las tres | Baseline aceptado |

El usuario dijo que el carro fue recto cuando la estimación por lados era muy dispar. Por eso no era válido concluir deriva física solo desde distancias calculadas con constantes erróneas. Midieron desde la misma punta del carro antes y después.

Últimas tres corridas, conservar literalmente

```text
INICIO | objetivo 30 cm | PWM 80
FIN POR DISTANCIA | estimado 30.1 cm | izq 29.5 cm | der 30.7 cm
DIAGNOSTICO | FL: 59 pulsos, 28.8 cm | RL: 57 pulsos, 30.1 cm | FR: 59 pulsos, 30.0 cm | RR: 57 pulsos, 31.3 cm

INICIO | objetivo 30 cm | PWM 80
FIN POR DISTANCIA | estimado 30.1 cm | izq 29.5 cm | der 30.7 cm
DIAGNOSTICO | FL: 60 pulsos, 29.3 cm | RL: 56 pulsos, 29.6 cm | FR: 59 pulsos, 30.0 cm | RR: 57 pulsos, 31.3 cm

INICIO | objetivo 30 cm | PWM 80
FIN POR DISTANCIA | estimado 30.0 cm | izq 29.5 cm | der 30.6 cm
DIAGNOSTICO | FL: 60 pulsos, 29.3 cm | RL: 56 pulsos, 29.6 cm | FR: 60 pulsos, 30.5 cm | RR: 56 pulsos, 30.8 cm
```

El equipo quedó satisfecho y pidió avanzar. No imponer recalibración de 30 cm como siguiente tarea.

### 10.5 Constantes recuperables y laguna de versión

La reconstrucción de conversación registra referencia de **13,2 cm para 27/25/26/24 pulsos FL/RL/FR/RR**. Equivale a factores aproximadamente 0,488889 / 0,528 / 0,507692 / 0,55 cm/pulso y es consistente con los últimos diagnósticos redondeados. **No se inspeccionó el sketch final de Arduino que los produjo.** Recuperarlo del usuario antes de reemplazar su control de movimiento.

La estimación de avance promedia FL/RL para izquierda, FR/RR para derecha y ambos lados para distancia total. El límite de seguridad temporal de la referencia era 3000 ms. No confundir promedio de recorridos con control cerrado que iguala velocidades. Los conteos no demuestran PID ni exactitud de giros.

## 11. Arduino IDE, puertos y errores de carga

Entorno mostrado Arduino IDE 2.3.10 en Windows. Se vieron Arduino-ESP32 3.3.12 y esptool 5.3.1. El archivo abierto se llamó muchas veces `sketch_oct22a.ino`, aunque se reemplazó su contenido para pruebas distintas.

Ruta de trabajo del IDE vista `C:\Users\AndresPC\Documents\Arduino\sketch_oct22a\sketch_oct22a.ino`. No contiene necesariamente el último código esperado.

| Placa | Selección en IDE | Puertos observados |
|---|---|---|
| DevKitC V4 / ESP-32D | ESP32 Dev Module | COM5 |
| ESP32-S3 | ESP32S3 Dev Module | COM6 y después COM8 |
| Adaptador CH340 del LiDAR | Se usa como puerto de comunicación, no placa programable | COM9 |

Los COM pueden cambiar. No inventar un puerto fijo si el dispositivo reaparece con otro número.

### 11.1 Incidencias y solución que funcionó

| Mensaje | Qué significó / qué se hizo |
|---|---|
| `#error Selecciona ESP32S3 Dev Module` | Se compiló para otro target. Seleccionar S3 en el selector de placa y compilar/subir de nuevo |
| Caracteres ilegibles en monitor | En una captura el monitor estaba a 9600. Se pasó a 115200 |
| `Wrong boot mode detected (0x13)` | No había entrado al cargador. BOOT sostenido y pulso EN permitieron DOWNLOAD_BOOT |
| `waiting for download` | ROM esperando carga, no sketch corriendo |
| `Serial data stream stopped` | Fallo de transporte/carga, no prueba automática de error de código; revisar USB, velocidad y alimentación |
| `Could not open port COM5` y FileNotFoundError | El puerto no existía en ese momento. Revisar detección Windows y cable |
| `Hash of data verified` y `Hard resetting via RTS pin` | Escritura verificada y reinicio posterior |
| `No changed sectors found, verifying` | Sectores ya coinciden; carga/verificación válida si concluye correctamente |
| `flash read err, 988`, reinicios RTCWDT | Hubo fallos reales de arranque, causa sin demostrar. Coincidieron con cambios de alimentación/MPU |
| `qio_mode: Failed to set Q` | Se mostró en una etapa; no hay diagnóstico causal definitivo archivado |

El usuario rechazó desarmar todo para probar una placa sin cables. Preferir aislamiento local del ramal implicado con todo apagado, sin insistir en retirar el montaje completo como primera respuesta.

### 11.2 Monitor vacío del S3

Se mostró COM8 y 115200 pero sin texto. Se indicó USB-UART y USB CDC On Boot Disabled, volver a subir tras cambiar ajuste y pulsar EN. Después aparecieron capturas de cámara. Esto confirma la combinación usada, no USB-OTG como puerto probado.

Configuración recomendada usada en conversación para el S3 N16R8

- ESP32S3 Dev Module.
- Flash Size 16MB (128Mb).
- PSRAM OPI PSRAM, aunque su detección siguió fallando.
- USB CDC On Boot Disabled usando USB-UART.
- Upload Speed 115200 como referencia conservadora.
- Monitor 115200.

Para cargar, Verificar compila y Subir graba. EN reinicia lo que ya está grabado. Las pruebas de cámara arrancan solas y no necesitan comando de monitor.

## 12. Cámara OV5640, montaje y pruebas

### 12.1 Montaje físico

El usuario envió fotos del S3 y cámara separada, con lente y pestaña roja sobre protector. El conector FPC es blanco con pieza negra junto al blindaje y USB. Se pidió verificar seguro y cara de contactos porque el dedo ocultaba el extremo de la cinta. No llegó una confirmación fotográfica inequívoca de esa orientación. Luego el usuario informó que ya la conectó y las pruebas capturaron correctamente.

No desarmar una cinta que ya funciona para volver a probar orientación. No conectar alimentación aparte a la cámara montada por FPC. La carcasa/marca del S3 no se confirmó definitivamente como Freenove, pero el mapa siguiente quedó funcional con esa unidad.

### 12.2 Mapa de cámara comprobado por ejecución

| Señal | GPIO S3 |
|---|---|
| D0 | 11 |
| D1 | 9 |
| D2 | 8 |
| D3 | 10 |
| D4 | 12 |
| D5 | 18 |
| D6 de datos cámara | 17 |
| D7 | 16 |
| XCLK | 15 |
| PCLK | 13 |
| VSYNC | 6 |
| HREF | 7 |
| SCCB SDA | 4 |
| SCCB SCL | 5 |
| PWDN | -1 / sin GPIO dedicado |
| RESET de cámara | -1 / sin GPIO dedicado |

Este mapa de referencia Freenove/S3-EYE funcionó. No confundir D6 de datos de cámara con el nombre del sensor LiDAR D6. Tampoco confundir GPIO17 del S3 de cámara con GPIO17 de la UART en el DevKitC, que es otra placa.

GPIO41/42 del S3 quedan fuera de este mapa de cámara. La microSD no se utilizó en las pruebas. No habilitarla suponiendo pines libres sin revisar su circuito.

### 12.3 Primera prueba de capturas por USB

`esp_camera_init`, JPEG, QVGA 320×240, XCLK 20 MHz, calidad 12, un búfer. Si no había PSRAM, usaba DRAM. Capturaba cada 2 s y devolvía el frame buffer después de reportar tamaño.

Salida observada

```text
Identificador del sensor: 0x5640
CAMARA INICIADA | comenzando capturas
CAPTURA 1 | 320 x 240 | 5072 bytes | OK
CAPTURA 2 | 320 x 240 | 3544 bytes | OK
CAPTURA 3 | 320 x 240 | 4910 bytes | OK
CAPTURA 4 | 320 x 240 | 6442 bytes | OK
CAPTURA 5 | 320 x 240 | 6442 bytes | OK
CAPTURA 6 | 320 x 240 | 3683 bytes | OK
CAPTURA 7 | 320 x 240 | 6960 bytes | OK
```

Esto identifica OV5640 y confirma datos JPEG. No demuestra PSRAM, autofocus configurado ni FPS máximos.

### 12.4 Primera página web de fotos

- Librerías Arduino.h, WiFi.h, WebServer.h y esp_camera.h.
- Modo AP, red **RobotCam**, clave de laboratorio **CAMBIA_CLAVE_ROBOTCAM**.
- Página habitual **http://192.168.4.1**; comprobar la IP anunciada por `WiFi.softAPIP()`.
- `/` entrega HTML y `/captura` entrega JPEG.
- Página solicita foto y al recibirla espera `setTimeout(actualizar, 1000)` antes de otra.
- Un búfer y `CAMERA_GRAB_WHEN_EMPTY`.
- Cada foto termina conexión HTTP. Monitor imprime tamaño transferido.

El usuario recibió ERR_CONNECTION_TIMED_OUT al abrir la IP. Se indicó conectar el computador al Wi-Fi RobotCam, conservar la red aunque diga sin Internet y usar HTTP. Luego envió imagen visible. No se documentó exactamente qué cambio resolvió el timeout.

El USB alimenta y programa. No crea por sí solo acceso a la página del AP. Un PC conectado a RobotCam puede perder acceso a su Wi-Fi habitual. La prueba no necesita Internet.

El usuario señaló FPS bajos. La primera explicación correcta fue que había espera intencional de 1000 ms por foto más captura/transporte. Se sugirió 50 ms en JavaScript, pero el código que el usuario pegó seguía teniendo 1000 ms y había cambiado `cliente.setTimeout(50)`. **Ese timeout de socket no es el intervalo JavaScript y no elimina el límite de 1 foto/s.**

## 13. Video MJPEG y PSRAM

### 13.1 Cambio solicitado

El usuario pidió cámara rápida. Se entregó streaming MJPEG con `esp_http_server.h`, servidor web puerto 80 y video puerto 81, petición persistente `/stream`. Se eliminó el polling de fotos del navegador y se preservó `/captura` para guardar una imagen.

Configuración de video propuesta y conservada

- QVGA 320×240.
- JPEG calidad numérica 16, con mayor compresión que 12.
- XCLK 20 MHz.
- AP RobotCam / CAMBIA_CLAVE_ROBOTCAM, canal 1, máximo un cliente Wi-Fi.
- Cada tres segundos de envío imprime FPS enviados.
- Ceder CPU con delay(1) por frame y delay(100) en loop.
- Servidores HTTP en tareas independientes, stack de 8192, timeout de envío 3 s.
- Servidor 81 usa puerto de control distinto al de 80.

El contador mide frames enviados por el S3, no FPS de decodificación/presentación ni latencia extremo a extremo. No se registró un valor numérico de FPS final.

### 13.2 Fallo real

La primera versión MJPEG requería `psramFound()` y salía de setup si no detectaba PSRAM. Dos capturas mostraron

```text
ROBOTCAM | VIDEO CONTINUO MJPEG
ERROR | PSRAM no detectada.
Selecciona PSRAM: OPI PSRAM y vuelve a subir.
```

El usuario afirmó haber seleccionado OPI y seguir con el mismo resultado. No se demostró si el ajuste llegó a la compilación/carga correcta ni una avería de memoria. No basta asumir memoria dañada porque falla `psramFound()`. La primera prueba de cámara funcionaba con alternativa DRAM.

### 13.3 Corrección final entregada

Se eliminó el retorno obligatorio y se añadió diagnóstico

```cpp
const bool tienePsram = psramFound();
Serial.printf("PSRAM detectada: %s | total: %lu bytes\n",
              tienePsram ? "SI" : "NO",
              static_cast<unsigned long>(ESP.getPsramSize()));
#ifdef BOARD_HAS_PSRAM
  Serial.println("Compilacion: soporte PSRAM habilitado");
#else
  Serial.println("Compilacion: soporte PSRAM deshabilitado");
#endif

if (tienePsram) {
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
} else {
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
}
```

Se entregó el sketch completo con esa alternativa. El usuario respondió **«listo prueba completada»**. Conservarlo como prueba exitosa de cámara/video reportada. No afirmar que OPI quedó resuelto ni que estaba usando dos búferes. La salida que indicaría el camino final de memoria no se compartió.

### 13.4 Consideraciones para futura integración

El streaming bloquea su handler de servidor 81 mientras está conectado. El servidor 80 sigue en otra tarea, pero eso no sustituye una arquitectura de órdenes y STOP ya diseñada. Con un solo búfer, `/captura` y streaming comparten recursos y pueden competir. No hay validación de múltiples clientes ni garantía de FPS.

El último sketch de cámara no controla motores, no procesa UART de aplicación y no contiene ROS 2. El usuario sí reportó cámara + UART probadas por otra combinación. Recuperar su versión combinada antes de reemplazarla.

## 14. LiDAR D6 y adaptador CH340

### 14.1 Estado físico y fotos

El usuario tiene un sensor con base cuadrada, orejas de montaje, cubierta cilíndrica negra brillante y conector blanco de cuatro posiciones. Una etiqueta verde muestra un código de producción/serie, leído aproximadamente `A32608140166/11010V11`, no una ficha de alimentación.

En la foto del conector, con la orientación de esa foto, el orden visible de izquierda a derecha es **amarillo, verde, negro, rojo**. Esta es una observación geométrica, **no la asignación eléctrica**. Girar el sensor cambia izquierda/derecha, por lo que no debe usarse sin una vista de referencia.

Los terminales libres son Dupont hembra. No hay evidencia de conexión de esos cuatro cables a adaptador o fuente ni de funcionamiento del LiDAR.

### 14.2 Adaptador USB

Fotos de dos caras de una placa azul con CH340C y circuitos adicionales. Etiquetas visibles

| Cara / grupo | Etiquetas |
|---|---|
| UART TTL / alimentación | DTR, +5V, 3V3, TXD, RXD, GND |
| Otras interfaces | GND, 232R, 232T, B, A, RTS |

Usar la serigrafía real de la cara correspondiente. No leer el orden de una cara como si fuese el mismo al mirar la otra. A/B son RS-485; 232R/232T son RS-232. No unirlos directamente a GPIO de 3,3 V del ESP32.

Windows confirmó **USB-SERIAL CH340 (COM9)**. Esto solo prueba enumeración del adaptador. No es prueba de alimentación o datos del LiDAR. No hace falta escoger una placa Arduino para abrir COM9 como serial.

No se midió el nivel del TXD del adaptador, no se comprobó un jumper selector de niveles y no se verificó la capacidad de su +5V bajo arranque del sensor. No alimentar un LiDAR con 3V3 por comodidad ni confundir tensión de alimentación del sensor con nivel lógico UART.

### 14.3 Investigación documental al momento del relevo

La búsqueda inicial por «D6» dio **otros equipos** con diez cables y pinout distinto (FASELASE D6) y LD06 con PWM. No usar esos resultados para este sensor solo porque comparten letras y colores.

La carcasa y el conector coinciden visualmente con **COIN-D6 de Guoke/CSPC, distribuido por WitMotion**. Esa identificación sigue siendo una inferencia visual, no lectura inequívoca de referencia en la etiqueta del usuario.

Se encontró una carpeta pública de documentación enlazada desde un manual del vendedor

- Biblioteca LiDAR `https://drive.google.com/drive/folders/1Y5UCcPQQu9vQ9fj5v28oCBQBs3egtmY4`
- Carpeta Radar COIN-D6 `https://drive.google.com/drive/folders/1_XfZTemyIBoZssQmFW5Jx17jxx3hn337`
- Manual inglés `https://drive.google.com/file/d/10pA6OUonRKfPwFtCTezj35YqPVXA76NQ/view`
- Subcarpeta manuales `https://drive.google.com/drive/folders/1Wnvd2Q4A29xh0e60M3QPS1s5nBuLZyFD`
- Ficha Single-Line A1 `https://drive.google.com/file/d/1j-oi7kPCEAPHREYggjbOy9C-KsARW4Vk/view`
- Especificación protocolo EN `https://drive.google.com/file/d/11bBGx5RCkZnwQlYUY3u3yagJFsP3wJo0/view`
- Paquetes ROS/ROS2 `https://drive.google.com/drive/folders/1yiiONkmzb4KWPYYFDwfQyIUfweCqo4F7`
- Software Windows `https://drive.google.com/drive/folders/1VvkS_9-yEOnCZP06cbVsvULS9lwOo7jo`
- Demo STM32 `https://drive.google.com/drive/folders/12j5PsBnLiZHJcqOpoY4qnVsF-yyeSJDM`

El manual inglés se descargó y leyó. Su página 8 usa un arnés de ejemplo con colores **amarillo, blanco, rojo y negro** y dice amarillo a 5V, blanco a GND, rojo a TXD y negro a RX. **Esos colores NO son los del arnés del usuario, que lleva amarillo, verde, negro y rojo en posiciones distintas.** No transcribir esa tabla sobre el sensor del usuario. Se requiere el diagrama de posiciones del conector, no un cruce por semejanza de colores.

Se intentó leer la ficha de especificaciones con diagrama; la llamada fue interrumpida por el usuario antes de completar su análisis. Existe una descarga temporal posible, pero no se considera verificado su pinout. El QR no fue decodificado.

### 14.4 Datos documentales por verificar contra la unidad

El material COIN-D6 encontrado describe UART, alimentación nominal 5 V, 230400 baudios, mensajes de ángulo/distancia/intensidad y software CSPC. El manual documenta comandos hexadecimales de arranque `AA 55 F0 0F` y parada `AA 55 F5 0A`. **No se enviaron al equipo.** Verificar en la especificación de su modelo antes de usarlos. Son bytes binarios, no texto literal enviado con New Line.

No se instaló SDK, programa CSPC ni controlador adicional durante la investigación. COM9 ya aparecía. No se generó `/scan` ni nube de puntos.

### 14.5 Siguiente paso correcto

Abrir la ficha del modelo, ver el dibujo del conector con referencia física y numeración, cruzarlo con amarillo/verde/negro/rojo de la foto y verificar la alimentación y niveles del adaptador. Solo entonces entregar tabla color → función → pin. La prueba inicial prevista es LiDAR → adaptador → computador, sin incorporarlo todavía a ESP32.

No insistir únicamente en pedir más fotos ya enviadas. Hay material documental localizado para continuar la investigación. Si las fuentes no resuelven una ambigüedad, especificar exactamente qué falta.

## 15. Registro de decisiones y errores de análisis

| Tema | Decisión / corrección | Consecuencia para la próxima IA |
|---|---|---|
| Dos ESP32 | DevKitC para actuación/sensores, S3 para cámara/enlace | No volver al S3 como único controlador por seguir diagramas antiguos |
| Independencia de motores | Doce señales separadas | Conservar PWM individual; distinguir velocidad de dirección |
| VCC/VM | 3V3 lógico y potencia XL4016 separada | Evitar el falso diagnóstico «todo driver va a fuente de motor» |
| GND | Red común, retornos de potencia separados | No usar un cable fino de MPU como retorno de motores |
| Multiconector | Compartir 3V3 no demuestra por sí solo fallo | Verificar continuidad y corriente antes de culparlo |
| MPU | Otro GND permitió lectura | No declarar sensor malo por fallo inicial ni bueno solo por LED |
| Conteos | Diferencias por giros manuales distintos | No inventar desfase de sensores a partir de esos datos |
| Mecánica | Equipo revisa ruedas y acepta avance | Registrar pendiente sin bloquear todo con recalibración repetitiva |
| Distancia | Referencia empírica terminó en ~30 cm físicos | Respaldar versión real y no cargar constantes provisionales |
| Firmware | Sketch de encoders no mueve el carro | Mirar banner/comandos antes de diagnosticar driver |
| Arduino | Selección de target importa | No eliminar #error de placa para ocultar configuración incorrecta |
| Programación | BOOT + EN funcionó | Reutilizar procedimiento ante el mismo fallo, sin desarmar todo primero |
| Cámara | PID 0x5640 y mapa probado | Actualizar OV56 a OV5640 en continuidad técnica |
| Página lenta | Espera JS 1000 ms y HTTP por foto | Timeout del socket 50 ms no corrige ese cuello de botella |
| MJPEG | Streaming persistente y medición de FPS | No prometer 30 FPS sin resultados de hardware |
| PSRAM | Detección sigue pendiente | Última versión permite DRAM; no afirmar PSRAM resuelta |
| Cámara+UART | Usuario afirma prueba completada | No pedir repetir etapa por olvido del asistente |
| LiDAR | Usuario decidió priorizarlo | Continuar pinout/lectura D6 antes de nuevas pruebas de 30 cm |
| Documentación | Primer PDF tenía demasiado texto | Usuario prefiere láminas, componentes reconocibles y cables claros |
| Esquemas iniciales | Parecían definitivos con valores aún abiertos | Señalar propuestas y no certificar montaje solo con un dibujo |
| Cámara física | No se pudo confirmar orientación de contactos con fotos iniciales | El montaje posterior funcionó; no inventar descripción de orientación ya no observada |

Otros errores que deben evitarse

- Asociar LM393 con imanes por defecto. Es un comparador y el montaje observado usa discos ópticos.
- Llamar «control cerrado de velocidad» a aplicar mismo PWM y contar distancia.
- Afirmar que a 3V3 «no alcanza corriente» solo porque un LED no enciende. El trayecto GND resultó decisivo en este caso.
- Convertir un fallo de compilación por llaves en un problema de alimentación o de placa.
- Presentar un banner de UART con motores deshabilitados como un firmware de navegación.
- Interpretar el reset `SPI_FAST_FLASH_BOOT 0x13` de ejecución normal como éxito de modo descarga. El contexto del error es lo importante.
- Dar por hecho que las pruebas autónomas, cámara simultánea con motores o ruido eléctrico están certificadas por una prueba aislada.
- Sustituir hardware o proponer compras sin instrucción del usuario.

## 16. Archivos disponibles y diferencias con lo cargado

Raíz de proyecto

```text
C:\Users\AndresPC\Documents\Univesidad\UNIVERSIDAD\SEMESTRE VIII\
PROGRAMACION DISTRIBUIDA Y PARALELA\Proyecto de Clase PDyP
```

| Archivo | Papel / precaución |
|---|---|
| AGENTS.md | Contexto y reglas. Tiene diagnóstico de 29-oct y arquitectura conceptual parcialmente superados |
| MAPA_CONEXIONES.md | Tablas completas y etiquetas, revisión de alimentación; estados antiguos de validación |
| REVISION_CABLEADO.md | Auditoría de errores de instrucciones y alimentación; diagnóstico del MPU quedó superado por prueba posterior |
| pruebas_uart/devkitc_receptor/devkitc_receptor.ino | UART HOLA/RECIBIDO sin cámara ni movimiento |
| pruebas_uart/s3_emisor/s3_emisor.ino | S3 emisor, RX42/TX41 |
| pruebas_encoders/devkitc_encoders/devkitc_encoders.ino | Conteo manual y UART, sin motores habilitados |
| pruebas_mpu/devkitc_mpu/devkitc_mpu.ino | Diagnóstico MPU+UART, drivers deshabilitados |
| pruebas_distancia/devkitc_distancia/devkitc_distancia.ino | Control de distancia provisional, constantes antiguas; no representa calibración final |
| esquema-01-motores.svg/.png | Esquema funcional de motores, pines ordenados para lectura, no ubicación física |
| esquema-02-sensores-uart.svg/.png | Esquema funcional de sensores/enlace |
| esquema-03-alimentacion.svg/.png | Topología propuesta, parámetros pendientes |
| diseno-montaje-dos-esp32.svg | Distribución orientativa sobre chasis |
| montaje-robot-4wd.drawio | Plano antiguo, no usar para cablear sin compararlo con mapa actual |
| Guía de integración y pruebas - Robot 4WD.pdf | 13 páginas de planificación, creado 10-nov; extenso para preferencia del usuario |
| Guía visual de montaje - Robot 4WD.pdf | 8 láminas, chasis, motores, potencia, sensores, UART, cámara, prueba y ruta |
| MEMORIA_SUPREMA.md | Este documento, estado consolidado y anexos |

No hay un sketch de cámara dentro de esta carpeta de proyecto. Durante la revisión se encontró la copia actual en C:/Users/AndresPC/Documents/Arduino/sketch_oct22a/sketch_oct22a.ino, que contiene el último MJPEG con alternativa DRAM. Su copia íntegra se conserva en el anexo A. No se recuperó el sketch final de 30 cm. También se recuperó el texto pegado antiguo de distancia, de 5-nov, con DIAMETRO_CM=0 y conteos 148/36/130/202, archivado en el anexo C.

Las fotos del usuario viven en rutas temporales `C:\Users\AndresPC\AppData\Local\Temp\codex-clipboard-...`. Esas rutas pueden desaparecer y no acompañan automáticamente este MD.

## 17. Pendientes y ruta de integración

### 17.1 Trabajo inmediato autorizado

1. Concluir identificación de modelo/pinout del D6 con el material ya localizado.
2. Preparar conexión al adaptador CH340 por interfaz UART TTL correcta, con alimentación adecuada y referencia común, después de verificar posiciones.
3. Abrir COM9 actual en software del sensor y comprobar inicio, distancias y nube de puntos.
4. Guardar modelo, diagrama de posiciones, colores reales, velocidad serial, protocolo y alimentación utilizados.

### 17.2 Recuperación de versiones funcionales

Respaldar sketch final de 30 cm y sketch que el usuario haya usado para cámara+UART. No reemplazarlos por archivos locales antiguos. Si se integra software nuevo, conservar los factores, sentidos y límites que ya funcionaron.

### 17.3 Integración pendiente del robot

- Consolidar firmware del DevKitC con motores, encoders, IMU y UART, con parada local y límites.
- Consolidar S3 con cámara y puente de comunicación sin bloquear órdenes por streaming.
- Medir latencia y parada por pérdida de enlace, STOP prioritario, reset seguro y reconexión sin reanudación automática.
- Verificar batería, fusible, VM, 5V, 3V3, corrientes y ausencia de reinicio bajo carga.
- Resolver o documentar la ausencia de PSRAM según la configuración realmente cargada.
- Fijar mecánica de ruedas antes de odometría de giros y trayectos largos.
- Elegir interfaz LiDAR→PC/ROS y mantener timestamps y marcos.
- Crear puente `/cmd_vel` y telemetría, luego odometría y TF.
- Modelar robot en Gazebo si aporta al objetivo de clase y reutilizar las interfaces en robot real.
- Validar SLAM/localización/Nav2 con alcance y driver reales del D6 antes de afirmar autonomía con obstáculos.
- Añadir visión semántica/VLM y objetivo geométrico validado después de control básico.

### 17.4 Propuestas de planificación que no son funciones actuales

Se propusieron estados DESARMADO/LISTO/MOVIENDO/FALLO, mensajes CMD/ACK/FIN/ERR con identificadores, latidos cada 100 ms y vencimiento inicial de 400 ms, y control continuo VEL en m/s y rad/s. Son ideas de diseño pendientes de medir e implementar.

Para odometría diferencial, se propuso promedio de recorridos por lado y cambio angular (derecha−izquierda)/trocha efectiva. Trocha real y efectiva no se midieron en esta conversación. La fusión EKF con IMU también es futura.

La planificación sugirió un escenario visual inicial con marcador/objeto de posición conocida, detección en PC y objetivo validado. No se entrenó un modelo ni se decidió que deba entrenarse desde cero.

## 18. Documentación de clase y fuentes

### 18.1 Material de clase

En carpetas vecinas se encontraron trabajos de visión/lenguaje de Clase 01 y un ros-lab de Clase 05. Se leyó la portada/resumen de `Visual_Language_Maps_for_Robot_Navigation.pdf`, que combina semántica con reconstrucción espacial. No se puede afirmar que se hayan analizado íntegramente todos los artículos.

Otros títulos localizados incluían VLM-Social-Nav, Vision-Language-Action Models for Robotics, Physically Grounded VLMs, LLM as Policy Teacher, Co-NavGPT y microservicios de robótica cloud. Clase 05 contiene docker.pdf; Clase 09, material de Vue y Spring/GraphQL. No todo aplica directamente al montaje actual.

La memoria `../Clase 05/ros-lab/MEMORIA_PRINCIPAL.md` identifica un entorno existente **ROS 2 Humble + Ignition Gazebo Fortress en Docker**. El intento viejo `PDyP/ROS2LINUX` de WSL no debe confundirse con ese laboratorio. La planificación también mencionó Jazzy/Harmonic como opción para un entorno nuevo, no como migración ya hecha ni obligación.

El usuario anunció nuevas guías del profesor, pero no se adjuntaron durante la preparación del primer PDF. Adaptar el objetivo académico al hardware disponible, sin compras ni sustituciones automáticas. Conservar `sources/` sin cambios si existe.

### 18.2 Fuentes técnicas consultadas

- Espressif, DevKitC V4 y alimentación `https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html`
- Espressif, arranque ESP32 `https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html`
- Espressif, fallos de esptool `https://docs.espressif.com/projects/esptool/en/latest/esp32/troubleshooting.html`
- Espressif, controlador de cámara `https://github.com/espressif/esp32-camera`
- Espressif, CameraWebServer `https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/Camera/CameraWebServer`
- Freenove, referencia de pines `https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board/blob/main/C/Sketches/Sketch_07.3_Camera_SDcard/camera_pins.h`
- Espressif, Wi-Fi AP `https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html`
- Toshiba, TB6612FNG `https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG`
- WCH, CH340 `https://www.wch.cn/products/CH340.html`
- Gazebo, ROS `https://gazebosim.org/docs/harmonic/ros_installation/`
- Nav2, configuración del robot `https://docs.nav2.org/jazzy/configuration_and_development/first_time_robot_setup_guide/gazebo/`
- Biblioteca pública COIN-D6 y documentos concretos listados en sección 14. Preferir los originales a páginas que reescriben manuales.

Referencias iniciales compartidas por usuario, sin validación completa de contenidos en este corte

- Video original `https://www.youtube.com/watch?v=kurIK5AGhnw`.
- Artículo abierto de 4WD `https://adityakamath.github.io/2021-06-21-4wd-mobile-base-with-teensy4-esp32/`.
- Compra S3 Amazon ASIN B0GBT212KM.
- AliExpress de placa considerada `https://es.aliexpress.com/item/1005009425934712.html`.
- MercadoLibre ESP32UE ECO V3 MCOU3520226322 y ESP32-C3 MCO2062751451, alternativas evaluadas.

Se discutieron Tinkercad y Wokwi para visualizar montaje realista. El usuario quería ver cable por cable y no quedó una simulación electrónica integral funcionando con todos los componentes. No afirmar que el plano Draw.io es un simulador.

## 19. Evidencias fotográficas

Para buscar en el historial o temporales. Son nombres de evidencia, no garantía de que los archivos sigan en disco.

| Grupo | Identificadores de adjuntos |
|---|---|
| Chasis inicial | 8e1e6d4e-bfb1-4e3e-909b-9a74fd4a611d; 30a02357-be41-4d91-b78a-4c3b85fd7519; a58cce14-26c7-4cdd-aaef-b01a85c467f6 |
| Primeras fotos S3 | e8ec069c-8e54-426d-a68f-9dcd2247f6f7; c505023b-fb58-44f2-af2f-32a801239bb2 |
| DevKitC ESP-32D | 48ae2a77-5f6b-4acf-ac7e-152db03890cf; 81259da1-4e98-4242-9980-7acfc40bbc21 |
| HOLA repetidos DevKitC | df300787-8027-4ff3-8f3b-a612fd33eafc |
| MPU físico | 71d06290-8d54-4dfb-88ac-7b768642a633; 218b5aa4-535b-4b71-80f7-24323f6488cd |
| MPU con datos | 6efbe803-f469-4fb5-a39a-52d1227be7e4 |
| S3/cámara montaje reciente | c268692b-995e-4c4e-a312-52c29588f2bc; 459681c9-5344-4200-97e2-24e982cf30f5; 45aeb57b-0b5a-4426-80a8-7e800823e92e; 1af41663-1d9d-46ed-b572-296ffefa40b0 |
| Cámara PID/capturas | eb8b9aef-3e6d-4eea-805f-b9668cd5c3ec |
| Página visible y lenta | f2b64132-00b1-4446-ac43-92feea2a6814 |
| Fallo PSRAM | 8f562892-1446-438a-a186-2224fef47c63; 7154cebb-954d-4d44-a5be-85c5c25abc2c |
| Adaptador LiDAR | 1946cf86-ef70-440a-b8f1-46c1c7004853; 5edfb018-5838-4b0a-858c-53fd0d5b6c08 |
| Sensor primera cara posterior | 085e886e-1fa2-4b0e-8c4e-2b01579a60fb |
| COM9 | df12571a-9290-4c59-81bf-7a20438f69b8 |
| Sensor frente/etiqueta/conector | 2b463400-c413-4159-b8c8-3f4e4c785191; 8eba0130-3810-48bb-97c9-022ffb2be25e; a4c47023-d5d5-4400-b7e5-69abd1d3dfe3 |

## 20. Contrato de relevo para otra IA

1. Hablar español y explicar paso por paso con nombres físicos, colores solo cuando estén verificados y GPIO explícitos.
2. Leer esta memoria y comparar archivos antes de cambiar código o cableado.
3. Conservar el reparto de dos ESP32 y las pruebas aceptadas. No reiniciar el proyecto ni pedir calibración repetida por olvidar su historia.
4. La prioridad elegida por el usuario es LiDAR. Concluir el pinout usando documentación real y posiciones del conector.
5. No adjudicar amarillo/verde a TX/RX por costumbre ni rojo/negro a potencia sin verificar este arnés.
6. Respaldar versiones funcionales antes de integrar. No confundir copias de conversación con archivos actualizados en disco.
7. Distinguir prueba física, reporte del usuario, propuesta y limitación.
8. Si algo falla, aislar la causa con un cambio por vez y una comprobación concreta. El usuario no desea desarmar todo sin motivo.
9. Para la documentación, preferir dibujos legibles, fotografías reales, pines y cables sobre texto excesivo. Para código, entregar el sketch completo cuando el usuario lo pida.
10. No comprar, sustituir componentes, crear infraestructura pesada ni afirmar autonomía solo por tener cámara y encoders.

### Mensaje de arranque sugerido para el relevo

> El movimiento de 30 cm está aceptado, la cámara OV5640 ya entrega imagen web y el usuario confirma comunicación entre ESP32 y prueba cámara+UART. PSRAM sigue sin detectarse y el último MJPEG permite usar DRAM. Estamos identificando el arnés del LiDAR probable COIN-D6 para probarlo en PC mediante el adaptador CH340 ya reconocido como COM9. Hay documentación original localizada, pero todavía no se confirmó la correspondencia eléctrica de los cuatro colores del arnés real.

## 21. Anexos de código y documentos existentes

Los anexos siguientes conservan copias de referencia para facilitar el análisis fuera de esta carpeta. Los scripts locales antiguos se rotulan como tales. El último sketch de cámara se leyó de la copia guardada en Documents/Arduino y corresponde al entregado en conversación, sin ser una lectura desde la placa. El firmware final de distancia con calibración aceptada sigue pendiente de recuperación íntegra.
## Anexo A — Copia exacta del sketch de cámara encontrado en Arduino

Ruta C:/Users/AndresPC/Documents/Arduino/sketch_oct22a/sketch_oct22a.ino. Leído el 14-nov-2026. Contiene el último MJPEG con alternativa DRAM y no inicializa UART de aplicación. Es copia del archivo guardado, no lectura del binario de la placa. No se compiló ni se probó hardware al redactar esta memoria. Se normalizan únicamente los saltos de línea.

SHA-256 del archivo original

`63568a9ef2ca9f651c7112cb52a70c7e79060a8a2f84c171b5ae20bfbe4621b2`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Selecciona ESP32S3 Dev Module."
#endif

const char *NOMBRE_WIFI = "RobotCam";
const char *CLAVE_WIFI = "CAMBIA_CLAVE_ROBOTCAM";

httpd_handle_t servidorWeb = nullptr;
httpd_handle_t servidorVideo = nullptr;

constexpr framesize_t RESOLUCION = FRAMESIZE_QVGA;  // 320 x 240
constexpr int CALIDAD_JPEG = 16;

const char PAGINA[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>RobotCam</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background: #142b43;
      color: white;
      margin: 24px;
    }
    img {
      display: block;
      width: 100%;
      max-width: 640px;
      margin: 20px auto;
      border-radius: 12px;
      background: #263747;
    }
    a, button {
      color: #8deadb;
      font-size: 16px;
    }
    button {
      background: #263747;
      padding: 10px 18px;
      border: 1px solid #8deadb;
      border-radius: 8px;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <h1>RobotCam</h1>
  <p>Video continuo MJPEG</p>

  <img id="video" alt="Video de la camara">

  <button onclick="conectarVideo()">Reconectar video</button>

  <p>
    <a href="/captura" target="_blank">
      Abrir una captura para guardarla
    </a>
  </p>

  <script>
    const video = document.getElementById("video");

    function conectarVideo() {
      video.src = "http://" + location.hostname
                + ":81/stream?t=" + Date.now();
    }

    conectarVideo();
  </script>
</body>
</html>
)HTML";

esp_err_t mostrarPagina(httpd_req_t *peticion) {
  httpd_resp_set_type(peticion, "text/html; charset=utf-8");
  httpd_resp_set_hdr(peticion, "Cache-Control", "no-store");

  return httpd_resp_send(
    peticion,
    PAGINA,
    HTTPD_RESP_USE_STRLEN
  );
}

esp_err_t enviarCaptura(httpd_req_t *peticion) {
  camera_fb_t *imagen = esp_camera_fb_get();

  if (imagen == nullptr) {
    return httpd_resp_send_err(
      peticion,
      HTTPD_500_INTERNAL_SERVER_ERROR,
      "No se pudo capturar la imagen."
    );
  }

  httpd_resp_set_type(peticion, "image/jpeg");
  httpd_resp_set_hdr(peticion, "Cache-Control", "no-store");
  httpd_resp_set_hdr(
    peticion,
    "Content-Disposition",
    "inline; filename=robot.jpg"
  );

  esp_err_t resultado = httpd_resp_send(
    peticion,
    reinterpret_cast<const char *>(imagen->buf),
    imagen->len
  );

  esp_camera_fb_return(imagen);
  return resultado;
}

esp_err_t transmitirVideo(httpd_req_t *peticion) {
  esp_err_t resultado = httpd_resp_set_type(
    peticion,
    "multipart/x-mixed-replace;boundary=robotframe"
  );

  if (resultado != ESP_OK) {
    return resultado;
  }

  httpd_resp_set_hdr(peticion, "Cache-Control", "no-store");

  uint32_t cuadros = 0;
  uint32_t inicioMedicion = millis();

  Serial.println("VIDEO | cliente conectado");

  while (true) {
    camera_fb_t *imagen = esp_camera_fb_get();

    if (imagen == nullptr) {
      Serial.println("ERROR | fallo de captura durante video");
      return ESP_FAIL;
    }

    char encabezado[128];

    int longitud = snprintf(
      encabezado,
      sizeof(encabezado),
      "\r\n--robotframe\r\n"
      "Content-Type: image/jpeg\r\n"
      "Content-Length: %u\r\n\r\n",
      static_cast<unsigned int>(imagen->len)
    );

    resultado = httpd_resp_send_chunk(
      peticion,
      encabezado,
      longitud
    );

    if (resultado == ESP_OK) {
      resultado = httpd_resp_send_chunk(
        peticion,
        reinterpret_cast<const char *>(imagen->buf),
        imagen->len
      );
    }

    // Devolver siempre el bufer, incluso al desconectarse el cliente.
    esp_camera_fb_return(imagen);

    if (resultado != ESP_OK) {
      Serial.println("VIDEO | cliente desconectado");
      return resultado;
    }

    ++cuadros;

    const uint32_t ahora = millis();
    const uint32_t transcurrido = ahora - inicioMedicion;

    if (transcurrido >= 3000) {
      const float fps = cuadros * 1000.0f / transcurrido;

      Serial.printf("VIDEO | FPS enviados: %.1f\n", fps);

      cuadros = 0;
      inicioMedicion = ahora;
    }

    delay(1);
  }
}

bool iniciarServidores() {
  httpd_config_t web = HTTPD_DEFAULT_CONFIG();
  web.server_port = 80;
  web.stack_size = 8192;
  web.send_wait_timeout = 3;

  if (httpd_start(&servidorWeb, &web) != ESP_OK) {
    return false;
  }

  httpd_uri_t pagina = {};
  pagina.uri = "/";
  pagina.method = HTTP_GET;
  pagina.handler = mostrarPagina;

  httpd_uri_t captura = {};
  captura.uri = "/captura";
  captura.method = HTTP_GET;
  captura.handler = enviarCaptura;

  if (httpd_register_uri_handler(servidorWeb, &pagina) != ESP_OK ||
      httpd_register_uri_handler(servidorWeb, &captura) != ESP_OK) {
    httpd_stop(servidorWeb);
    servidorWeb = nullptr;
    return false;
  }

  httpd_config_t video = HTTPD_DEFAULT_CONFIG();
  video.server_port = 81;
  video.ctrl_port = web.ctrl_port + 1;
  video.stack_size = 8192;
  video.send_wait_timeout = 3;

  if (httpd_start(&servidorVideo, &video) != ESP_OK) {
    httpd_stop(servidorWeb);
    servidorWeb = nullptr;
    return false;
  }

  httpd_uri_t stream = {};
  stream.uri = "/stream";
  stream.method = HTTP_GET;
  stream.handler = transmitirVideo;

  if (httpd_register_uri_handler(servidorVideo, &stream) != ESP_OK) {
    httpd_stop(servidorVideo);
    httpd_stop(servidorWeb);

    servidorVideo = nullptr;
    servidorWeb = nullptr;
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("ROBOTCAM | VIDEO CONTINUO MJPEG");

  const bool tienePsram = psramFound();

  Serial.printf(
    "PSRAM detectada: %s | total: %lu bytes\n",
    tienePsram ? "SI" : "NO",
    static_cast<unsigned long>(ESP.getPsramSize())
  );

#ifdef BOARD_HAS_PSRAM
  Serial.println("Compilacion: soporte PSRAM habilitado");
#else
  Serial.println("Compilacion: soporte PSRAM deshabilitado");
#endif

  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  // Mismo mapa que funciono con tu camara OV5640.
  config.pin_d0 = 11;
  config.pin_d1 = 9;
  config.pin_d2 = 8;
  config.pin_d3 = 10;
  config.pin_d4 = 12;
  config.pin_d5 = 18;
  config.pin_d6 = 17;
  config.pin_d7 = 16;

  config.pin_xclk = 15;
  config.pin_pclk = 13;
  config.pin_vsync = 6;
  config.pin_href = 7;

  config.pin_sccb_sda = 4;
  config.pin_sccb_scl = 5;

  config.pin_pwdn = -1;
  config.pin_reset = -1;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = RESOLUCION;
  config.jpeg_quality = CALIDAD_JPEG;

  if (tienePsram) {
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
    Serial.println("VIDEO | dos buferes en PSRAM");
  } else {
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    Serial.println("VIDEO | un bufer en memoria interna");
  }

  const esp_err_t resultado = esp_camera_init(&config);

  if (resultado != ESP_OK) {
    Serial.printf(
      "ERROR CAMARA | 0x%X\n",
      static_cast<unsigned int>(resultado)
    );
    return;
  }

  sensor_t *sensor = esp_camera_sensor_get();

  if (sensor != nullptr) {
    Serial.printf(
      "Sensor detectado: 0x%04X\n",
      static_cast<unsigned int>(sensor->id.PID)
    );
  }

  WiFi.mode(WIFI_AP);

  // Un dispositivo Wi-Fi para esta prueba.
  if (!WiFi.softAP(NOMBRE_WIFI, CLAVE_WIFI, 1, false, 1)) {
    Serial.println("ERROR | no se pudo crear RobotCam");
    esp_camera_deinit();
    return;
  }

  if (!iniciarServidores()) {
    Serial.println("ERROR | no se pudieron iniciar los servidores");
    WiFi.softAPdisconnect(true);
    esp_camera_deinit();
    return;
  }

  Serial.println("CAMARA Y VIDEO LISTOS");
  Serial.printf("Wi-Fi: %s\n", NOMBRE_WIFI);
  Serial.printf("Clave: %s\n", CLAVE_WIFI);

  Serial.print("Pagina: http://");
  Serial.println(WiFi.softAPIP());

  Serial.println("Abre la pagina para medir los FPS.");
}

void loop() {
  // Los servidores HTTP se ejecutan en tareas independientes.
  delay(100);
}
```

## Anexo B — Instantáneas de los archivos del proyecto

Copias leídas el 14-nov-2026. Incluyen estados históricos. Para las pruebas recientes consultar el cuerpo de esta memoria. El archivo de distancia conserva constantes provisionales. Se normalizan únicamente los saltos de línea de las copias.

### pruebas_uart/devkitc_receptor/devkitc_receptor.ino

SHA-256 del archivo original

`3a9f61d0846167f7a6145a0075e30cb5eb1eee2b29018c4bfa9b348f2514edee`

````cpp
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
````

### pruebas_uart/s3_emisor/s3_emisor.ino

SHA-256 del archivo original

`d672cca50c240a6b614db295deb45fde71e8b4e1d75de62357e1721fa50db351`

````cpp
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
````

### pruebas_encoders/devkitc_encoders/devkitc_encoders.ino

SHA-256 del archivo original

`849b54a39d88709efeca21147dde65a918c6f7a951d481e35eaa8fdcd4e83c89`

````cpp
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
````

### pruebas_mpu/devkitc_mpu/devkitc_mpu.ino

SHA-256 del archivo original

`5db4e6589e937afe3e4ad7dbc9b1e2ad46b305016fc0c6fc07ec1c1836be6c4b`

````cpp
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
````

### pruebas_distancia/devkitc_distancia/devkitc_distancia.ino

SHA-256 del archivo original

`7148a2dd4481ab5864830ce0dcfa3a974afebf149d48baec9ccfc084243ee854`

````cpp
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
````

### pruebas_uart/LEEME.md

SHA-256 del archivo original

`e5101d83725dab4776d4131eaaa90a9091fa10bf406abf3e8188b982a94ce152`

````markdown
# Prueba UART entre los dos ESP32

Son dos programas separados. No pegarlos juntos. No requieren bibliotecas externas: solo el paquete ESP32 de Espressif para Arduino IDE.

Estado: fuentes revisadas; todavía sin compilación para las placas ni prueba física. No cuentan encoders, leen IMU ni inician cámara. El programa del DevKitC mantiene STBY y las doce señales de motor en bajo mientras se ejecuta; no protege el intervalo anterior a setup ni reemplaza la verificación de hardware. Mantener VM desconectado.

## Ahora: DevKitC V4 conectado

1. Batería/fuentes externas desconectadas, VM de ambos drivers sin alimentación. No usar USB y entrada 5V simultáneamente.
2. Con ambas placas apagadas, retirar temporalmente las dos señales UART (S3 41 a DevKitC 16 y S3 42 a DevKitC 17). GND puede permanecer.
3. Conectar únicamente DevKitC a USB. No conectar S3 todavía.
4. Abrir `devkitc_receptor/devkitc_receptor.ino` en Arduino IDE.
5. Gestor de placas: debe estar instalado `esp32 by Espressif Systems`. Placa `ESP32 Dev Module`. Puerto: el COM que aparece al enchufar el DevKitC, sin inventar su número. Upload Speed 115200 si se quiere una primera carga conservadora. Resto de ajustes predeterminados de ESP32 Dev Module.
6. Pulsar Verificar y luego Subir. Si queda en Connecting, mantener BOOT durante inicio de conexión y soltar al comenzar escritura. No cambiar cables con USB conectado. Si falla, guardar error exacto antes de otro intento.
7. Abrir Monitor Serie a 115200. Cada 3 s: `DEVKITC ACTIVO | HOLA recibidos: 0 | motores deshabilitados`. El cero es normal hasta conectar S3 programado. Esto confirma ejecución local, todavía no enlace entre placas.

## Después: S3 por separado

1. Desconectar USB DevKitC. Señales UART aún retiradas.
2. Conectar S3 por USB-UART; abrir `s3_emisor/s3_emisor.ino`.
3. Placa `ESP32S3 Dev Module`, puerto COM del S3. USB CDC On Boot: Disabled, para Monitor Serie por puente USB-UART. Estos sketches mínimos no necesitan activar cámara ni PSRAM.
4. Verificar/Subir. Monitor Serie a 115200 mostrará HOLA y SIN RESPUESTA: normal estando solo.
5. Desconectar USB S3.

## Unir y probar

Con todo apagado: S3 41 (TX) a DevKitC 16 (RX), DevKitC 17 (TX) a S3 42 (RX), GND común. No unir salidas 3V3. Mantener cables cortos. Alimentar ambas por USB del mismo computador, sin alimentación externa, y evitar dejar una sola encendida durante la transmisión. VM desconectado.

En Monitor del DevKitC: `S3 -> HOLA | respuesta -> RECIBIDO | total: ...` incrementando. En S3: `DevKitC -> RECIBIDO | ida y vuelta OK`. Para observar ambos a la vez, usar dos ventanas IDE, cada una con su sketch, placa y COM; también se pueden revisar por separado sin apagar placas.

La recepción es una línea ASCII terminada en salto de línea; 115200, 8N1. No enviar órdenes de motor: se descartan. Una respuesta no es un protocolo robusto de movimiento: esta prueba no mide errores sostenidos ni valida funcionamiento con ruido, cámara o corriente de motores.

GPIO41/42 del S3 siguen basados en placa de referencia: probar en hardware sin cámara activa no certifica compatibilidad simultánea con cámara. Si faltan respuestas, guardar salidas de ambos monitores, apagar ambas placas y revisar continuidad/etiquetas antes de modificar GPIO. No cambiar aleatoriamente pines.

Fuente API: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/serial.html
````

### pruebas_encoders/LEEME.md

SHA-256 del archivo original

`b074d8a26e6b1ec8190ef6f1533c1e1c4b6a561e37e14487c2361ba61c9c13a4`

````markdown
# Prueba manual de los cuatro encoders

Programa separado para DevKitC V4. No sobrescribe los sketches originales de UART. El usuario compiló, cargó y confirmó funcionamiento en hardware el 2026-10-29: captura de los cuatro contadores con FL=80, RL=55, FR=140, RR=60; al detenerse, NUEVOS=0. Es evidencia de conteo básico, no de calibración ni de prueba con motores energizados.

La carga inicial falló por modo de arranque; entrada manual con BOOT + EN produjo DOWNLOAD_BOOT y la carga posterior terminó con verificación de hash. No se confirmó la causa física exacta de los fallos previos.

## Antes de cargar

- Batería, reguladores externos y VM de ambos drivers sin alimentación. Solo lógica por USB.
- D0: FL→34, RL→35, FR→VP/36, RR→VN/39. A0 sin conectar; GND común.
- VCC del encoder debe corresponder a su módulo real; si usan el riel 3V3, comprobar que opera ahí y que D0 cambia entre bajo y un alto compatible con GPIO. Nunca aplicar D0 de 5 V al ESP32. No activar esta prueba sobre alimentación no verificada. No alimentar motores desde USB.
- Placas apagadas al cambiar cables. Si solo DevKitC queda alimentado, retirar las dos señales UART hacia el S3 apagado (GND puede permanecer). Para mantener UART, alimentar ambas placas sin fuentes externas y no cambiar el sketch del S3.

## Arduino IDE

1. Abrir `devkitc_encoders/devkitc_encoders.ino` en una ventana propia.
2. Elegir `ESP32 Dev Module` y COM del DevKitC (última sesión: COM5). No subir al S3/COM6.
3. Conservar los ajustes con los que ya arrancó el receptor. Verificar y Subir.
4. Monitor USB a 115200. Al iniciar contadores quedan en cero; también escribir `r` y Enter para reiniciarlos.

## Qué observar

Cada 500 ms imprime TOTAL, NUEVOS (pulsos desde el reporte anterior), niveles instantáneos D0 y número de HOLA recibidos.

1. Sin tocar ruedas: NUEVOS debería quedar en 0 tras estabilizar; TOTAL debe permanecer fijo.
2. Girar solo FL una vuelta lenta: aumenta FL; las otras columnas deben quedar fijas.
3. Repetir RL, FR y RR, una rueda a la vez. No forzar engranajes ni dejar que el disco toque el sensor.
4. Para calibración preliminar, poner cero y dar varias vueltas completas de la rueda elegida en el mismo sentido; anotar pulsos/vueltas. No asumir 20 pulsos por vuelta sin medir.
5. El conteo crece en ambos sentidos: son sensores de un canal. Esta prueba no calcula direccion, RPM ni distancia.
6. Si el S3 sigue con emisor, UART_HOLA puede aumentar y el emisor recibe RECIBIDO como antes. No cambia el protocolo ni envía pulsos por UART.

El programa cuenta flancos descendentes sin filtro de rebote. No confundir conteo bruto con lectura calibrada: bordes ruidosos pueden sumar de más. Si cuenta sin movimiento o los resultados varían, revisar estabilidad mecánica, GND, pull-up del módulo, ruido y alineación antes de añadir filtrado. GPIO34–39 no admiten pull-up interno. Se deja radio y ADC sin iniciar, especialmente por las restricciones de GPIO36/39.

Si un contador no cambia: revisar la pareja D0/GPIO con ambas placas apagadas; comprobar alineación disco/sensor y medir D0 al bloquear/liberar óptica. El nivel D0 impreso es una muestra lenta: no siempre cambia visiblemente aun con pulsos. Un LED del módulo no prueba por sí solo que el ESP32 reciba una señal correcta.

Siguiente etapa tras validar los cuatro: leer IMU, terminar medidas de alimentación y hacer prueba individual de motores, todavía sin navegación.

Fuente API GPIO: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/gpio.html
````

### pruebas_mpu/LEEME.md

SHA-256 del archivo original

`2804ce1ae7e1eb97496cb5e8561bfdaaca123565d5a31ad0abe35a70f330a21d`

````markdown
# Prueba MPU6050 — DevKitC V4

Archivo: `devkitc_mpu/devkitc_mpu.ino`. No requiere librerias adicionales: Wire viene con Arduino ESP32. Estado: preparado y revisado, sin compilacion ni prueba local en la placa. Usar Verificar en el IDE.

## Cableado y carga

- MPU SDA → DevKitC 21; SCL → 22; GND → tierra comun.
- VCC segun modulo real verificado; SDA/SCL deben tener pull-ups a 3,3 V. No inferir compatibilidad a 5 V de la etiqueta MPU6050. AD0 determina 0x68/0x69; se prueban ambas direcciones. No dejar AD0 flotante si el modulo no fija su nivel. INT, XDA, XCL sin conectar.
- Bateria y fuentes externas apagadas, VM sin alimentar. Solo logica USB. Si S3 apagado, retirar las dos senales UART con todo apagado; si se mantiene enlace, alimentar ambas placas. No unir salidas 3V3.
- Abrir sketch separado. Placa ESP32 Dev Module, puerto DevKitC (antes COM5). No cargar al S3. Conservar ajustes de carga que funcionaron en prueba encoders.
- Verificar/Subir. Si hace falta modo descarga manual: mantener BOOT, pulsar/soltar EN; confirmar DOWNLOAD_BOOT, cerrar monitor antes de subir. No pulsar EN durante escritura. Al finalizar EN sin BOOT si queda esperando descarga.
- Monitor Serie 115200. Si se perdio el mensaje de deteccion inicial, EN una vez y leer desde inicio.

## Resultado esperado

`MPU6050 LISTO` seguido de aceleracion X/Y/Z en g y velocidades angulares en grados/s cada 500 ms. En reposo el modulo de aceleracion debe ser aproximadamente 1 g (gravedad). Si Z esta vertical, su lectura sera aproximadamente +1 o -1 g y X/Y proximos a cero; depende del montaje. El giroscopio quieto cerca de cero, con sesgo: no exigir cero exacto. La temperatura es interna del chip, no temperatura ambiente certificada.

Inclinar suavemente el chasis cambia la distribucion de gravedad entre ejes; al girarlo cambian las velocidades angulares durante el movimiento. Mover despacio y sujetar sin tensar cables. No desmontar la IMU para probarla ni forzar sus soportes.

Si WHO_AM_I no es 0x68 no configura el dispositivo; guardar salida y revisar identidad del sensor. Si no detecta o falla lectura, no interpretar ceros como datos validos. Revisar cables y niveles con alimentacion desconectada antes de reiniciar.

Esta prueba NO mide orientacion absoluta/yaw, no calibra sesgos ni fusiona sensores. Los sketches anteriores se conservan en disco; al subir este, se reemplaza el programa activo del DevKitC: encoders quedan temporalmente sin contar. Conserva respuesta RECIBIDO al HOLA del S3, pero no envia IMU por UART ni cambia sketch S3. Motores permanecen deshabilitados.

## Referencias

- https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html
- https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf
````

### MAPA_CONEXIONES.md

SHA-256 del archivo original

`3712ca677422c016249c45309772fe7a4c5a6b3e6a1c31ebd174c0f383d27ff4`

````markdown
# Mapa de conexiones — robot 4WD con dos ESP32

Estado: revisión de documentación 2026-10-22; diseño de cableado NO probado físicamente. Sustituye el mapa anterior que compartía dirección y PWM por lado. Los números son GPIO, no posiciones del conector. El DevKitC de las fotos es una variante con USB-C; la guía oficial es referencia de GPIO, no certifica su circuito de alimentación exacto.

## Láminas para seguir el montaje

1. `esquema-01-motores.svg`: doce señales independientes, salidas de los motores y conexiones compartidas.
2. `esquema-02-sensores-uart.svg`: cuatro OUT, bus I2C y enlace entre placas.
3. `esquema-03-alimentacion.svg`: ramas, retornos y condensadores.

Las láminas son esquemas funcionales: los pines de cada bloque están ordenados para leer cables, NO como aparecen físicamente en el módulo. Seguir siempre la etiqueta del pin real. El dibujo anterior `diseno-montaje-dos-esp32.svg` es únicamente una propuesta de ubicación sobre el chasis. El plano Draw.io antiguo no debe usarse para cablear.

## Reparto

### Auditoría de alimentación tras fallos de arranque (2026-10-29)

- **Se mantiene 3V3 DevKitC → VCC de ambos TB6612FNG.** Alimenta la lógica, no los motores. El TB6612 especifica VCC entre 2,7 y 5,5 V; escoger 3,3 V mantiene compatibilidad con los GPIO. No mover VCC a 5 V como corrección automática: con VCC=5 V, el mínimo VIH=0,7×VCC exige 3,5 V y un GPIO de 3,3 V no garantiza alto válido.
- **Se mantiene fuente de motores verificada → VM**, con GND común y retorno de potencia independiente. El puente H no aísla galvánicamente ni regula tensión.
- El usuario indica un multiconector desde 3V3 y una unión que comparte alimentación entre driver y MPU. La conexión compartida no demuestra por sí sola un error. No se confirmó por fotografía el pin real de cada extremo, la continuidad ni el consumo disponible. Colores no identifican voltajes.
- No se aprobó todavía alimentar el módulo MPU desconocido a un voltaje concreto: hay que identificar su regulador/pull-ups o documentación. No trasladar VCC a 5 V por ensayo; las líneas I2C deben permanecer compatibles con 3,3 V. No conectar VCC a GND.
- Se confirmó conteo manual de encoders, pero no consumo total/pull-ups/niveles medidos ni ruido con motores. No declarar que encender LED valida un módulo.
- Capturas posteriores muestran `flash read err, 988`, reinicios y fallo de carga/interrupción serial. No hay detección MPU exitosa documentada. No está demostrada la causa: revisar primero distribución/voltajes/cambios de cableado. No borrar memoria ni cambiar GPIO a ciegas.
- Si se aísla la MPU para diagnóstico, apagar todo y retirar sus cuatro conexiones eléctricas (VCC/GND/SDA/SCL), o desconectar su conector completo, para evitar alimentación por SDA/SCL. No hace falta desarmar el chasis ni retirar todo el robot. No dejar una placa apagada recibiendo UART de otra alimentada.
- El mapa utiliza GPIO2 y GPIO15 con restricciones de arranque; no fue una asignación libre de restricciones. Comprobar niveles al reset antes de reubicar señales. No se cambian esos pines automáticamente en este documento.

Orden de diagnóstico actual: apagar fuentes → foto/trazado del multiconector y etiquetas VCC/VM → continuidad de distribución sin energía → medida de 3V3 respecto a GND (solo tras descartar corto/conexión incorrecta y evitando puentear pines con puntas) → aislamiento local de MPU si el fallo persiste → nueva carga solo con alimentación y arranque estables. Si se mide continuidad/resistencia, hacerlo siempre sin USB/baterías/fuentes.

- ESP32 DevKitC V4 / ESP-WROOM-32D: cuatro motores independientes, cuatro entradas de encoder, MPU6050, control local y parada por pérdida de órdenes.
- ESP32-S3 N16R8 con conector de cámara: cámara, comunicación con computador y puente de órdenes/telemetría por UART.
- D6: sin asignación hasta identificar modelo, alimentación e interfaz.

## Driver izquierdo → DevKitC V4

| TB6612FNG | GPIO | Función |
|---|---:|---|
| AIN1 | 13 | Dirección FL |
| AIN2 | 14 | Dirección FL |
| PWMA | 25 | Velocidad FL |
| BIN1 | 26 | Dirección RL |
| BIN2 | 27 | Dirección RL |
| PWMB | 32 | Velocidad RL |
| STBY | 2 | Habilitación común de ambos drivers |
| VCC | 3V3 | Lógica |
| GND | GND | Tierra común |
| VM | pendiente | Fuente de motores con tensión verificada |

AO1/AO2: rojo/negro FL. BO1/BO2: rojo/negro RL. Colores no garantizan sentido de avance.

## Driver derecho → DevKitC V4

| TB6612FNG | GPIO | Función |
|---|---:|---|
| AIN1 | 18 | Dirección FR |
| AIN2 | 19 | Dirección FR |
| PWMA | 23 | Velocidad FR |
| BIN1 | 4 | Dirección RR |
| BIN2 | 15 | Dirección RR; pin de arranque, ver restricciones |
| PWMB | 33 | Velocidad RR |
| STBY | 2 | Habilitación común de ambos drivers |
| VCC | 3V3 | Lógica |
| GND | GND | Tierra común |
| VM | pendiente | Misma fuente regulada de motores |

AO1/AO2: rojo/negro FR. BO1/BO2: rojo/negro RR.

No unir AIN con BIN ni PWMA con PWMB. STBY, VCC y GND sí se distribuyen en paralelo. Compartir STBY permite detener todos los motores sin compartir velocidad ni dirección.

## Encoders → DevKitC V4

| Encoder | DO/OUT a GPIO |
|---|---:|
| FL | 34 |
| RL | 35 |
| FR | 36 (VP) |
| RR | 39 (VN) |

GND a tierra común. VCC depende del módulo exacto: no asumir alimentación compatible por el nombre LM393. Verificar que DO no supere 3,3 V. Si el módulo funciona correctamente a 3,3 V y su salida es compatible, usar 3V3; si necesita 5 V, adaptar la salida antes del GPIO. AO no se usa. GPIO34–39 son solo entradas y no tienen pull-up/pull-down internos; comprobar el pull-up del módulo. No conectar OUT a drivers ni a motores.

Un encoder óptico de un canal cuenta pulsos, no mide dirección por sí solo. La dirección inferida de la orden no detecta movimiento contrario, deslizamiento o rueda empujada.

GPIO36/39: no usar ADC ni ahorro de energía Wi-Fi/Bluetooth en este controlador durante el conteo. Se propone radio desactivada y UART cableada.

## MPU6050 → DevKitC V4

SDA → GPIO21; SCL → GPIO22; GND → GND. VCC según módulo verificado; mantener SDA/SCL y pull-ups a 3,3 V. AD0 a GND si el módulo lo permite para dirección 0x68; no alimentar módulos desconocidos sin revisar su placa. INT, XDA y XCL sin conectar en esta etapa. IMU fija y ejes documentados.

## UART entre placas

| Origen | Destino |
|---|---|
| S3 GPIO41 (TX propuesto) | DevKitC GPIO16 (RX) |
| DevKitC GPIO17 (TX) | S3 GPIO42 (RX propuesto) |
| GND S3 | GND DevKitC |

GPIO16/17 disponibles en WROOM-32D, no extrapolar a WROVER. GPIO41/42 del S3 son propuesta basada en placa de cámara de referencia; fabricante/esquema exacto del S3 no confirmado. Validar UART con motores sin alimentación antes de adoptar este mapa como probado. No conectar 3V3 de una placa con 3V3 de la otra. Señales UART de 3,3 V. Cables cortos, alejados de potencia; ambos equipos encendidos durante comunicación para evitar alimentación por señales.

No usar TX/RX impresos del DevKitC (GPIO1/3): se reservan para USB/programación. UART de aplicación se configura en 16/17. El computador puede controlar y recibir telemetría mediante el S3; la conexión de cámara/ROS aún requiere software.

## Arranque y restricciones

- GPIO2 → STBY: mantener bajo durante reset y programación; nunca puente a 3V3. El TB6612 tiene pull-down interno, pero verificar que el breakout no incorpore pull-up o puente STBY-VCC. Recomendada resistencia externa 10 kΩ de STBY a GND si se dispone de ella; no figura en inventario. No activar VM hasta comprobar STBY bajo durante arranque.
- GPIO15 → BIN2 RR: su nivel durante reset puede suprimir mensajes ROM. No añadir pull-up/pull-down sin revisar; el pull-down de la entrada TB6612 puede cambiar esos mensajes y no implica por sí solo un fallo de firmware. Motors deshabilitados mediante STBY mientras arranca.
- No usar GPIO0, 5, 12 ni D0/D1/D2/D3/CMD/CLK para este mapa. GPIO6–11 son memoria. EN es reset.
- Si el breakout modifica niveles de arranque, revisar el mapa; no eliminar protección ni forzar encendido.

## Alimentación pendiente de medidas

Batería (+) → fusible → interruptor → distribución → entradas de reguladores.

- Rama motores: LM2596/XL4016 elegido según tensión de batería, corriente real y motor; salida positiva → VM de ambos drivers; negativa → tierra común.
- Rama lógica: regulador con salida 5 V verificada → entradas 5V de ambas placas, solo cuando se compruebe compatibilidad de las placas reales. No usar alimentación externa simultánea con USB hasta verificar protección frente a retorno de corriente.
- 3V3 DevKitC → VCC de ambos drivers por distribución; sensores únicamente tras comprobar compatibilidad y presupuesto de corriente del regulador.
- Tierra común para placas, drivers, sensores y fuente, sin hacer circular retorno de corriente de motores por cables finos del ESP32/protoboard.
- Desacoplo de referencia Toshiba, por CADA driver: 10 µF en paralelo con 100 nF entre VM y GND, y 10 µF en paralelo con 100 nF entre VCC y GND, próximos al módulo. Revisar los condensadores ya incorporados antes de añadir otros. Electrolítico positivo a VM/VCC y negativo a GND; cerámico sin polaridad. Comprobar tensión nominal superior a la máxima prevista, incluidos transitorios. Capacidad adicional por determinar según ruido y pruebas. El inventario no confirma los valores disponibles.
- Revisar corriente del motor al arrancar y bloquearse, y capacidad continua y térmica del driver: el pico breve de la ficha no autoriza esa corriente continua. Tampoco elegir regulador por el amperaje publicitario sin considerar disipación, cableado y carga simultánea de cuatro motores. No dimensionar fusible o tensión de motores por su color o apariencia.
- No poner batería directamente en 3V3, VCC de drivers ni GPIO.

## Secuencia física

1. Batería y USB desconectados: fijar DevKitC junto a drivers; S3 cerca de cámara; IMU centrada. Ordenar cables y conservar acceso USB.
2. Verificar salidas de motores ya montadas: canal A delantero, B trasero, un motor por canal.
3. Distribuir GND y VCC lógico. Cablear las doce señales según tablas y STBY común.
4. Preparar OUT de encoders etiquetados; conectar solo después de medir/verificar su alimentación y salida.
5. Conectar IMU después de revisar niveles I2C.
6. Preparar UART cruzada; comprobar señales/telemetría con VM desconectado y firmware de arranque seguro.
7. Medir batería y reguladores sin cargas; completar fusible, tensión y alimentación.
8. Con ruedas elevadas, probar un motor por vez y parada por pérdida de comunicación. Corregir sentido en software o intercambiando cables con todo apagado.
9. Probar encoders manualmente y luego junto a cada motor; calibrar pulsos por vuelta antes del PID.

El plano Draw.io anterior no representa este reparto y debe tratarse como esquema conceptual antiguo.

## Fuentes

- https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html
- https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG

Inventario actualizado por usuario: se añadió DevKitC V4 / ESP-32D. No se considera comprado ningún otro controlador mencionado antes.

## Orientación para localizar los GPIO del DevKitC

Vista de componentes, módulo metálico/antena arriba y USB abajo. Comparar primero con la serigrafía de la placa real; no contar pines a ciegas. En las fotografías coincide con esta distribución de referencia:

| De arriba hacia abajo | Lado izquierdo | Lado derecho |
|---:|---|---|
| 1 | 3V3 | GND |
| 2 | EN | 23 |
| 3 | VP (36) | 22 |
| 4 | VN (39) | TX (1), reservado USB |
| 5 | 34 | RX (3), reservado USB |
| 6 | 35 | 21 |
| 7 | 32 | GND |
| 8 | 33 | 19 |
| 9 | 25 | 18 |
| 10 | 26 | 5, sin usar |
| 11 | 27 | 17 |
| 12 | 14 | 16 |
| 13 | 12, sin usar | 4 |
| 14 | GND | 0, sin usar |
| 15 | 13 | 2 |
| 16 | D2, memoria | 15 |
| 17 | D3, memoria | D1, memoria |
| 18 | CMD, memoria | D0, memoria |
| 19 | 5V | CLK, memoria |

La numeración de la primera columna es solo la fila de esta tabla. GPIO2 NO significa la fila 2.

## Lista de cables etiquetados

Usar conectores firmes y aislados; en señales con headers macho puede usarse Dupont hembra-hembra. En distribución, una bornera/protoboard para lógica o una unión soldada aislada; no meter dos conectores en un solo pin. Corriente de motores por conexiones adecuadas a la corriente medida, no por rieles de protoboard.

| Etiqueta | Desde | Hasta | Motivo |
|---|---|---|---|
| I1 | DevKitC 13 | IZQ AIN1 | Dirección motor FL, entrada 1 |
| I2 | DevKitC 14 | IZQ AIN2 | Dirección motor FL, entrada 2 |
| I3 | DevKitC 25 | IZQ PWMA | PWM exclusivo FL |
| I4 | DevKitC 26 | IZQ BIN1 | Dirección RL, entrada 1 |
| I5 | DevKitC 27 | IZQ BIN2 | Dirección RL, entrada 2 |
| I6 | DevKitC 32 | IZQ PWMB | PWM exclusivo RL |
| D1 | DevKitC 18 | DER AIN1 | Dirección FR, entrada 1 |
| D2 | DevKitC 19 | DER AIN2 | Dirección FR, entrada 2 |
| D3 | DevKitC 23 | DER PWMA | PWM exclusivo FR |
| D4 | DevKitC 4 | DER BIN1 | Dirección RR, entrada 1 |
| D5 | DevKitC 15 | DER BIN2 | Dirección RR, entrada 2; restricción de arranque |
| D6 | DevKitC 33 | DER PWMB | PWM exclusivo RR; etiqueta de cable, no sensor D6 |
| S1 | DevKitC 2 | STBY IZQ y DER en Y | Deshabilitar los cuatro motores |
| L1 | Distribución 3V3 DevKitC | VCC IZQ | Lógica compatible con GPIO |
| L2 | Misma distribución | VCC DER | Alimentación lógica compartida |
| G1 | Distribución GND | GND IZQ | Referencia y retorno del driver |
| G2 | Distribución GND | GND DER | Referencia y retorno del driver |
| E1 | LM393 FL OUT | DevKitC 34 | Conteo FL |
| E2 | LM393 RL OUT | DevKitC 35 | Conteo RL |
| E3 | LM393 FR OUT | DevKitC VP/36 | Conteo FR |
| E4 | LM393 RR OUT | DevKitC VN/39 | Conteo RR |
| EG1–EG4 | Cada LM393 GND | GND lógica | Referencia de los pulsos |
| EV1–EV4 | Cada LM393 VCC | Rama compatible por confirmar | No conectar hasta verificar salida |
| U1 | DevKitC 21 | MPU6050 SDA | Datos I2C |
| U2 | DevKitC 22 | MPU6050 SCL | Reloj I2C |
| UG | MPU6050 GND | GND lógica | Referencia I2C |
| UV | MPU6050 VCC | Rama compatible por confirmar | Evitar pull-ups I2C a 5 V |
| UA | MPU6050 AD0 | GND si está disponible y permitido | Dirección I2C 0x68 |
| C1 | S3 41 TX propuesto | DevKitC 16 RX | Órdenes de movimiento |
| C2 | DevKitC 17 TX | S3 42 RX propuesto | Telemetría y estado |
| C3 | S3 GND | GND lógica común | Referencia UART |
| P1 | Regulador motores OUT+ | VM IZQ | Potencia de FL/RL |
| P2 | Mismo OUT+ | VM DER | Potencia de FR/RR |

Las etiquetas D1/D2/D3 de cables NO son los pines D1/D2/D3 de memoria de la placa: esos pines físicos no se usan. Para evitar confusión, anotar siempre también la pareja GPIO → pin driver en la etiqueta.

## Cómo funciona y qué significa cada grupo

- IN1/IN2 seleccionan giro, parada o frenado. PWM regula la activación del canal; no garantiza RPM por sí solo. El encoder aporta la medición para ajustar después con control de velocidad.
- AO1/AO2 y BO1/BO2 son pares de potencia con polaridad reversible. Ningún cable de motor va permanentemente a GND. Un canal, un motor.
- VCC del TB6612 alimenta lógica, VM alimenta motores. Son redes distintas. Poner 5 V en VCC no es la elección de este diseño: con VCC 3,3 V los niveles de control quedan coherentes con el ESP32.
- GND común no significa usar un mismo cable fino para toda la corriente. Separar los retornos de drivers de los de lógica hasta la distribución común.
- El computador procesa visión/navegación; el S3 transporta órdenes; DevKitC controla motores y cuenta pulsos. La cámara no tiene que enviar imágenes por la UART entre ESP32.
- Parada por pérdida de órdenes, STBY bajo antes de habilitar y cuatro PWM inicialmente a cero son requisitos del firmware futuro; el cableado solo no los implementa.

## Completitud y límites

| Parte | Estado |
|---|---|
| Doce señales independientes de motores | Asignadas sin duplicar GPIO |
| STBY compartido | Asignado; comprobar nivel de reset y breakout |
| Cuatro OUT de encoder | Entradas asignadas; nivel/alimentación pendientes |
| I2C MPU6050 | Señales asignadas; módulo y pull-ups pendientes |
| UART DevKitC | GPIO16/17 asignados para WROOM-32D |
| UART del S3 | GPIO41/42 propuestos; verificar con cámara funcionando |
| Cámara | Conector propio del S3; modelo/pinout exactos pendientes |
| Sensor D6 | Sin conectar: falta identificarlo |
| Alimentación y protecciones | Topología definida; valores/capacidad sin cerrar |
| Simulación/prueba real | No realizadas |

No se puede llamar listo para energizar al conjunto hasta resolver estas celdas pendientes. No se requieren compras automáticas: primero revisar los módulos y componentes disponibles.
````

### REVISION_CABLEADO.md

SHA-256 del archivo original

`cadfcdcb0acca46099a71da1a90e1b587729e7c998fd1bb3aa82701d3d44885f`

````markdown
# Revisión de indicaciones y cableado

Revisión 2026-10-29. Referencia activa: MAPA_CONEXIONES.md. No certifica el montaje físico: faltan fotos de los extremos y medidas.

## Lo que sí se mantiene

| Conexión | Evaluación |
|---|---|
| 3V3 DevKitC → VCC de dos TB6612 | Correcta para lógica compatible; verificar breakout y corriente total del regulador |
| Multiconector divide 3V3 en ramas | Válido si contactos firmes, aislados, con continuidad correcta y sin sobrecarga |
| XL4016/fuente motores → VM de drivers | Topología adecuada; tensión y corriente deben verificarse antes de conectar |
| AO1/AO2 y BO1/BO2 → un motor por canal | Correcto; ningún cable motor directamente a 3V3/GPIO |
| GND común | Necesario; retorno motor no debe pasar por cable fino de ESP/IMU |
| Encoders D0→34,35,VP36,VN39 | Adecuado para entradas; usuario confirmó prueba manual |
| MPU SDA21/SCL22 | Asignación adecuada; no identifica alimentación del módulo |
| UART16/17 del DevKitC | Adecuado para WROOM; llegada de mensajes confirmada, no ruido con motores/cámara |

## Qué estuvo mal, fue incompleto o necesitó corrección

1. Se presentó demasiado pronto un mapa como completo mientras faltaban valores de alimentación, características de breakouts y pines reales de cámara. Las láminas son propuestas, no un esquema verificado listo para energizar.
2. Se sugirió repartir 3V3 entre drivers y sensores sin cerrar presupuesto de corriente y fichas de módulos. Compartir no es el error por sí mismo; faltó verificar carga y niveles antes de energizarlo todo. La MPU sigue sin identificar físicamente.
3. La primera propuesta unía PWM/dirección por lado; no satisfacía el objetivo posterior de independencia por motor. Ya fue sustituida por doce señales separadas. No conservar puentes PWMA/PWMB o AIN/BIN del diseño agrupado.
4. Se usaron GPIO2 y GPIO15, que son de arranque: necesitan condiciones documentadas. GPIO2/STBY no debe estar forzado alto para programar; GPIO15 bajo puede silenciar ROM. No atribuirles todos los fallos sin medirlos.
5. Se describió inicialmente que el encoder indicaría sentido y se mencionaron imanes como si LM393 definiera tecnología. LM393 es un comparador presente en distintos módulos; el sensor óptico de una salida con disco regular no determina sentido por sí solo. Contar pulsos no equivale a odometría validada.
6. Se planteó retirar solo VCC de la MPU como prueba. Para aislamiento eléctrico fiable, desconectar también SDA/SCL, evitando alimentación por señales. GND puede mantenerse como referencia, pero retirar conector completo es la comprobación más clara.
7. Se asoció el código abierto en IDE con el grabado en placa. No son lo mismo. Los mensajes de ejecución y la carga verificada son la evidencia.
8. Un archivo copiado quedó truncado. Se reparó y se comprobó compilación; ese error de llaves era independiente de los fallos de arranque/carga.

## Solución de alimentación prevista

Durante diagnóstico: USB como única entrada de alimentación de cada placa; batería, fuentes externas y VM apagados. Mantener ambas placas alimentadas si sigue unida UART. El rail 3V3 del DevKitC puede alimentar VCC de drivers; no conectar otra fuente activa a ese mismo rail.

Montaje final: batería protegida alimenta dos ramas reguladas, con negativos de reguladores no aislados en distribución común. Rama motores a VM; rama lógica verificada a entradas adecuadas de ambas placas. Dejar al regulador de cada placa generar su 3V3, salvo rediseño explícito. No unir 3V3 S3 con 3V3 DevKitC. No alimentar por USB y fuente externa simultáneamente sin resolver retorno de corriente.

Si la carga total excede el regulador, se debe rediseñar la rama de sensores a una fuente apropiada y verificar niveles. No se propone comprar ni agregar reguladores sin medir ni se autoriza unir salidas de reguladores.

Para sensor con alimentación 5 V, verificar regulación propia, salida D0 o pull-ups I2C antes de conectarlo a GPIO. Un resistor en serie por sí solo no garantiza conversión de 5 a 3,3 V.

## Paso actual

Apagar y comprobar físicamente el multiconector: el cable 3V3 debe ir a VCC de drivers, no VM, GND, EN, GPIO12 o pines de memoria. Verificar la conexión de la MPU y la tensión real de la rama antes de nuevas cargas. No se ha demostrado que el rail compartido sea la causa ni que ninguna placa esté dañada.

## Fuentes

- Toshiba TB6612FNG: https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG
- ESP32 DevKitC V4: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html
- Arranque ESP32: https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html
````

### AGENTS.md

SHA-256 del archivo original

`1265f7b8663fcc03387d4007333517f91941049774e106cbc2657fd77b29d883`

````markdown
# AGENTS.md

## 1. Contexto del proyecto

Este repositorio corresponde al proyecto de la asignatura **Programación Distribuida y Paralela**.

El objetivo general es desarrollar un **robot móvil 4WD con percepción visual, sensores y capacidades de navegación**, usando una arquitectura distribuida basada principalmente en **ROS 2**.

La idea final del proyecto es que el robot pueda interpretar instrucciones de alto nivel como:

> "Ve a la izquierda de las gafas."

y combinar varias capacidades para ejecutarlas:

- detección visual de objetos;
- percepción del entorno;
- localización y/o construcción de mapas;
- navegación;
- control de motores;
- lectura de sensores;
- comunicación entre distintos procesos o nodos.

El proyecto comenzó con una propuesta mucho más simple basada en un ESP32, una cámara y un chasis móvil. Con el avance del diseño se añadieron sensores y componentes orientados a lograr una plataforma robótica más completa.

Este documento sirve como **contexto y guía para agentes de IA que trabajen sobre el repositorio**.  
No debe interpretarse como una especificación rígida ni como una arquitectura inmodificable.

Los archivos sincronizados bajo `sources/` son material de referencia y deben conservarse sin cambios.

---

## 2. Principio general para trabajar en este repositorio

Antes de realizar cambios importantes:

1. Revisar la estructura actual del repositorio.
2. Identificar qué partes ya están implementadas.
3. No asumir que toda la arquitectura descrita aquí ya existe.
4. No introducir dependencias, frameworks o componentes nuevos sin una razón técnica clara.
5. Preferir soluciones simples, modulares y verificables.
6. Mantener separadas las responsabilidades de percepción, navegación, control y hardware.
7. Cuando sea posible, permitir que el mismo software de alto nivel pueda funcionar tanto con simulación como con el robot real.

La arquitectura descrita más adelante es una **referencia deseada**, pero puede cambiar conforme avance el proyecto.

---

## 3. Hardware actual o previsto

### Actualización de montaje — 2026-10-29

- Hay dos placas físicas: ESP32 DevKitC V4 con módulo ESP-32D/WROOM-32D para motores, encoders y MPU6050; ESP32-S3 anterior para cámara y comunicación. Este reparto sustituye las asignaciones antiguas al S3 que aparecen como arquitectura conceptual más abajo.
- Mapa de cableado actual: `MAPA_CONEXIONES.md`. Es diseño parcialmente probado; no asumir alimentación, sensores ni motores ya validados.
- Prueba UART `pruebas_uart/`: en capturas se confirmó DevKitC ejecutando receptor y recibiendo HOLA del S3 de manera repetida (contador al menos 12). Falta evidencia del monitor S3 confirmando RECIBIDO; no declarar validado ida y vuelta, cámara simultánea o ruido con motores.
- Puertos vistos: DevKitC COM5; S3 COM6. Pueden cambiar al reconectar.
- Diagnóstico actual: prueba MPU aún sin éxito; inicialmente no respondió en I2C y, después de cambios de alimentación, hubo capturas con `flash read err, 988` y reinicios/carga interrumpida. El usuario informa multiconector desde 3V3 compartido por driver/MPU. Esto no demuestra conexión incorrecta ni sobrecarga: revisar físicamente VCC frente a VM, continuidad y tensiones antes de reintentar cargas. Auditoría/correcciones en `REVISION_CABLEADO.md`; no cambiar GPIO ni mover sensores a 5 V sin verificar módulo y niveles.
- Prueba `pruebas_encoders/devkitc_encoders/devkitc_encoders.ino`: compilada y cargada por el usuario. Inicialmente falló carga (Wrong boot mode 0x13 / interrupción serial); BOOT sostenido + pulso EN permitió DOWNLOAD_BOOT y luego escritura verificada. El usuario confirmó que la prueba manual funciona; captura con TOTAL FL=80, RL=55, FR=140, RR=60 y NUEVOS=0 en reposo al final. Confirma conteo básico, no calibración, ausencia de ruido con motores, dirección ni distancia. VM/batería sin alimentar en esta fase. Siguiente etapa: prueba MPU6050 antes de energizar motores.
- UART cableada propuesta: S3 TX41→DevKitC RX16, DevKitC TX17→S3 RX42 y GND común. No unir salidas 3V3. Niveles de encoders/IMU y alimentación de motores siguen pendientes de evidencia técnica.

### Computación y control

- **ESP32-S3**
  - Microcontrolador principal del robot.
  - Se encargará principalmente de tareas de bajo nivel.
  - Control de motores.
  - Lectura de sensores.
  - Lectura de encoders.
  - Comunicación con el computador y/o ROS 2.

### Cámara

- **Módulo de cámara OV56** asociado al ESP32-S3. El modelo exacto no está confirmado en este documento.
- Su función principal será proporcionar imágenes para visión artificial.

La inferencia pesada de visión artificial no necesariamente debe ejecutarse en el ESP32.  
Puede ejecutarse en un computador externo y utilizar al ESP32 como nodo/controlador de hardware.

### Chasis

- Chasis móvil **4WD**.
- Cuatro ruedas.
- Cuatro motores DC.
- Movimiento principalmente mediante control diferencial.

Conceptualmente:

```text
lado izquierdo                  lado derecho

Motor FL ─┐                     ┌─ Motor FR
          ├── mismo sentido ────┤
Motor RL ─┘                     └─ Motor RR
```

El giro se realiza modificando la velocidad y/o sentido relativo entre ambos lados.

### Drivers de motores

- **2 × TB6612FNG**, cada uno con dos canales.

Cada driver controla dos motores.

Distribución orientativa:

```text
TB6612FNG #1
 ├── Motor delantero izquierdo
 └── Motor trasero izquierdo

TB6612FNG #2
 ├── Motor delantero derecho
 └── Motor trasero derecho
```

Esta distribución puede cambiar si el diseño físico o eléctrico lo requiere.

### Encoders

- **4 × encoders LM393** asociados a las ruedas/motores.

Objetivos:

- medir movimiento de las ruedas;
- estimar velocidad;
- apoyar la odometría;
- eventualmente implementar control cerrado de velocidad.

### IMU

- **MPU6050**

Objetivos principales:

- aceleración;
- velocidad angular;
- apoyo a estimación de orientación;
- posible fusión con odometría de ruedas.

### Sensor de distancia

- **Sensor D6** proporcionado por el profesor. Verificar sus características e interfaz antes de integrarlo.

Objetivos previstos:

- obtener distancias al entorno;
- detección geométrica de obstáculos;
- alimentar SLAM si sus datos e interfaz lo permiten;
- apoyar navegación autónoma.

### Alimentación

El sistema utiliza baterías y regulación de tensión.

Entre los componentes disponibles se encuentran:

- reguladores DC-DC **LM2596** y **XL4016**;
- baterías **18650** con protección integrada;
- condensadores **cerámicos** y **electrolíticos**;
- interruptor de **balancín**;
- **fusibles y portafusible en línea**;
- distribución de alimentación separando adecuadamente lógica y motores.

No modificar configuraciones eléctricas únicamente basándose en este documento.  
Consultar siempre el esquema eléctrico vigente del proyecto.

---

## 4. Arquitectura conceptual

La arquitectura de referencia es aproximadamente:

```text
                      ┌──────────────────────────────┐
                      │          COMPUTADOR          │
                      │                              │
Cámara ──────────────►│ OpenCV / VLM                │
Instrucción ─────────►│ percepción e interpretación│
                      │                              │
Sensor D6 ───────────►│ ROS 2                       │
                      │ SLAM / localización          │
                      │ Nav2 / navegación            │
                      │ lógica de alto nivel         │
                      │ objetivos validados         │
                      └──────────────┬───────────────┘
                                     │
                                 /cmd_vel
                              comunicación
                                     │
                                     ▼
                      ┌──────────────────────────────┐
                      │          ESP32-S3            │
                      │                              │
                      │ control de motores           │
                      │ lectura de encoders          │
                      │ lectura de IMU               │
                      │ comunicación con ROS 2       │
                      └──────────────┬───────────────┘
                                     │
                         ┌───────────┴───────────┐
                         ▼                       ▼
                    TB6612FNG                TB6612FNG
                         │                       │
                  motores izquierdos      motores derechos
```

Este diagrama representa una **separación conceptual de responsabilidades**.

No significa que todos los procesos deban ejecutarse exactamente en esos dispositivos ni que todos los nodos deban tener esos nombres.

---

## 5. Separación de responsabilidades

La arquitectura ideal intenta dividir el sistema aproximadamente en cuatro capas:

```text
PERCEPCIÓN        LOCALIZACIÓN       DECISIÓN        CONTROL
    │                  │                │               │
 cámara/VLM         SLAM/odom       Nav2/lógica       ESP32
    │                  │                │               │
    └──────────────────┴────────────────┴───────────────┘
                              ROS 2
```

### Percepción

Puede incluir:

- cámara;
- OpenCV;
- VLM para percepción visual e interpretación de instrucciones;
- sensor D6;
- procesamiento de sensores.

Responsabilidad:

> Convertir datos del mundo físico en información útil para el resto del sistema.

---

### Localización y representación del entorno

Puede incluir:

- odometría;
- encoders;
- IMU;
- sensor D6;
- SLAM;
- filtros de estimación de estado.

Responsabilidad:

> Determinar dónde está el robot y cómo es el entorno que lo rodea.

---

### Decisión y navegación

Puede incluir:

- ROS 2;
- Nav2;
- planificador de rutas;
- lógica propia;
- objetivos estructurados y validados a partir de la salida del VLM.

Responsabilidad:

> Transformar objetivos de alto nivel en acciones de navegación.

---

### Control

Principalmente asociado al ESP32.

Responsabilidades:

- recibir comandos de movimiento;
- controlar motores;
- leer encoders;
- leer sensores de bajo nivel;
- ejecutar lazos de control;
- informar estado al sistema superior.

---

## 6. ROS 2

ROS 2 es la capa de integración principal prevista para el sistema.

Siempre que tenga sentido, los componentes deben comunicarse mediante interfaces bien definidas.

Ejemplos orientativos:

```text
/cmd_vel
/odom
/scan
/imu
/camera/image_raw
```

Los nombres concretos pueden variar.

### Idea importante

El software de alto nivel debería depender de interfaces y no directamente del hardware.

Por ejemplo:

```text
Nav2
 │
 ▼
/cmd_vel
 │
 ├── Gazebo / robot simulado
 │
 └── ESP32 / robot real
```

Esto permite reutilizar una mayor parte del sistema entre simulación y hardware real.

---

## 7. Gazebo

Gazebo se utilizará, si resulta útil para el desarrollo, como simulador del robot.

La idea no es escribir "código para Gazebo" y después copiarlo al ESP32.

La estrategia deseada es:

1. Crear un modelo aproximado del robot.
2. Simular sensores y movimiento.
3. Conectar el robot simulado con ROS 2.
4. Desarrollar y probar nodos de alto nivel.
5. Reemplazar posteriormente el hardware simulado por interfaces del robot real.

Ejemplo:

```text
                ROS 2
                  │
          ┌───────┴────────┐
          │                │
       Gazebo          Robot real
          │                │
 sensores simulados    sensores físicos
 motores simulados     ESP32 + motores
```

La simulación debe considerarse una herramienta de desarrollo, no una obligación si introduce una complejidad innecesaria para los objetivos académicos.

---

## 8. Visión artificial

### OpenCV

Puede utilizarse para:

- captura y manipulación de imágenes;
- conversiones de color;
- filtros;
- geometría;
- calibración;
- preprocesamiento;
- procesamiento posterior a una detección.

### VLM

Un **modelo de visión y lenguaje (VLM)** combina la imagen de la cámara con una instrucción para reconocer objetos relevantes e interpretar la intención. Por ejemplo, ante "Ve a la izquierda de las gafas", puede producir una salida estructurada como:

```text
acción: navegar
objeto_referencia: gafas
relación_espacial: izquierda
evidencia_visual: gafas identificadas en la imagen
```

La presencia de un objeto en la imagen no determina por sí sola una posición navegable. Otros componentes deben relacionar la observación con la localización del robot y el entorno, calcular un objetivo y ejecutar la navegación.

---

## 9. Instrucciones y percepción con VLM

Flujo conceptual:

```text
imagen de cámara + "Ve a la izquierda de las gafas"
                         │
                         ▼
                        VLM
             objeto e intención estructurados
                         │
                         ▼
          validación y posición objetivo
             con localización y sensores
                         │
                         ▼
                   Nav2 / lógica
                         │
                         ▼
                      /cmd_vel
                         │
                         ▼
                    ESP32-S3
                         │
                         ▼
                      motores
```

El VLM **no debe controlar directamente los motores**. Una capa intermedia debe validar su salida y traducirla a objetivos seguros y concretos. Si el objeto o la relación espacial no se pueden determinar con suficiente claridad, no se debe iniciar el movimiento basado en esa instrucción.

---

## 10. Programación distribuida y paralela

El proyecto debe aprovechar su naturaleza distribuida.

Ejemplos de procesos que podrían ejecutarse de forma independiente:

```text
Nodo de cámara
Nodo de VLM (visión e instrucciones)
Nodo del sensor D6
Nodo de IMU
Nodo de odometría
Nodo de SLAM
Nodo de navegación
Nodo puente con ESP32
```

No es obligatorio crear un nodo separado para cada función.

La división debe responder a una necesidad real de:

- modularidad;
- procesamiento paralelo;
- comunicación;
- independencia;
- reutilización;
- mantenimiento.

Evitar fragmentar el sistema artificialmente solo para aumentar el número de nodos.

---

## 11. Prioridades del proyecto

Al trabajar sobre el código, priorizar aproximadamente este orden:

```text
1. Robot puede moverse de forma controlada
          ↓
2. Comunicación estable computador ↔ ESP32
          ↓
3. Encoders / odometría
          ↓
4. IMU
          ↓
5. Sensor D6
          ↓
6. Integración ROS 2
          ↓
7. Simulación
          ↓
8. SLAM / localización
          ↓
9. Navegación autónoma
          ↓
10. Percepción visual e interpretación de instrucciones con VLM
          ↓
11. Relación objeto ↔ posición
```

Este orden es una recomendación y puede cambiar según las necesidades de la asignatura.

No intentar implementar todo simultáneamente.

---

## 12. Reglas para agentes de IA

Cuando un agente trabaje sobre este repositorio:

### Debe

- leer antes de modificar;
- explicar cambios arquitectónicos importantes;
- mantener modularidad;
- evitar acoplar visión, navegación y control de motores innecesariamente;
- reutilizar interfaces estándar de ROS 2 cuando tenga sentido;
- documentar supuestos;
- distinguir claramente código experimental de código funcional;
- priorizar implementaciones que puedan probarse por etapas;
- conservar compatibilidad entre simulación y robot real cuando sea razonable.

### No debe

- asumir que todo lo descrito en este archivo ya está implementado;
- reemplazar una arquitectura funcional solo porque exista una alternativa más sofisticada;
- instalar dependencias pesadas sin justificación;
- modificar configuración eléctrica o pines sin revisar el código/esquema vigente;
- inventar características de sensores o componentes;
- integrar el VLM con el movimiento antes de validar las capas básicas de percepción, navegación y control;
- introducir abstracciones complejas innecesarias.

---

## 13. Filosofía del proyecto

La meta no es construir el robot más complejo posible.

La meta es construir un sistema que:

- funcione;
- pueda explicarse;
- pueda demostrarse;
- esté dividido en componentes comprensibles;
- permita observar programación distribuida y paralela;
- pueda ampliarse progresivamente.

Preferir:

```text
simple + funcional + demostrable
```

sobre:

```text
sofisticado + frágil + difícil de explicar
```

---

## 14. Guías del profesor y adaptación al hardware real

Durante el semestre el profesor puede entregar guías, ejemplos, prácticas o procedimientos.

Estas guías deben utilizarse como **referencia funcional y académica**, pero no debe asumirse que el hardware utilizado en ellas coincide con el hardware de este proyecto.

Una guía puede estar diseñada para compañeros que tengan menos hardware, otro hardware o una configuración diferente.

Por tanto, el objetivo no es copiar ciegamente la guía, sino:

> entender qué se pretende lograr y determinar cómo implementarlo correctamente con el hardware que realmente tenemos.

---

### Regla principal

Ante una nueva guía:

```text
Guía del profesor
       │
       ▼
¿Qué funcionalidad o aprendizaje busca?
       │
       ▼
¿Qué hardware tenemos realmente?
       │
       ▼
¿Podemos implementar esa funcionalidad
con nuestro hardware actual?
       │
       ├── Sí → adaptar la implementación
       │        al hardware disponible
       │
       └── No → señalar la limitación
                y NO comprar, sustituir
                ni inventar hardware
                automáticamente
```

---

### Las guías NO redefinen automáticamente nuestro hardware

Si una guía utiliza, por ejemplo:

```text
ESP32
+
cámara
+
motores
```

pero nuestro robot además dispone de:

```text
sensor D6
IMU
encoders
mejores drivers
u otros componentes
```

no se deben ignorar automáticamente esos componentes adicionales.

Se debe evaluar si pueden utilizarse de manera útil para implementar mejor, con mayor precisión o de forma más robusta lo que pide la guía.

Ejemplo conceptual:

```text
La guía pide:
"mover el robot cierta distancia"

Compañeros:
tiempo de activación de motores

Nuestro proyecto:
encoders + odometría
```

Si los encoders permiten cumplir el mismo objetivo de una forma más apropiada para nuestro hardware, puede utilizarse esa solución siempre que no contradiga explícitamente la actividad.

---

### Tampoco se debe usar todo el hardware porque esté disponible

Disponer de un componente no significa que deba utilizarse en todas las prácticas.

Si una actividad puede resolverse de forma simple con:

```text
ESP32 + motores
```

no es obligatorio introducir:

```text
sensor D6 + VLM + SLAM + Nav2
```

solo porque estén contemplados en el proyecto.

Usar únicamente los componentes que aporten valor real a la tarea actual.

---

### Cuando una guía utiliza hardware que NO tenemos

Si una guía menciona un módulo, sensor, placa o componente que no poseemos:

**NO asumir automáticamente que hay que comprarlo.**

**NO buscar automáticamente un sustituto.**

**NO modificar la arquitectura para introducir un equivalente.**

Primero se debe determinar qué papel cumple ese componente dentro de la guía.

Después comprobar si:

1. la función puede realizarse con hardware que ya tenemos;
2. otra parte de nuestra arquitectura ya proporciona una capacidad equivalente;
3. esa sección puede omitirse sin impedir el objetivo principal de la práctica;
4. la actividad depende necesariamente de ese componente.

El resultado debe comunicarse claramente.

Ejemplo:

```text
La guía utiliza: sensor X

Nuestro robot: no tiene sensor X
       │
       ▼
Analizar qué hace sensor X
       │
       ├── La función puede cubrirla nuestro sensor D6
       │       → verificar sus capacidades y adaptar
       │
       ├── Esa parte es opcional o irrelevante
       │       → omitirla
       │
       └── Es indispensable y no tenemos equivalente
               → informar la limitación
                  y esperar decisión del usuario
```

---

### Prohibición de adquirir o sustituir hardware por iniciativa del agente

Un agente de IA que trabaje en este repositorio **no debe decidir que hace falta comprar hardware nuevo**.

Tampoco debe asumir que el usuario desea:

- comprar un módulo mencionado en una guía;
- reemplazarlo por otro componente;
- buscar un equivalente comercial;
- rediseñar el robot para adaptarlo a ese módulo;
- cambiar componentes ya comprados.

Eso solamente debe hacerse cuando **el usuario lo decida explícitamente**.

Hasta entonces, trabajar con:

```text
hardware disponible
+
software disponible
+
objetivo funcional de la guía
```

---

### Tratamiento recomendado de cada guía nueva

Cuando se reciba una guía nueva:

1. Identificar el objetivo de la práctica.
2. Separar los requisitos funcionales de los componentes concretos utilizados en el ejemplo.
3. Comparar esos requisitos con nuestro hardware actual.
4. Clasificar cada parte como:

```text
A. Aplicable directamente
B. Aplicable con adaptación
C. Ya cubierta por hardware mejor o diferente
D. No aplicable por falta de hardware
E. Prescindible para nuestro caso
```

5. Implementar A, B y C cuando corresponda.
6. Omitir D o E cuando sea razonable.
7. Si una parte D es indispensable para cumplir el objetivo, informarlo antes de proponer compras o sustituciones.
8. No buscar sustitutos de hardware salvo petición expresa del usuario.
9. Mantener el resto del robot funcional y evitar romper capacidades existentes por seguir literalmente una guía.

---

### Prioridad práctica

Cuando una guía y nuestra configuración difieran, pensar en este orden:

```text
1. ¿Qué quiere enseñar o conseguir la guía?
                 ↓
2. ¿Qué necesita realmente la actividad?
                 ↓
3. ¿Qué hardware tenemos?
                 ↓
4. ¿Cómo logramos el mismo objetivo con ese hardware?
                 ↓
5. ¿Hay partes de la guía que no aplican a nuestro robot?
                 ↓
6. Omitirlas o marcarlas como no aplicables.
```

La prioridad es cumplir correctamente el objetivo académico **con nuestro robot**, no reproducir exactamente el montaje de otro grupo.

---

### Hardware conocido actualmente

Mantener como referencia el inventario conocido del proyecto:

```text
ESP32-S3 con módulo de cámara OV56
chasis 4WD
ruedas
4 motores DC
4 encoders LM393
2 × TB6612FNG de dos canales
MPU6050
sensor D6 proporcionado por el profesor
LM2596
XL4016
condensadores cerámicos y electrolíticos
interruptor de balancín
baterías 18650 con protección integrada
fusibles y portafusible en línea
```

Este inventario puede cambiar durante el proyecto.

Cuando cambie, actualizar esta sección o la sección de hardware principal del documento.

## 15. Estado cambiante del proyecto

Este archivo describe el contexto acumulado, el hardware conocido y la dirección general del proyecto, pero **no es una fuente definitiva para cada detalle técnico**.

Debe conservarse como memoria del proyecto para evitar perder decisiones previas.

Las guías nuevas deben interpretarse junto con este contexto, no reemplazarlo automáticamente.

Ante discrepancias, considerar en conjunto:

```text
objetivo de la actividad
+
hardware realmente disponible
+
estado actual del código
+
decisiones recientes del equipo
```

Si el repositorio, el hardware o las decisiones del proyecto evolucionan significativamente, actualizar este documento.

---

## 16. Resumen rápido para un agente nuevo

Si acabas de entrar al repositorio, entiende esto primero:

> Estamos construyendo un robot móvil 4WD para Programación Distribuida y Paralela. El ESP32-S3 controla el hardware de bajo nivel. El robot tiene un módulo de cámara OV56, cuatro motores con encoders, dos TB6612FNG, MPU6050, sensor D6 y los componentes de alimentación indicados en el inventario. Un computador puede ejecutar ROS 2, percepción con VLM, localización y navegación. El VLM combina la imagen con instrucciones como "ve a la izquierda de las gafas" y entrega información para calcular un objetivo validado; Nav2 o la lógica de navegación y el ESP32-S3 se encargan del movimiento. Gazebo puede utilizarse para simular el robot. Esta arquitectura es una guía: revisar siempre el estado actual del repositorio antes de realizar cambios.
````

## Anexo C — Texto pegado histórico recuperado

Adjunto .codex/attachments/0c805285-2b89-49ab-914d-8535571c6caf/Texto pegado.txt, fase de calibración. Incluye DIAMETRO_CM=0 y los conteos antiguos. No es el firmware final de 30 cm.

SHA-256 del archivo original

`d415b1bbf3fa4c9168021a09c559f0bba3c87219d913bfbca8e1c884e4208740`

```cpp
#include <Arduino.h>
#include <atomic>
#include <string.h>

// Completar con las mediciones que ya hicieron.
constexpr float DIAMETRO_CM = 0.0f;  // Reemplaza 0.0 por el diámetro real en cm.

constexpr float PULSOS_CINCO_VUELTAS[] = {
  148.0f,  // FL: delantera izquierda
  36.0f,  // RL: trasera izquierda
  130.0f,  // FR: delantera derecha
  202.0f   // RR: trasera derecha
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
```

## Cierre del registro

La continuación inmediata sigue siendo el pinout real del LiDAR y su primera lectura en PC. El adaptador es COM9 en la última captura. El usuario pidió esta memoria antes de completar esa conexión. Actualizarla después de cada decisión o prueba que cambie ese estado.

## Actualización 2026-11-14 — LiDAR en firmware integral

La captura del usuario confirmó cámara a 10,7 FPS, UART/MPU/operador OK, LiDAR con 483267 bytes, 25 paquetes, 5985 errores de checksum, cero errores de cabecera, tres inicios y dos barridos sin cobertura válida. Se contrastó con el SDK original m1ct_d4_ros2_v16.zip del fabricante. El código inicial siguió un diagrama del manual que agrupa las muestras de otra manera. Se corrigió LidarD6.h para aplicar Si_L XOR palabraLE(Si_2nd,Si_H). La página identifica esta versión como DIAGNOSTICO D6 V3 SDK. Pruebas C++ reales pasan con un vector fijo independiente del generador y rechazo del checksum anterior. Pendiente confirmación física de V3 tras recargar solo el S3. Mantener bloqueo por LiDAR no válido. No cambiar pines ni eliminar checksum/condiciones de movimiento.


## Actualización 2026-11-14 — respaldo y navegación continua V1

El usuario confirmó que el LiDAR V3 detecta. Capturas: 31000+ paquetes, cero errores, 67–68/72 sectores, cámara/UART/MPU OK; mínimos de 54–71 mm mantenían bloqueado AVANZA 50. Solicitó respaldo de los códigos que funcionaron y nueva marcha continua con evasión y botones ARRANCAR/STOP.

Se respaldaron los ocho archivos de prueba_integral_50cm en primera_prueba_exitosa_2026-11-14, comparados individualmente por SHA256. Se mantuvo intacto también el paquete original. El nuevo desarrollo está separado en navegacion_autonoma. Ver su LEEME.md para carga y comportamiento. Protocolo nuevo V1/A1: actualizar ambos ESP32, no solo S3. Ninguna carga automática realizada.

Usuario confirma LiDAR centrado en el chasis y 15 cm hasta cada punta. Radio propuesto 150 mm. Falta ángulo frontal real: se confirma en el formulario del radar, no se supone cero. Configuración solo en RAM y arranque siempre detenido. Marcha continua PWM80; pausa y giro izquierdo FL/RL atrás, FR/RR adelante; regresa al avance tras tres barridos con trayectoria libre. Sin límite de centímetros ni duración del avance. Protección independiente DevKit de 450 ms sin órdenes, MPU y encoders; sesión y corrida enclavadas para impedir rearme por mensajes tardíos. STOP cancela corrida. Fallos de sensor, enlace o espacio siguen deteniendo: no prometer movimiento perpetuo sin posibilidad de choque.

Planificación en 36 celdas de 10 grados a partir de los 72 sectores originales: distancia menor real por par, sin interpolar ni eliminar cercanos. Desconocidos impiden la maniobra correspondiente. Radio más margen inicial de 100 mm y anticipación de 500 mm. Giro de más de 6,5 segundos sin salida produce fallo. Es evasión reactiva con preferencia fija por la izquierda, sin planificación global ni dirección absoluta por IMU. La distancia de ruedas incluye giros, no posición.

Compilación Arduino y pruebas C++ de lector/protocolo, navegación y sketch DevKit con dobles, más interfaz JavaScript. Aún pendiente prueba física de navegación, sentidos de giro y distancias de frenado. No afirmar que el vehículo ya evadió obstáculos con esta versión.


## 2026-11-24 — reanudación autónoma V1.1

Usuario cargando/probando firmware autónomo; informa parada al corregir trayectoria. /estado no accesible (timeout), aún falta motivo real del panel. Revisado código: salida de giro mandaba cero un ciclo y obstrucción temporal o celda ausente enclavaba FALLO. Cambiado solo S3: estado ESPERANDO_ESPACIO mantiene corrida activa, reevalúa y reanuda tras tres barridos distintos con el mismo tipo de salida. Salida de giro manda avance80 directamente; DevKit conserva pausa local de inversión. STOP y fallos reales siguen enclavados, sin rearme automático. No reducir distancias ni ocultar ecos. Radio150mm; verificar configuración aplicada pues captura anterior decía radio500 aunque formulario150. Respaldo completo de 15 archivos en respaldo_autonomia_antes_reanudacion_2026-11-24 con SHA256. Prueba inicial y ZIP V1 preservados. Corrección requiere recargar solo S3 con todos sus encabezados. No declarar causa física resuelta hasta captura y prueba.


Ajuste posterior 24-nov: usuario pide anticipacion15cm; ANTICIPACION_MM=150 en S3 V1.1. No reducir el limite global del chasis a150 (radio150+margen100 sigue250). Giro cuatro ruedas FL/RL -80 FR/RR +80, salida cuatro+80. Captura aportada muestra Sin conexion al S3 y frente escrito307/aplicado90; diagnostico viejo no prueba sensores actuales. Interfaz nueva limpia datos congelados en errores de consulta. No afirmar aprendizaje RL ni ausencia de pausas de inversion.


## 24-nov-2026 — éxito informado, respaldo y versión V1.2

Usuario confirma éxito de la pruebaV1.1 con objetos cercanos, pero reporta solo giro izquierdo, corrección insatisfactoria a distancias mayores y paradas que exigen ARRANCAR. Solicitó respaldar códigos y entregar versión completa con pruebas de casos base. Copiados15 archivos previos a prueba_autonoma_exitosa_2026-11-24, verificados individualmente porSHA256; preservar. Diagnóstico de código: elección fija izquierda, corte de giro6,5s y cancelación por cualquier barrido inválido o caída de video. Sin mensaje actual de parada no atribuirles toda la causa física.

V1.2 actual en navegacion_autonoma elige espacioizquierda/derecha, conserva dirección sin zigzag y tras6,5s prueba otro lado sin cancelar corrida. Falta temporal deLiDAR pasa aESPERANDO_DATOS con motorescero, reanuda al validar tres barridos nuevos de la misma salida. Espacio/desconocidos hacenespera, nunca se inventa distancia libre. Reenvío de inicioD6 cada4s sin barrido válido. Video deja de ser condición de movimiento; operador/navegador sí mantiene watchdog. Fallos UART/MPU/encoder permanecen enclavados porseguridad; no se generan nuevas corridas para eludirlos. DevKit ahora reporta cuál encoderfalló y S3 conserva Última parada. Sentido angular delLiDAR debe confirmarse físicamente en la página antes de seleccionar lado. Se mantiene radio150mm margen100 anticipación150 adicional. NoRL.

Pruebas de protocolo, planificación,32 combinaciones de salud,28 casos de frente/sentido/elección,2 escenasvariables,4 encoders, interfaz y canal planificador→CRC→DevKitconGPIOsimulados. CompilaciónArduino de ambasplacas. No se cargaron ni activaron motores automáticamente. Cargar ambas para diagnósticocompleto. ValidaciónV1.2 en suelo pendiente; éxito informado corresponde al respaldoV1.1.

