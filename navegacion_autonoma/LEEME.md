# Navegación autónoma V1.6 — gira hasta encontrar salida

24 de noviembre de 2026. La página debe decir Version 1.6. El carro que respondió en Lamborgini estaba en ROS2-1: esa página no arranca la marcha. La primera carga de esta versión es por USB. Las siguientes salen por Wi-Fi.

## Qué cambió

- El carro solo frena por un eco real en el corredor delantero. Un sector vacío detrás o a un lado no lo deja quieto.
- El lado sin ecos cuenta como espacio abierto. Gira hacia el lado con más espacio, con las cuatro ruedas.
- Si el obstáculo delantero está a menos de radio+80 mm, retrocede medio segundo y después pivota.
- Si hay un obstáculo, gira hacia el lado libre y no corta el pivote: sigue girando hasta que dos barridos vean el frente libre. Si queda a menos de radio+80 mm después de haber girado, retrocede y continúa el mismo giro. La misma orden ARRANCAR sigue viva.
- Dos barridos con el frente libre devuelven la recta. Un barrido malo en medio del giro no pone las velocidades en cero.
- Ecos a menos de 80 mm se ignoran: son el soporte y los cables del LiDAR.
- Avance y giro usan PWM 140 de 255. El retroceso usa 120. El DevKit acepta hasta ±140. Hay que cargar las dos placas: si el DevKit sigue en el tope de 80, ignora la orden y el carro no se mueve.
- El S3 acepta `POST /mision?accion=` con explorar, avanzar, girar_izquierda, girar_derecha o parar. Explorar es el esquive de siempre. Avanzar se acerca y, a menos de radio+80 mm, para al lado del objetivo en vez de esquivarlo. Girar no esquiva solo. Esta orden la manda el orquestador de `servicios/`; el DevKit no cambia.
- La primera vez que una rueda no da pulsos en 1,5 s se reintenta. La segunda vez sí cancela la marcha.
- STOP, la caída de UART y la MPU sin datos siguen cancelando.

## Cargar los códigos completos

En la copia local, cada sketch lee la clave privada desde su archivo `RedLocal.h`. Al clonar el repositorio en otro equipo, crea ese archivo dentro de cada carpeta del sketch con la misma definición `constexpr char CLAVE_RED[] = "tu-clave-wifi";`. Git ignora esos archivos.

Abrir en Arduino IDE desde ESTA carpeta, sin pegar fragmentos:

| Placa | Archivo | Pestañas necesarias |
|---|---|---|
| DevKitC | devkit_integral/devkit_integral.ino | EnlaceRobot.h, GuardiaMarcha.h |
| S3 con cámara | s3_integral/s3_integral.ino | EnlaceRobot.h, EstadoIntegral.h, LidarD6.h, Navegacion.h, SupervisorMarcha.h |

Para recibir también el diagnóstico específico de ruedas cargar AMBAS placas. Protocolo V1/A1 permanece compatible con el DevKit autónomo anterior; no mezclar con la antigua prueba de50cm.

Cuando la página ya dice Version 1.6, el computador se une a Lamborgini y Arduino IDE muestra los puertos de red `robot-s3` y `robot-devkit`. La clave de la carga es la de Lamborgini. El carro queda quieto: al empezar la carga se apagan los motores y la placa se reinicia. El DevKit solo aparece si el S3 ya creó la red. OTA se atiende también durante la marcha; al iniciar la carga se envía STOP.

DevKit: ESP32 Dev Module, Flash Mode DIO. S3: ESP32S3 Dev Module; Flash16MB; PSRAM OPI; partición APP3MB/FATFS9MB; USB CDC On Boot Disabled para el USB-UART usado. Paquete compilado con Arduino-ESP32 instalado. Programar con motores sin alimentación y manteniendo el aislamiento USB/alimentación externa adoptado.

No cambia cableado: D6 amarilloTX→S3GPIO1RX, verdeRX→S3GPIO2TX, rojo5V y negroGND; UART230400. S3TX41→DevKitRX16, DevKitTX17→S3RX42, GND común; UART115200. MPU VCC3V3, GND, SDA21, SCL22 en DevKit. XL4016 a5V según decisión previa; GPIO nunca a5V.

## Configuración de dirección física

Wi-Fi Lamborgini, página http://192.168.4.1/. La página debe decir Version 1.6. Mantener la página abierta. Sin internet es normal en esa red; perder Lamborgini no es normal para controlar.

1. Con el robot parado, confirmar frente y radio150mm. Centrar un objeto delante y seleccionar su ángulo en el radar; no convertir15cm en grados. Comparar la línea de valores aplicados con los campos escritos.
2. Nuevo selector obligatorio: sentido angular del LiDAR. Mover el objeto del frente un poco hacia la derecha física del carro y observar si su ángulo aumenta o disminuye. Elegir crece hacia derecha si aumenta y crece hacia izquierda si disminuye. En el cruce359→0 aumenta; 0→359 disminuye. No asumir un sentido sin comprobarlo. El dibujo del radar por sí solo no demuestra la orientación física del sensor.
3. Pulsar Confirmar frente y radio y comprobar que la línea aplicada incluye el sentido elegido. La configuración permanece en RAM y debe confirmarse al reiniciarS3. Cambiarla cancela la corrida para impedir modificar geometría en marcha.
4. Retirar el objeto de referencia y comprobar LiDAR, UART, MPU y operador. La cámara puede estar pendiente sin cancelar el permiso de navegación.
5. Para validar los dos sentidos, primero comprobarlos con ruedas elevadas. Luego en suelo libre ensayar un obstáculo delante con más espacio a derecha y después con más espacio a izquierda. Mantener STOP y el interruptor accesibles. Registrar capturas con motivo y Última parada si algo falla.

## Distancias y expectativas

Anticipación 150 mm, radio 150 mm y margen 100 mm en el corredor delantero. Un eco real dentro de ese corredor hace esquivar. Por debajo de radio+80 mm (230 mm con el radio actual) primero retrocede. Los ecos de menos de 80 mm se descartan como cuerpo del sensor.

Se usan 36 celdas de 10 grados. Una celda sin retorno, fuera del corredor delantero, cuenta como espacio abierto. El giro no exige las 36 celdas llenas. El lector D6 sigue pidiendo un barrido válido (cobertura, checksum y edad); sin barrido válido los motores quedan en cero y la marcha se reanuda sola al volver los datos.

## Casos de parada y recuperación

| Caso | Motores | ¿Conserva la corrida? | Recuperación |
|---|---|---|---|
| Obstáculo frontal, con sitio para girar | Pausa y pivote al lado libre | Sí | Recta tras dos barridos con frente libre |
| Obstáculo delantero a menos de radio+80 mm | Retroceso y luego pivote | Sí | Sigue en la misma corrida |
| Obstáculo que sigue al frente | Sigue girando, sin poner las ruedas en cero | Sí | Recta en cuanto dos barridos vean el frente libre |
| Hueco del LiDAR detrás o al lado | Sigue la maniobra | Sí | No se detiene por ese hueco |
| Barrido inválido o LiDAR caído | Cero | Sí | Dos barridos válidos con frente libre |
| Video interrumpido | Navegación permitida con LiDAR sano | Sí | Reconectar video |
| STOP en página o USB | Cero | No | ARRANCAR explícito |
| Cambio de frente/radio/sentido | Cero | No | Configurar y ARRANCAR |
| Navegador ausente 1,2 s | Cero | No | Reconectar y ARRANCAR |
| Sin órdenes UART 450 ms en DevKit | Cero | No | Resolver enlace y ARRANCAR |
| Sin telemetría DevKit 600 ms en S3 | Cero | No | Resolver enlace y ARRANCAR |
| MPU sin datos | Cero | No | Resolver cableado y ARRANCAR |
| Rueda sin pulsos, primera vez en 1,5 s | Sigue, reintento | Sí | Si se repite, cancela y pide ARRANCAR |
| Reinicio de una placa | Cero | No | Sincronizar, configurar y ARRANCAR |

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
