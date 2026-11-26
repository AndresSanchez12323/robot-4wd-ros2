# Robot 4WD — versión 1.6

Aquí queda solo el firmware de navegación autónoma 1.6. Las copias anteriores no están en esta rama.

Abrir en Arduino IDE, con todas las pestañas de cada carpeta:

| Placa | Sketch |
|---|---|
| ESP32-S3, cámara y LiDAR | `navegacion_autonoma/s3_integral/s3_integral.ino` |
| ESP32 DevKitC, motores | `navegacion_autonoma/devkit_integral/devkit_integral.ino` |

Después de cargar el S3, la página `http://192.168.4.1/` debe decir **Version 1.6**. La carga, la geometría y las paradas están en `navegacion_autonoma/LEEME.md`.

El árbol completo anterior sigue en el commit `1b12bde`. Para recuperarlo, revertir el merge de la rama `solo-v1.6`.

La misión con LLM, VLM, VLA y GPU está en `servicios/`. La entrada es el gateway GraphQL; el carro ejecuta las órdenes y, si un servicio no responde, no se mueve. Los pasos están en `servicios/LEEME.md`.
