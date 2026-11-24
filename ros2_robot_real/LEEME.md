# ROS 2 en el robot real

Esta integración sigue las páginas 9–12 de la guía del 4WD. El computador ejecuta ROS 2 Humble y Qwen3-VL 2B Instruct. El firmware ROS2-1 se entrega por separado de la versión 1.2 del carro y del ZIP 1.4 aportado por el compañero.

## Arrancar en este PC

Los pesos de Qwen se descargaron en `%LOCALAPPDATA%/PDyP/vision`. El archivo Compose incluye la ruta predeterminada. Si se copian a otra máquina, colocar allí los dos GGUF indicados en `compose.yaml` o definir `MODELOS_DIR`. El servidor local usa CPU y los contenedores se comunican por la red privada de Compose.

Ejecutar desde la raíz del proyecto en PowerShell:

```powershell
docker compose -f ros2_robot_real/compose.yaml up -d
docker compose -f ros2_robot_real/compose.yaml ps
```

Con el S3 encendido y los motores apagados, el PC necesita una ruta a `http://192.168.4.1/captura`. RobotCam es la red local del S3. Para conservar Internet mientras se usa RobotCam, conectar primero otra interfaz con Internet, por ejemplo Ethernet o anclaje USB de un teléfono. El nodo espera imágenes de esa dirección. Puede usarse otra dirección con `ROBOT_URL` si el S3 se configuró para una red diferente.

Comprobar en ROS 2 que se recibe un cuadro. El comando muestra el formato del primer cuadro:

```powershell
docker compose -f ros2_robot_real/compose.yaml exec -T ros /ros_entrypoint.sh ros2 topic echo --once --field format --qos-reliability best_effort /camera/image_raw/compressed
```

## Primera revisión visual

Poner un objeto identificable delante de la cámara y usar dos terminales. En la primera se observa el resultado:

```powershell
docker compose -f ros2_robot_real/compose.yaml exec -T ros /ros_entrypoint.sh ros2 topic echo /robot/vision/revision
```

En la segunda se publica una instrucción:

```powershell
docker compose -f ros2_robot_real/compose.yaml exec -T ros /ros_entrypoint.sh ros2 topic pub --once /robot/vision/instruccion std_msgs/msg/String "{data: 'Ve a la izquierda de la botella'}"
```

El resultado incluye dos identificadores de captura, la interpretación inicial, la revisada y si cambiaron objeto, relación o visibilidad. Si pierde de vista el objeto, debe informar `sin_evidencia`. La visión publica observaciones. Una futura etapa de metas visuales requiere calibrar la cámara, asociar la observación con el mapa y validar la trayectoria.

## Firmware de movimiento ROS2-1

Para el primer uso en placas con versión 1.2, cargar por USB los dos sketches en `firmware/` con los motores sin alimentación. En el carro actual ya se verificó ROS2-1 y Arduino descubrió los dos puertos OTA. Los sketches usan los pines del montaje existente y conservan el protocolo V1/A1. La extensión O1 del DevKit informa los cuatro contadores de rueda con signo inferido del sentido aplicado, el reloj local y el giroscopio de tres ejes.

| Placa | Sketch | Ajustes de Arduino IDE |
|---|---|---|
| ESP32 DevKitC | `firmware/devkit_integral/devkit_integral.ino` | ESP32 Dev Module, Flash Mode DIO, partición Default con dos slots OTA |
| ESP32-S3 | `firmware/s3_integral/s3_integral.ino` | ESP32S3 Dev Module, Flash 16 MB, PSRAM OPI, APP 3 MB/FATFS 9 MB, USB CDC On Boot Disabled |

Con las dos placas actualizadas, mantener el PC en RobotCam. Arduino IDE podrá descubrir `robot-s3` y `robot-devkit` por mDNS. El DevKit se une a la red que crea el S3. La contraseña de la carga es la configurada en ambos sketches. Las dos particiones indicadas disponen de slots alternos. Detener la marcha antes de iniciar OTA. Las placas atienden OTA solo con la marcha desarmada. Falta validar el descubrimiento y una carga OTA reales después del primer flasheo.

### Control desde ROS 2

1. Calibrar la dirección frontal con el carro quieto y confirmar el ángulo en el radar. `sentido_lidar=1` indica que el ángulo crece hacia la derecha física y `-1` que crece hacia la izquierda. El radio inicial de 150 mm corresponde a la medida del chasis. Ajustar la geometría por ROS mediante estos comandos, reemplazando ANGULO y SENTIDO por los valores observados:

    ```powershell
    docker compose -f ros2_robot_real/compose.yaml exec -T ros /ros_entrypoint.sh ros2 param set /control_robot frente_grados ANGULO
    docker compose -f ros2_robot_real/compose.yaml exec -T ros /ros_entrypoint.sh ros2 param set /control_robot sentido_lidar SENTIDO
    docker compose -f ros2_robot_real/compose.yaml exec -T ros /ros_entrypoint.sh ros2 service call /robot/configurar std_srvs/srv/Trigger "{}"
    ```

2. Confirmar `/robot/estado`, `/scan`, `/imu/data_raw` y `/robot/encoders`. Los sectores sin retorno se publican como NaN. Para activar `/odom`, establecer `trocha_m` con la separación efectiva de los lados calibrada en piso. `/odom` estima el movimiento con ruedas de un canal y aún necesita validación frente a deslizamiento y giros medidos. La posición de los sensores y sus TF para Nav2 se calibran en una etapa posterior.
3. Con las ruedas elevadas en la primera prueba de movimiento, ejecutar `/robot/arrancar` y `/robot/parar`. Ambos servicios son `std_srvs/srv/Trigger`. Confirmar el estado posterior en `/robot/estado`. La lectura de telemetría es pasiva. El nodo manda latidos solo después de un ARRANCAR explícito. El permiso local dura 900 ms y la pérdida de enlace desarma la corrida. El DevKit mantiene además su vigilancia UART, de IMU y de encoders. Verificar STOP y pérdida del enlace antes de la primera corrida en el suelo.
4. Observar `/robot/aprendizaje/giros`. El sistema guarda éxitos y fallos en un volumen Docker. Solo usa el aprendizaje para desempatar giros con espacio medido equivalente. Para registrar un resultado exige pulsos firmados de ambos lados en las direcciones del giro. Un éxito requiere que el giro conduzca de nuevo al avance. STOP, pérdida de datos y falta de mediciones no son muestras de entrenamiento. Los márgenes y las comprobaciones del LiDAR se aplican en el S3.

El observador busca un objeto fijo al iniciar y vuelve a buscar cada minuto cuando está libre. Si dos capturas nuevas coinciden en la etiqueta y la visibilidad, actualiza `/datos/objetos.json` y publica el registro en `/robot/aprendizaje/objetos`. La memoria guarda etiquetas y evidencia visual. Las órdenes de movimiento siguen pasando por el supervisor del S3.

Con el firmware que publica `puedeArmar`, una orden `/robot/arrancar` puede quedar en espera con los motores a cero aun cuando `permiso` sea cero. Para salir de esa espera el S3 exige tres barridos nuevos que confirmen una maniobra. La validez del barrido depende de su integridad, cantidad de puntos y antigüedad. El planificador verifica por separado que estén medidos los sectores necesarios para la maniobra. `/robot/parar` cancela la espera o la marcha. En el firmware anterior, el puente sigue usando `permiso` para aceptar el arranque.

El código del S3 incluye un retroceso de recuperación de hasta 350 ms, una vez por corrida, si un giro se agota, el frente sigue bloqueado y tres barridos nuevos confirman espacio trasero y cobertura alrededor del carro. Faltas de cobertura, obstáculos y STOP interrumpen la maniobra. El LiDAR horizontal requiere una superficie nivelada para la primera prueba física de retroceso.

El DevKit con la extensión E2 calcula distancia y vigila los encoders con flancos separados al menos 2 ms y registrados mientras la rueda tiene orden de potencia. Los cuatro valores A1 previos a la aceleración siguen mostrando los contadores brutos para diagnosticar señales en reposo. La extensión O1 entrega contadores firmados filtrados. Si se usa un DevKit antiguo, el puente ROS calcula el recorrido desde los contadores firmados y conserva la distancia del S3 en `distancia_s3_cm` para diagnóstico.

Esta etapa ejecuta evasión reactiva comandada por ROS 2 y adaptación por experiencia local. La finalización de una vuelta, la navegación a metas y el entrenamiento de una política global requieren todavía mapa, localización y validación física.

### Pruebas

```powershell
docker compose -f ros2_robot_real/compose.yaml run --rm --no-deps -T -e ROS_DOMAIN_ID=43 -e PYTHONPATH=/proyecto ros python3 pruebas/control_integracion.py
```

`pruebas/guardia_ros.cpp` se ejecuta con g++ C++17. Comprueba caducidad y rechazo de órdenes duplicadas, con sesión incorrecta o fuera de tiempo. También comprueba que una preferencia aprendida solo desempate. La prueba ROS usa un S3 HTTP simulado para verificar sensores, servicio de marcha, STOP, corte de red, odometría y persistencia. Ninguna de estas pruebas envía órdenes al carro físico.

## Evidencia del 24 de noviembre de 2026

- ROS 2 Humble y el servidor del modelo arrancaron. El tópico de cámara y los tópicos de instrucción y revisión quedaron registrados.
- Prueba de integración con Qwen3-VL real y dos imágenes JPEG de banco. La primera mostraba un círculo rojo y la segunda un fondo sin el círculo. Detectó el objeto, corrigió su visibilidad y publicó `sin_evidencia` en ROS 2.
- Prueba con cámara física por Wi-Fi `RobotCam`, con Internet conservado por anclaje USB del celular. ROS 2 recibió JPEG del S3 y Qwen revisó dos capturas reales. La primera respuesta marcó una botella como visible, aunque su descripción decía que no podía identificarla. Con la segunda captura corrigió el indicador y publicó `sin_evidencia`.
- El ZIP del compañero anuncia versión 1.4 y contiene ArduinoOTA en las dos placas. Se adoptó OTA en ROS2-1. En 1.4 se descartan retornos cercanos y no hay comprobación trasera específica antes de retroceder. El planificador de ROS2-1 conserva comprobaciones de cobertura y margen del firmware 1.2. La evidencia de compilación del ZIP está rotulada V1.2 y no acredita la variante 1.4.
- Compilación ROS2-1 verificada con Arduino-ESP32 3.3.12. DevKit: 1 008 530 bytes, 76 % del slot. S3: 1 093 291 bytes, 34 % del slot. Pasaron las pruebas de guardia en C++ y de integración ROS 2 contra el S3 simulado. El archivo de entrega es `ROBOT_4WD_ROS2_1.zip`.
- Corrección E2: compilación DevKit de 1 008 686 bytes, 76 % del slot. Pruebas del filtro y del aprendizaje con evidencia de encoders completadas. Se activó la corrección de respaldo en ROS y se verificó estabilidad del recorrido con el carro real en reposo. La nueva extensión E2 sigue pendiente de carga en el DevKit. El paquete actualizado es `ROBOT_4WD_ROS2_1_E2.zip`.
