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
