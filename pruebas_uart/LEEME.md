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
