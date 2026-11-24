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
