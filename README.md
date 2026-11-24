# Robot móvil 4WD con ROS 2

Proyecto de Programación Distribuida y Paralela. Un ESP32-S3 gestiona la cámara OV5640 y el LiDAR COIN-D6. Un ESP32 DevKitC controla los motores y lee los encoders y la IMU. ROS 2 conecta esos datos con la navegación y el modelo visual Qwen3-VL.

- [Integración ROS 2 y firmware actual](ros2_robot_real/LEEME.md)
- [Mapa de conexiones](MAPA_CONEXIONES.md) y [revisión de cableado](REVISION_CABLEADO.md)
- [Navegación autónoma](navegacion_autonoma/LEEME.md) y [prueba de 50 cm](prueba_integral_50cm/LEEME.md)
- [Memoria técnica](MEMORIA_SUPREMA.md) e [historial del proyecto](AGENTS.md)

El repositorio incluye pruebas, diagramas, guías y respaldos históricos. En esta copia, CAMBIA_CLAVE_ROBOTCAM es un ejemplo. Configura la misma clave privada en ambos sketches y en OTA antes de compilar para tu robot. Conserva la clave privada fuera de Git. Los manifiestos de los respaldos verifican las copias publicadas.

Los registros del PC, los binarios generados localmente y el volumen Docker de aprendizaje quedan en el equipo de trabajo.
