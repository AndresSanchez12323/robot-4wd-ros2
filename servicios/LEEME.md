# Misión por servicios

El carro sigue con el firmware 1.6. Estos programas deciden a dónde ir. Cada uno vive en su contenedor. La página y las consultas entran solo por el gateway GraphQL. Si uno no responde, la consulta falla y el carro no se mueve.

La prueba se hace en el Victus de Edwin, porque ahí Docker puede ver la NVIDIA. El LLM no corre en el Victus: se publica con `compose.nube.yaml` en un servidor y el Victus lo llama por `LLM_URL`.

## Qué hay

| Servicio | Programa | Se apaga y entonces |
|---|---|---|
| LLM | contenedor en la nube | la frase no se convierte en lugares y el carro no arranca |
| VLM | `vlm` en el Victus | no hay reconocimiento de la cámara |
| GPU | `gpu` en el Victus | el VLM deja de responder, aunque el modelo siga encendido |
| VLA | `vla` en el Victus | no sale la siguiente acción y el carro se detiene |
| Robot | las dos placas | la frase puede interpretarse, pero no hay movimiento |
| Orquestador | red interna, sin puerto publicado | recorre los servicios cuando el gateway se lo pide |
| Gateway | Apollo, `http://localhost:4000/graphql` | única puerta. Si se apaga, la página no puede pedir la misión |

El catálogo está en `orquestador/catalogo.json`. Los lugares de clase siguen primero: cocina 1, baño 2, garaje 3 y salida 4. «Me quiero lavar los dientes» tiene que salir como baño. «Ve a la cocina, al baño y al garaje» tiene que salir `1 2 3`.

Para ensayar sin armar una cocina se agregaron objetos, con números nuevos: gaseosa 5, celular 6, bolsa 7, gafas 8, botella 9, libro 10, llaves 11, zapato 12, silla 13, mesa 14, pelota 15 y mochila 16. «Quiero la gaseosa» sale como 5. «Ve a la gaseosa, al celular y a la bolsa» sale `5 6 7`. «Ve al bolso» también sale como bolsa, 7.

En un salón de universidad se pueden pedir, además: cuaderno 17, lápiz 18, esfero 19, borrador 20, marcador 21, cartuchera 22, computador 23, audífonos 24, cargador 25, carnet 26, regla 27 y reloj 28. «Ve al cuaderno» sale como 17. «Ve al reloj» sale como 28. «Ve a la mochila y al computador» sale `16 23`.

## Antes de arrancar los contenedores

1. En el S3, cargar otra vez `navegacion_autonoma/s3_integral/s3_integral.ino`. El DevKit que ya acepta PWM 140 no cambia. La cámara queda en 800×600. La página del carro debe seguir diciendo Versión 1.6 y, además, que acepta misiones del orquestador. Antes de cada foto el orquestador detiene el carro, mira y recién ahí lo mueve. Si ve el objeto al frente, lo confirma y avanza hasta quedar cerca. Si está cerca y la cámara lo confirma, se detiene aunque el modelo diga solo «centro» o «una botella». Si no lo ve y el frente está libre, avanza para buscarlo. Solo gira cuando hay una pared, y siempre hacia el mismo lado.
2. Confirmar en `http://192.168.4.1/` el frente, el radio 150 mm y que los ángulos crecen hacia la izquierda.
3. El Victus entra a Lamborgini, con la clave `andresx3`, y conserva internet por cable o por el celular. Sin internet el LLM de la nube sale apagado y el carro no se mueve.
4. Copiar `.env.example` a `.env`. Cambiar `TOKEN` y poner en `LLM_URL` la dirección pública del contenedor de la nube. El mismo token va en los dos sitios.
5. Instalar en el Victus el driver NVIDIA, Docker Desktop con WSL2 y el acceso de los contenedores a la GPU. Esta comprobación tiene que mostrar la tarjeta:

```powershell
docker run --rm --gpus all nvidia/cuda:12.4.1-base-ubuntu22.04 nvidia-smi
```

6. Bajar los modelos a la carpeta `MODELOS_DIR`:
   - `llm.gguf`: Qwen2.5 1.5B Instruct, archivo `qwen2.5-1.5b-instruct-q4_k_m.gguf`
   - `Qwen3VL-2B-Instruct-Q4_K_M.gguf` y `mmproj-Qwen3VL-2B-Instruct-Q8_0.gguf` para la cámara

## Nube

En el servidor, desde esta carpeta:

```bash
docker compose -f compose.nube.yaml up -d --build
```

Queda publicado el LLM para que el gateway del Victus lo llame. El navegador no usa esa dirección. Para la demostración se apaga ese contenedor y la página del Victus debe mostrar el LLM en rojo, con el carro quieto.

## Victus

Desde esta carpeta, con el `.env` ya escrito:

```powershell
docker compose --profile modelos up -d --build
```

Abrir `http://localhost:4000/`. La página pide el token una vez y solo habla con `/graphql`. También se puede abrir `http://localhost:4000/graphql` y lanzar la consulta `iniciar`. Los servicios tienen que decir LISTO. Recién ahí se escribe la frase y se pulsa IR.

`docker compose stop gateway` deja la página sin respuesta. `docker compose stop gpu` deja el VLM en rojo y `iniciar` no mueve el carro. `docker compose stop vla` detiene una marcha que ya iba. `docker compose stop vlm` hace lo mismo. Parar el contenedor del LLM en la nube impide empezar.

Si todavía no hay servidor, el mismo LLM puede correr en el Victus solo para ensayar, con `LLM_URL=http://llm:8080` y:

```powershell
docker compose --profile llm-local --profile modelos up -d --build
```

Ese modo no cumple la parte de la nube. La entrega usa `compose.nube.yaml`.

## Lo que no hace solo

Estas pruebas de PC no mueven el carro:

```bash
python3 pruebas/test_servicios.py
node --test gateway/resolvers.test.js
```

El carro, con los motores sin alimentación, se comprueba en `navegacion_autonoma` con `python3 pruebas/verificar.py`. Después hace falta la prueba en el piso, con PARAR a mano: una frase hacia una gaseosa, la secuencia cocina-baño-garaje y apagar cada servicio.
