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
