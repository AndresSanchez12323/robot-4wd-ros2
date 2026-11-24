# AGENTS.md

## 1. Contexto del proyecto

Este repositorio corresponde al proyecto de la asignatura **Programación Distribuida y Paralela**.

El objetivo general es desarrollar un **robot móvil 4WD con percepción visual, sensores y capacidades de navegación**, usando una arquitectura distribuida basada principalmente en **ROS 2**.

La idea final del proyecto es que el robot pueda interpretar instrucciones de alto nivel como:

> "Ve a la izquierda de las gafas."

y combinar varias capacidades para ejecutarlas:

- detección visual de objetos;
- percepción del entorno;
- localización y/o construcción de mapas;
- navegación;
- control de motores;
- lectura de sensores;
- comunicación entre distintos procesos o nodos.

El proyecto comenzó con una propuesta mucho más simple basada en un ESP32, una cámara y un chasis móvil. Con el avance del diseño se añadieron sensores y componentes orientados a lograr una plataforma robótica más completa.

Este documento sirve como **contexto y guía para agentes de IA que trabajen sobre el repositorio**.  
No debe interpretarse como una especificación rígida ni como una arquitectura inmodificable.

Los archivos sincronizados bajo `sources/` son material de referencia y deben conservarse sin cambios.

---

## 2. Principio general para trabajar en este repositorio

Antes de realizar cambios importantes:

1. Revisar la estructura actual del repositorio.
2. Identificar qué partes ya están implementadas.
3. No asumir que toda la arquitectura descrita aquí ya existe.
4. No introducir dependencias, frameworks o componentes nuevos sin una razón técnica clara.
5. Preferir soluciones simples, modulares y verificables.
6. Mantener separadas las responsabilidades de percepción, navegación, control y hardware.
7. Cuando sea posible, permitir que el mismo software de alto nivel pueda funcionar tanto con simulación como con el robot real.

La arquitectura descrita más adelante es una **referencia deseada**, pero puede cambiar conforme avance el proyecto.

---

## 3. Hardware actual o previsto

### ROS 2 y revisión visual de banco — 2026-11-24

- Actualización OTA del ESP32-S3 realizada desde el PC mediante el puerto de red de Arduino en RobotCam. Carga completada al 100 %, reinicio confirmado con nueva sesión y calibración restaurada por ROS. La revisión instalada reportó LiDAR válido con 50 sectores y explicitó «Falta medida en la zona de avance». El carro quedó desarmado, con motores a cero.

- El S3 que cargó el usuario se armó por ROS y esperó sin movimiento. Más tarde reportó STOP y corrida cero. En el barrido físico se observaron 56–59 sectores, cero errores de paquetes y una franja frontal persistente vacía alrededor de 10°–20°. La revisión posterior separa integridad y frescura del barrido de la cobertura que exige cada maniobra, conservando la comprobación frontal y la de giro/retroceso. Pendiente cargar esta revisión en el S3.

- S3 y puente ROS separan autorización para armar de permiso para mover motores. Una orden explícita puede quedar armada con motores a cero y salir después de tres barridos que confirmen la misma maniobra. STOP y la vigilancia del enlace siguen cancelando. El usuario cargó el S3, se restauró la calibración (0°, sentido horario, radio 150 mm) y la telemetría confirmó `puedeArmar=1`, `permiso=0`, `activa=0` y `moviendo=0`. Por instrucción del usuario no se hicieron pruebas de movimiento.

- Se compiló y probó una corrección del S3 para marcha recta ante un solo hueco intermitente. Usa únicamente una medición lejana real (al menos 800 mm) del barrido válido inmediatamente anterior y con una separación máxima de 150 ms. Mantiene el barrido crudo para radar, giros y retroceso. La revisión de 78 muestras reales halló un hueco persistente alrededor de 75° que esta corrección no cubre. Una proyección de 12 mapas favorece reorientar el carro a la izquierda antes de repetir la prueba. Firmware nuevo todavía pendiente de descarga al S3.

- Memoria visual automática en ROS 2 activo. Qwen3-VL etiquetó dos capturas físicas independientes como «columna» y el observador guardó el primer registro en el volumen Docker. Las pruebas de software verificaron persistencia y rechazo de observaciones discordantes. Se añadió al código del S3 un retroceso breve de recuperación, con verificación trasera, límite de tiempo y parada ante pérdida de cobertura. Probado en C++, pendiente de carga y prueba física. El permiso de marcha del pasillo siguió en cero durante los muestreos.

- ROS 2 conserva un giro verificado durante esperas temporales por cobertura o espacio. Guarda el éxito tras confirmar avance de los dos lados con encoders firmados. STOP, fallo y cambios de sesión descartan los intentos pendientes. Integración verificada con robot simulado y cambio publicado en el ROS 2 activo. En el pasillo real se verificaron cámara, /scan y telemetría con el carro parado. La cobertura intermitente tras girarlo a mano todavía impide validar aprendizaje de giros en el piso.

- Ajuste activo del puente ROS: tolera un fallo HTTP breve sin armar una corrida nueva y abandona la corrida tras 650 ms sin latido confirmado. Solo renueva con telemetría de la misma sesión y corrida. El S3 conserva el vencimiento local de 900 ms. La prueba simulada confirmó recuperación breve y STOP sin rearme tras interrupción sostenida.
- El aprendizaje también registra giros comprobados en escenarios con mayor espacio de un lado. La preferencia aprendida se usa dentro de espacios equivalentes. La simulación verificó ambas condiciones. Tras otro reinicio del S3 se restauraron frente 0°, sentido 1 y radio 150 mm. En el piso el LiDAR global estuvo válido, pero los sectores 70°–80° quedaron sin retorno de forma repetida y bloquearon la marcha. No se envió ARRANCAR con ese bloqueo.

- Actualización E2 instalada por el usuario y confirmada en el carro mediante telemetría por RobotCam. En reposo el contador bruto RR pasó de 2294 a 2315, con distancia y los cuatro contadores E2 fijos en cero. Reconectado Docker Desktop y levantados el puente ROS y Qwen3-VL; restaurados desde ROS frente 0°, sentido 1 y radio 150 mm tras el reinicio del S3. El modelo detectó y confirmó un cable blanco en dos cuadros reales (26 y 35); publicó `observado`. Verificados el tópico de encoders filtrados y el publicador de aprendizaje. Marcha detenida con permiso 0; en la revisión hubo 46–50 sectores y retornos cercanos a 100 mm. Sin prueba física de aprendizaje en esta sesión.

- Corrección de encoders E2 compilada y probada en PC. El DevKit E2 filtra flancos menores de 2 ms y separa pulsos brutos de los usados para recorrido, guardia y odometría. La carga OTA desde Codex fue bloqueada antes de ejecutarse. En el carro sigue el DevKit anterior. El puente ROS activo usa sus pulsos firmados para ignorar pulsos en reposo: verificado durante 12 muestras, con recorrido ROS 0,0 cm desde el reinicio del nodo y lectura bruta anterior de 1091,6 cm. Aprendizaje en ROS activo, exige evidencia firmada de ambos lados en sentidos de pivote y una salida confirmada. La simulación verificó aprendizaje persistente. La muestra física sigue pendiente mientras `permiso=0`.

- Primera prueba en el piso: 15 barridos previos con permiso y mínimo de al menos 579 mm. ROS envió ARRANCAR y STOP en una prueba de aproximadamente un segundo con STOP de respaldo. El DevKit confirmó movimiento y, tras STOP, `activa=0` y `moviendo=0`. Los cuatro contadores firmados avanzaron. El contador bruto de RR siguió subiendo de 427 a 441 después de la parada mientras los firmados se mantuvieron en 41/46/77/48. Verificar en reposo físico y revisar esa señal antes de usar la distancia del panel como evidencia. El recorrido de la prueba no se midió con regla. Sin prueba aún de giros ni aprendizaje en suelo.

- `ros2_robot_real/compose.yaml` ejecuta ROS 2 Humble y Qwen3-VL 2B Instruct local con llama.cpp. El observador publica `/camera/image_raw/compressed` a partir de `GET /captura`. Mantener separado del entorno Gazebo de Clase 05.
- `/robot/vision/instruccion` solicita dos observaciones en capturas nuevas. La segunda revisa la primera y publica el resultado en `/robot/vision/revision`. La prueba con Qwen real confirmó cambio de objeto visible a ausente usando JPEG de banco.
- Cámara física comprobada con el PC en `RobotCam` por Wi-Fi e Internet por anclaje USB. ROS 2 publicó JPEG y Qwen revisó dos capturas reales, corrigió un indicador de visibilidad contradictorio y publicó `sin_evidencia`.
- Integración ROS2-1 en `ros2_robot_real/`: control desde ROS con permiso local de 900 ms, telemetría pasiva, LiDAR, encoders, IMU y odometría con trocha calibrada. El aprendizaje persiste resultados de giros y solo desempata espacios comparables. Se incorporó el OTA del ZIP 1.4 aportado por el compañero, manteniendo las comprobaciones geométricas de la navegación 1.2.
- ROS2-1 ya está en el carro. S3 reportó `ROS2-1` y el DevKit entregó extensión `O1`; ROS recibió JPEG, IMU y contadores. Arduino descubrió dos puertos de red. OTA todavía no se ha probado subiendo un sketch. Con un computador de referencia centrado y luego movido 25 cm a la izquierda física, se verificó un cambio de unos 0° a 333°. Aplicados por ROS `frente_grados=0`, `sentido_lidar=1` y `radio_mm=150`; verificado `/scan` en ROS. El S3 conserva la geometría en RAM. En el banco la cobertura osciló entre 57 y 61 de 72 sectores y persisten ecos a unos 230 mm en torno a 80–100°. La marcha quedó desarmada y sin permiso. Pendientes despejar entorno, calibrar trocha, comprobar ambos giros y frenado, y validar el recorrido. Mantener intactos V1.2 y los respaldos.

### Prueba confirmada y corrección bidireccional — 2026-11-24

- El usuario confirmó éxito de la prueba autónoma V1.1 con objetos cercanos, con límites: solo izquierda y paradas que exigían ARRANCAR. Respaldados los15 archivos anteriores en `prueba_autonoma_exitosa_2026-11-24`, manifiestoSHA256 y nota de evidencia. No modificar ese respaldo ni las pruebas exitosas anteriores.
- Desarrollo actual en `navegacion_autonoma` V1.2: comparación de ambos lados, sentido angular físico configurable y obligatorio en la página, giros izquierda/derecha con cuatro ruedas. El tiempo6,5s ya no cancela la corrida: reevalúa otro lado con pausa. Recupera automáticamente espera de espacio o barridoLiDAR inválido/caducado tras tres barridos nuevos confirmados, con motores a cero mientras no haya datos/salida. Sin bajar distancias ni convertir retornos desconocidos en espacio libre.
- Video interrumpido ya no cancela una navegación con LiDAR y operador sanos. Navegador, UART, MPU y encoder siguen teniendo protección de parada enclavada, sin forzar rearme automático. La página conserva Última parada. DevKit identifica cuál encoder FL/RL/FR/RR dejó de contar. Cargar ambas placas para diagnóstico completo; protocoloV1/A1 compatible con el DevKit autónomo.
- Anticipación150mm adicional al radio150 y margen100. No significa permitir un obstáculo a150mm del sensor. Sigue siendo algoritmo reactivo, no aprendizajeRL ni garantía de ausencia de choque.
- Nuevas pruebas:28 combinaciones de frente/sentido para ambos lados, escenas variables,32 combinaciones de salud, integración planificador→CRC/UART→sketchDevKit con dobles y GPIO reales simulados, cuatro encoders e interfaz. `python navegacion_autonoma/pruebas/verificar.py`. Ver LEEME para tabla de recuperación. V1.2 requiere validación física; no describir las pruebas dePC como prueba del vehículo.

### Corrección de reanudación — 2026-11-24

- Usuario informa parada al corregir trayectoria. Falta motivo del panel para atribuir la causa real; consulta HTTP a /estado no accesible durante la revisión.
- `navegacion_autonoma` S3 V1.1: salida de giro envía avance directamente; obstáculo temporal o celda necesaria sin observar pasa a `ESPERANDO_ESPACIO`, conserva corrida y reanuda tras tres barridos nuevos que confirmen el mismo tipo de salida. STOP y fallos de sensores, comunicación, MPU, encoders o giro sin salida siguen cancelando. Pausas breves por inversión de motores permanecen.
- Anticipación V1.1 ajustada por petición del usuario a150mm (más radio150 y margen100, no umbral de150mm desde el sensor). La corrección usa las cuatro ruedas. Protocolo y DevKit sin cambios: para V1.1 recargar solo S3 con sus encabezados. Respaldo previo verificado en `respaldo_autonomia_antes_reanudacion_2026-11-24`. No confundir tests de software con validación física.
- Los 50 cm sugeridos para identificar la caja frontal no son obligatorios ni determinan el ángulo. Radio real informado150mm; captura anterior mostraba radio500 aplicado aunque formulario150: verificar configuración confirmada.

### Actualización de montaje y firmware — 2026-11-14

- Reparto actual: DevKitC controla cuatro motores, encoders y MPU6050; S3 controla cámara OV5640, COIN-D6 y página Wi-Fi. Las referencias posteriores a un único S3 son conceptuales o históricas.
- Capturas recientes confirman cámara, UART, MPU y LiDAR activos simultáneamente. D6 V3 con checksum del SDK: más de 31000 paquetes, cero errores, 68/72 sectores. No confundir esto con una prueba de evasión física completada.
- LiDAR: amarillo TX a S3 RX GPIO1; verde RX a S3 TX GPIO2; rojo a 5V regulados; negro a GND común. UART a 230400. UART entre placas permanece S3 TX41/RX42 y DevKit RX16/TX17 a 115200.
- Usuario confirmó XL4016 regulado a 5V y decidió usarlo como único regulador. Conservar niveles lógicos y cableado ya probado; no volver a proponer LM2596 ni repetir mediciones de tensión sin un fallo nuevo.
- Antes de modificar se copiaron y verificaron por SHA256 los ocho archivos de `prueba_integral_50cm` a `primera_prueba_exitosa_2026-11-14`. Conservar ambos sin cambios. Respaldo solicitado por el usuario como primera prueba exitosa; no afirmar que el desplazamiento físico de 50 cm fue certificado.
- Firmware nuevo en `navegacion_autonoma`: ARRANCAR/STOP, avance sin límite de distancia, evasión reactiva girando a la izquierda con cuatro salidas firmadas. Protocolo UART V1/A1: cargar AMBAS placas, no mezclar con firmware integral anterior. Mantiene paradas por fallos y ausencia de maniobra; no hay OTA, SLAM ni Nav2.
- Geometría: usuario declara LiDAR centrado y 15 cm hasta cada punta. Formulario propone radio de 150 mm; ángulo frontal pendiente de identificar en radar y confirmar antes de arrancar. Se pierde la confirmación al reiniciar S3. No asumir frente=0 grados.
- Planificador usa 36 celdas de 10 grados, mínimo real de cada par de sectores, sin borrar ecos cercanos. Desconocidos bloquean la maniobra que requiere esa zona. Radio + 100 mm de margen inicial y 500 mm de anticipación, pendientes de validar físicamente. Capturas anteriores mostraban ecos a 54–71 mm: no ocultarlos para habilitar marcha.
- Estado del nuevo firmware: compilación y pruebas automatizadas de protocolo, planificación, GPIO/UART/I2C simulados e interfaz. Pendiente carga y validación física de los sentidos de giro, margen de parada y evasión.

### Actualización de montaje — 2026-10-29

- Hay dos placas físicas: ESP32 DevKitC V4 con módulo ESP-32D/WROOM-32D para motores, encoders y MPU6050; ESP32-S3 anterior para cámara y comunicación. Este reparto sustituye las asignaciones antiguas al S3 que aparecen como arquitectura conceptual más abajo.
- Mapa de cableado actual: `MAPA_CONEXIONES.md`. Es diseño parcialmente probado; no asumir alimentación, sensores ni motores ya validados.
- Prueba UART `pruebas_uart/`: en capturas se confirmó DevKitC ejecutando receptor y recibiendo HOLA del S3 de manera repetida (contador al menos 12). Falta evidencia del monitor S3 confirmando RECIBIDO; no declarar validado ida y vuelta, cámara simultánea o ruido con motores.
- Puertos vistos: DevKitC COM5; S3 COM6. Pueden cambiar al reconectar.
- Diagnóstico actual: prueba MPU aún sin éxito; inicialmente no respondió en I2C y, después de cambios de alimentación, hubo capturas con `flash read err, 988` y reinicios/carga interrumpida. El usuario informa multiconector desde 3V3 compartido por driver/MPU. Esto no demuestra conexión incorrecta ni sobrecarga: revisar físicamente VCC frente a VM, continuidad y tensiones antes de reintentar cargas. Auditoría/correcciones en `REVISION_CABLEADO.md`; no cambiar GPIO ni mover sensores a 5 V sin verificar módulo y niveles.
- Prueba `pruebas_encoders/devkitc_encoders/devkitc_encoders.ino`: compilada y cargada por el usuario. Inicialmente falló carga (Wrong boot mode 0x13 / interrupción serial); BOOT sostenido + pulso EN permitió DOWNLOAD_BOOT y luego escritura verificada. El usuario confirmó que la prueba manual funciona; captura con TOTAL FL=80, RL=55, FR=140, RR=60 y NUEVOS=0 en reposo al final. Confirma conteo básico, no calibración, ausencia de ruido con motores, dirección ni distancia. VM/batería sin alimentar en esta fase. Siguiente etapa: prueba MPU6050 antes de energizar motores.
- UART cableada propuesta: S3 TX41→DevKitC RX16, DevKitC TX17→S3 RX42 y GND común. No unir salidas 3V3. Niveles de encoders/IMU y alimentación de motores siguen pendientes de evidencia técnica.

### Computación y control

- **ESP32-S3**
  - Microcontrolador principal del robot.
  - Se encargará principalmente de tareas de bajo nivel.
  - Control de motores.
  - Lectura de sensores.
  - Lectura de encoders.
  - Comunicación con el computador y/o ROS 2.

### Cámara

- **Módulo de cámara OV56** asociado al ESP32-S3. El modelo exacto no está confirmado en este documento.
- Su función principal será proporcionar imágenes para visión artificial.

La inferencia pesada de visión artificial no necesariamente debe ejecutarse en el ESP32.  
Puede ejecutarse en un computador externo y utilizar al ESP32 como nodo/controlador de hardware.

### Chasis

- Chasis móvil **4WD**.
- Cuatro ruedas.
- Cuatro motores DC.
- Movimiento principalmente mediante control diferencial.

Conceptualmente:

```text
lado izquierdo                  lado derecho

Motor FL ─┐                     ┌─ Motor FR
          ├── mismo sentido ────┤
Motor RL ─┘                     └─ Motor RR
```

El giro se realiza modificando la velocidad y/o sentido relativo entre ambos lados.

### Drivers de motores

- **2 × TB6612FNG**, cada uno con dos canales.

Cada driver controla dos motores.

Distribución orientativa:

```text
TB6612FNG #1
 ├── Motor delantero izquierdo
 └── Motor trasero izquierdo

TB6612FNG #2
 ├── Motor delantero derecho
 └── Motor trasero derecho
```

Esta distribución puede cambiar si el diseño físico o eléctrico lo requiere.

### Encoders

- **4 × encoders LM393** asociados a las ruedas/motores.

Objetivos:

- medir movimiento de las ruedas;
- estimar velocidad;
- apoyar la odometría;
- eventualmente implementar control cerrado de velocidad.

### IMU

- **MPU6050**

Objetivos principales:

- aceleración;
- velocidad angular;
- apoyo a estimación de orientación;
- posible fusión con odometría de ruedas.

### Sensor de distancia

- **Sensor D6** proporcionado por el profesor. Verificar sus características e interfaz antes de integrarlo.

Objetivos previstos:

- obtener distancias al entorno;
- detección geométrica de obstáculos;
- alimentar SLAM si sus datos e interfaz lo permiten;
- apoyar navegación autónoma.

### Alimentación

El sistema utiliza baterías y regulación de tensión.

Entre los componentes disponibles se encuentran:

- reguladores DC-DC **LM2596** y **XL4016**;
- baterías **18650** con protección integrada;
- condensadores **cerámicos** y **electrolíticos**;
- interruptor de **balancín**;
- **fusibles y portafusible en línea**;
- distribución de alimentación separando adecuadamente lógica y motores.

No modificar configuraciones eléctricas únicamente basándose en este documento.  
Consultar siempre el esquema eléctrico vigente del proyecto.

---

## 4. Arquitectura conceptual

La arquitectura de referencia es aproximadamente:

```text
                      ┌──────────────────────────────┐
                      │          COMPUTADOR          │
                      │                              │
Cámara ──────────────►│ OpenCV / VLM                │
Instrucción ─────────►│ percepción e interpretación│
                      │                              │
Sensor D6 ───────────►│ ROS 2                       │
                      │ SLAM / localización          │
                      │ Nav2 / navegación            │
                      │ lógica de alto nivel         │
                      │ objetivos validados         │
                      └──────────────┬───────────────┘
                                     │
                                 /cmd_vel
                              comunicación
                                     │
                                     ▼
                      ┌──────────────────────────────┐
                      │          ESP32-S3            │
                      │                              │
                      │ control de motores           │
                      │ lectura de encoders          │
                      │ lectura de IMU               │
                      │ comunicación con ROS 2       │
                      └──────────────┬───────────────┘
                                     │
                         ┌───────────┴───────────┐
                         ▼                       ▼
                    TB6612FNG                TB6612FNG
                         │                       │
                  motores izquierdos      motores derechos
```

Este diagrama representa una **separación conceptual de responsabilidades**.

No significa que todos los procesos deban ejecutarse exactamente en esos dispositivos ni que todos los nodos deban tener esos nombres.

---

## 5. Separación de responsabilidades

La arquitectura ideal intenta dividir el sistema aproximadamente en cuatro capas:

```text
PERCEPCIÓN        LOCALIZACIÓN       DECISIÓN        CONTROL
    │                  │                │               │
 cámara/VLM         SLAM/odom       Nav2/lógica       ESP32
    │                  │                │               │
    └──────────────────┴────────────────┴───────────────┘
                              ROS 2
```

### Percepción

Puede incluir:

- cámara;
- OpenCV;
- VLM para percepción visual e interpretación de instrucciones;
- sensor D6;
- procesamiento de sensores.

Responsabilidad:

> Convertir datos del mundo físico en información útil para el resto del sistema.

---

### Localización y representación del entorno

Puede incluir:

- odometría;
- encoders;
- IMU;
- sensor D6;
- SLAM;
- filtros de estimación de estado.

Responsabilidad:

> Determinar dónde está el robot y cómo es el entorno que lo rodea.

---

### Decisión y navegación

Puede incluir:

- ROS 2;
- Nav2;
- planificador de rutas;
- lógica propia;
- objetivos estructurados y validados a partir de la salida del VLM.

Responsabilidad:

> Transformar objetivos de alto nivel en acciones de navegación.

---

### Control

Principalmente asociado al ESP32.

Responsabilidades:

- recibir comandos de movimiento;
- controlar motores;
- leer encoders;
- leer sensores de bajo nivel;
- ejecutar lazos de control;
- informar estado al sistema superior.

---

## 6. ROS 2

ROS 2 es la capa de integración principal prevista para el sistema.

Siempre que tenga sentido, los componentes deben comunicarse mediante interfaces bien definidas.

Ejemplos orientativos:

```text
/cmd_vel
/odom
/scan
/imu
/camera/image_raw
```

Los nombres concretos pueden variar.

### Idea importante

El software de alto nivel debería depender de interfaces y no directamente del hardware.

Por ejemplo:

```text
Nav2
 │
 ▼
/cmd_vel
 │
 ├── Gazebo / robot simulado
 │
 └── ESP32 / robot real
```

Esto permite reutilizar una mayor parte del sistema entre simulación y hardware real.

---

## 7. Gazebo

Gazebo se utilizará, si resulta útil para el desarrollo, como simulador del robot.

La idea no es escribir "código para Gazebo" y después copiarlo al ESP32.

La estrategia deseada es:

1. Crear un modelo aproximado del robot.
2. Simular sensores y movimiento.
3. Conectar el robot simulado con ROS 2.
4. Desarrollar y probar nodos de alto nivel.
5. Reemplazar posteriormente el hardware simulado por interfaces del robot real.

Ejemplo:

```text
                ROS 2
                  │
          ┌───────┴────────┐
          │                │
       Gazebo          Robot real
          │                │
 sensores simulados    sensores físicos
 motores simulados     ESP32 + motores
```

La simulación debe considerarse una herramienta de desarrollo, no una obligación si introduce una complejidad innecesaria para los objetivos académicos.

---

## 8. Visión artificial

### OpenCV

Puede utilizarse para:

- captura y manipulación de imágenes;
- conversiones de color;
- filtros;
- geometría;
- calibración;
- preprocesamiento;
- procesamiento posterior a una detección.

### VLM

Un **modelo de visión y lenguaje (VLM)** combina la imagen de la cámara con una instrucción para reconocer objetos relevantes e interpretar la intención. Por ejemplo, ante "Ve a la izquierda de las gafas", puede producir una salida estructurada como:

```text
acción: navegar
objeto_referencia: gafas
relación_espacial: izquierda
evidencia_visual: gafas identificadas en la imagen
```

La presencia de un objeto en la imagen no determina por sí sola una posición navegable. Otros componentes deben relacionar la observación con la localización del robot y el entorno, calcular un objetivo y ejecutar la navegación.

---

## 9. Instrucciones y percepción con VLM

Flujo conceptual:

```text
imagen de cámara + "Ve a la izquierda de las gafas"
                         │
                         ▼
                        VLM
             objeto e intención estructurados
                         │
                         ▼
          validación y posición objetivo
             con localización y sensores
                         │
                         ▼
                   Nav2 / lógica
                         │
                         ▼
                      /cmd_vel
                         │
                         ▼
                    ESP32-S3
                         │
                         ▼
                      motores
```

El VLM **no debe controlar directamente los motores**. Una capa intermedia debe validar su salida y traducirla a objetivos seguros y concretos. Si el objeto o la relación espacial no se pueden determinar con suficiente claridad, no se debe iniciar el movimiento basado en esa instrucción.

---

## 10. Programación distribuida y paralela

El proyecto debe aprovechar su naturaleza distribuida.

Ejemplos de procesos que podrían ejecutarse de forma independiente:

```text
Nodo de cámara
Nodo de VLM (visión e instrucciones)
Nodo del sensor D6
Nodo de IMU
Nodo de odometría
Nodo de SLAM
Nodo de navegación
Nodo puente con ESP32
```

No es obligatorio crear un nodo separado para cada función.

La división debe responder a una necesidad real de:

- modularidad;
- procesamiento paralelo;
- comunicación;
- independencia;
- reutilización;
- mantenimiento.

Evitar fragmentar el sistema artificialmente solo para aumentar el número de nodos.

---

## 11. Prioridades del proyecto

Al trabajar sobre el código, priorizar aproximadamente este orden:

```text
1. Robot puede moverse de forma controlada
          ↓
2. Comunicación estable computador ↔ ESP32
          ↓
3. Encoders / odometría
          ↓
4. IMU
          ↓
5. Sensor D6
          ↓
6. Integración ROS 2
          ↓
7. Simulación
          ↓
8. SLAM / localización
          ↓
9. Navegación autónoma
          ↓
10. Percepción visual e interpretación de instrucciones con VLM
          ↓
11. Relación objeto ↔ posición
```

Este orden es una recomendación y puede cambiar según las necesidades de la asignatura.

No intentar implementar todo simultáneamente.

---

## 12. Reglas para agentes de IA

Cuando un agente trabaje sobre este repositorio:

### Debe

- leer antes de modificar;
- explicar cambios arquitectónicos importantes;
- mantener modularidad;
- evitar acoplar visión, navegación y control de motores innecesariamente;
- reutilizar interfaces estándar de ROS 2 cuando tenga sentido;
- documentar supuestos;
- distinguir claramente código experimental de código funcional;
- priorizar implementaciones que puedan probarse por etapas;
- conservar compatibilidad entre simulación y robot real cuando sea razonable.

### No debe

- asumir que todo lo descrito en este archivo ya está implementado;
- reemplazar una arquitectura funcional solo porque exista una alternativa más sofisticada;
- instalar dependencias pesadas sin justificación;
- modificar configuración eléctrica o pines sin revisar el código/esquema vigente;
- inventar características de sensores o componentes;
- integrar el VLM con el movimiento antes de validar las capas básicas de percepción, navegación y control;
- introducir abstracciones complejas innecesarias.

---

## 13. Filosofía del proyecto

La meta no es construir el robot más complejo posible.

La meta es construir un sistema que:

- funcione;
- pueda explicarse;
- pueda demostrarse;
- esté dividido en componentes comprensibles;
- permita observar programación distribuida y paralela;
- pueda ampliarse progresivamente.

Preferir:

```text
simple + funcional + demostrable
```

sobre:

```text
sofisticado + frágil + difícil de explicar
```

---

## 14. Guías del profesor y adaptación al hardware real

Durante el semestre el profesor puede entregar guías, ejemplos, prácticas o procedimientos.

Estas guías deben utilizarse como **referencia funcional y académica**, pero no debe asumirse que el hardware utilizado en ellas coincide con el hardware de este proyecto.

Una guía puede estar diseñada para compañeros que tengan menos hardware, otro hardware o una configuración diferente.

Por tanto, el objetivo no es copiar ciegamente la guía, sino:

> entender qué se pretende lograr y determinar cómo implementarlo correctamente con el hardware que realmente tenemos.

---

### Regla principal

Ante una nueva guía:

```text
Guía del profesor
       │
       ▼
¿Qué funcionalidad o aprendizaje busca?
       │
       ▼
¿Qué hardware tenemos realmente?
       │
       ▼
¿Podemos implementar esa funcionalidad
con nuestro hardware actual?
       │
       ├── Sí → adaptar la implementación
       │        al hardware disponible
       │
       └── No → señalar la limitación
                y NO comprar, sustituir
                ni inventar hardware
                automáticamente
```

---

### Las guías NO redefinen automáticamente nuestro hardware

Si una guía utiliza, por ejemplo:

```text
ESP32
+
cámara
+
motores
```

pero nuestro robot además dispone de:

```text
sensor D6
IMU
encoders
mejores drivers
u otros componentes
```

no se deben ignorar automáticamente esos componentes adicionales.

Se debe evaluar si pueden utilizarse de manera útil para implementar mejor, con mayor precisión o de forma más robusta lo que pide la guía.

Ejemplo conceptual:

```text
La guía pide:
"mover el robot cierta distancia"

Compañeros:
tiempo de activación de motores

Nuestro proyecto:
encoders + odometría
```

Si los encoders permiten cumplir el mismo objetivo de una forma más apropiada para nuestro hardware, puede utilizarse esa solución siempre que no contradiga explícitamente la actividad.

---

### Tampoco se debe usar todo el hardware porque esté disponible

Disponer de un componente no significa que deba utilizarse en todas las prácticas.

Si una actividad puede resolverse de forma simple con:

```text
ESP32 + motores
```

no es obligatorio introducir:

```text
sensor D6 + VLM + SLAM + Nav2
```

solo porque estén contemplados en el proyecto.

Usar únicamente los componentes que aporten valor real a la tarea actual.

---

### Cuando una guía utiliza hardware que NO tenemos

Si una guía menciona un módulo, sensor, placa o componente que no poseemos:

**NO asumir automáticamente que hay que comprarlo.**

**NO buscar automáticamente un sustituto.**

**NO modificar la arquitectura para introducir un equivalente.**

Primero se debe determinar qué papel cumple ese componente dentro de la guía.

Después comprobar si:

1. la función puede realizarse con hardware que ya tenemos;
2. otra parte de nuestra arquitectura ya proporciona una capacidad equivalente;
3. esa sección puede omitirse sin impedir el objetivo principal de la práctica;
4. la actividad depende necesariamente de ese componente.

El resultado debe comunicarse claramente.

Ejemplo:

```text
La guía utiliza: sensor X

Nuestro robot: no tiene sensor X
       │
       ▼
Analizar qué hace sensor X
       │
       ├── La función puede cubrirla nuestro sensor D6
       │       → verificar sus capacidades y adaptar
       │
       ├── Esa parte es opcional o irrelevante
       │       → omitirla
       │
       └── Es indispensable y no tenemos equivalente
               → informar la limitación
                  y esperar decisión del usuario
```

---

### Prohibición de adquirir o sustituir hardware por iniciativa del agente

Un agente de IA que trabaje en este repositorio **no debe decidir que hace falta comprar hardware nuevo**.

Tampoco debe asumir que el usuario desea:

- comprar un módulo mencionado en una guía;
- reemplazarlo por otro componente;
- buscar un equivalente comercial;
- rediseñar el robot para adaptarlo a ese módulo;
- cambiar componentes ya comprados.

Eso solamente debe hacerse cuando **el usuario lo decida explícitamente**.

Hasta entonces, trabajar con:

```text
hardware disponible
+
software disponible
+
objetivo funcional de la guía
```

---

### Tratamiento recomendado de cada guía nueva

Cuando se reciba una guía nueva:

1. Identificar el objetivo de la práctica.
2. Separar los requisitos funcionales de los componentes concretos utilizados en el ejemplo.
3. Comparar esos requisitos con nuestro hardware actual.
4. Clasificar cada parte como:

```text
A. Aplicable directamente
B. Aplicable con adaptación
C. Ya cubierta por hardware mejor o diferente
D. No aplicable por falta de hardware
E. Prescindible para nuestro caso
```

5. Implementar A, B y C cuando corresponda.
6. Omitir D o E cuando sea razonable.
7. Si una parte D es indispensable para cumplir el objetivo, informarlo antes de proponer compras o sustituciones.
8. No buscar sustitutos de hardware salvo petición expresa del usuario.
9. Mantener el resto del robot funcional y evitar romper capacidades existentes por seguir literalmente una guía.

---

### Prioridad práctica

Cuando una guía y nuestra configuración difieran, pensar en este orden:

```text
1. ¿Qué quiere enseñar o conseguir la guía?
                 ↓
2. ¿Qué necesita realmente la actividad?
                 ↓
3. ¿Qué hardware tenemos?
                 ↓
4. ¿Cómo logramos el mismo objetivo con ese hardware?
                 ↓
5. ¿Hay partes de la guía que no aplican a nuestro robot?
                 ↓
6. Omitirlas o marcarlas como no aplicables.
```

La prioridad es cumplir correctamente el objetivo académico **con nuestro robot**, no reproducir exactamente el montaje de otro grupo.

---

### Hardware conocido actualmente

Mantener como referencia el inventario conocido del proyecto:

```text
ESP32-S3 con módulo de cámara OV56
chasis 4WD
ruedas
4 motores DC
4 encoders LM393
2 × TB6612FNG de dos canales
MPU6050
sensor D6 proporcionado por el profesor
LM2596
XL4016
condensadores cerámicos y electrolíticos
interruptor de balancín
baterías 18650 con protección integrada
fusibles y portafusible en línea
```

Este inventario puede cambiar durante el proyecto.

Cuando cambie, actualizar esta sección o la sección de hardware principal del documento.

## 15. Estado cambiante del proyecto

Este archivo describe el contexto acumulado, el hardware conocido y la dirección general del proyecto, pero **no es una fuente definitiva para cada detalle técnico**.

Debe conservarse como memoria del proyecto para evitar perder decisiones previas.

Las guías nuevas deben interpretarse junto con este contexto, no reemplazarlo automáticamente.

Ante discrepancias, considerar en conjunto:

```text
objetivo de la actividad
+
hardware realmente disponible
+
estado actual del código
+
decisiones recientes del equipo
```

Si el repositorio, el hardware o las decisiones del proyecto evolucionan significativamente, actualizar este documento.

---

## 16. Resumen rápido para un agente nuevo

Si acabas de entrar al repositorio, entiende esto primero:

> Estamos construyendo un robot móvil 4WD para Programación Distribuida y Paralela. El ESP32-S3 controla el hardware de bajo nivel. El robot tiene un módulo de cámara OV56, cuatro motores con encoders, dos TB6612FNG, MPU6050, sensor D6 y los componentes de alimentación indicados en el inventario. Un computador puede ejecutar ROS 2, percepción con VLM, localización y navegación. El VLM combina la imagen con instrucciones como "ve a la izquierda de las gafas" y entrega información para calcular un objetivo validado; Nav2 o la lógica de navegación y el ESP32-S3 se encargan del movimiento. Gazebo puede utilizarse para simular el robot. Esta arquitectura es una guía: revisar siempre el estado actual del repositorio antes de realizar cambios.
