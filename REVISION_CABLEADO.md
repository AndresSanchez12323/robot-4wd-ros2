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
