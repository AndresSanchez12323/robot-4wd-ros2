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
