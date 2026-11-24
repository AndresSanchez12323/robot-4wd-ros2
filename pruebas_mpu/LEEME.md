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
