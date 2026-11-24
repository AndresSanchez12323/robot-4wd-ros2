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
