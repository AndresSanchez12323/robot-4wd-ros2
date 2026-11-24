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
