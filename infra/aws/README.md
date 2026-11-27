# Modelos del robot en AWS

Cuenta de trading bot **473309510385**, perfil local **bot-trending**, región **us-east-1**. Terraform valida el ID de cuenta antes de operar. La instancia se llama **easo-robot-modelos**. El archivo de estado local debe conservarse en el equipo administrador.

La EC2 ejecuta los servicios LLM, VLM y GPU. Los dos primeros comparten Qwen3-VL-30B-A3B-Instruct Q4_K_M con su proyector Q8_0. Tiene 4096 tokens de contexto, una sesión simultánea y las capas en la GPU. Gateway, VLA, orquestador, ESP32 y LiDAR siguen en el Victus y el robot. El túnel de Systems Manager transporta las solicitudes a los modelos.

## Infraestructura

Desde esta carpeta, el administrador ejecuta:

```powershell
terraform init
terraform plan -out base.tfplan
terraform apply base.tfplan
```

El primer plan crea IAM, los documentos de Systems Manager y el grupo de seguridad. Cuando AWS conceda 4 vCPU On-Demand para G y VT, guarda el mismo token de `servicios/.env` como un **SecureString** con nombre `/easo-robot-modelos/token` en Parameter Store de esta cuenta. Se aceptan entre 16 y 128 caracteres alfanuméricos o `._~-`. El token se mantiene fuera de Git y del estado Terraform.

Crea `trading.auto.tfvars` con `provision_gpu = true` en el equipo administrador y conserva ese ajuste durante la vida de la instancia. Luego ejecuta:

```powershell
terraform plan -out gpu.tfplan
terraform apply gpu.tfplan
gh variable set ROBOT_AWS_ROLE --body "$(terraform output -raw github_role)"
gh variable set ROBOT_AWS_INSTANCE_ID --body "$(terraform output -raw instance_id)"
```

Espera que la instancia aparezca en Systems Manager y ejecuta el workflow **Desplegar modelos en AWS** desde `main`. La primera ejecución descarga aproximadamente 19 GB de pesos. La instancia acepta tráfico entrante únicamente a través de Systems Manager. El grupo de seguridad carece de reglas de entrada.

## Colaborador

El usuario IAM **easo-robot-modelos-colaborador** puede invocar el documento de despliegue y abrir el túnel de modelos. Si requiere ejecutar AWS CLI directamente, el administrador configura una credencial para ese usuario y la entrega por un canal privado. Para acceso a código, añade la cuenta GitHub del compañero como colaborador del repositorio.

El compañero edita desde VS Code, hace commit y push a su rama, y se integra el cambio a `main`. Cada push a `main` activa GitHub Actions, que se autentica con un rol IAM mediante OIDC y envía el SHA a Systems Manager. La EC2 comprueba que el SHA esté publicado en `main`. También puede volver a ejecutar el workflow desde la pestaña Actions. Los cambios de infraestructura se aplican con Terraform desde la sesión del administrador.

## Conectar el Victus

Instala AWS CLI y el complemento Session Manager. Con el perfil del colaborador o el perfil administrador, ejecuta en PowerShell reemplazando el ID por el de Terraform:

```powershell
aws ssm start-session --target i-ID --document-name easo-robot-modelos-tunnel --profile bot-trending --region us-east-1
```

Mantén esa ventana abierta. Desde `servicios` verifica los tres endpoints `http://127.0.0.1:18080/llm/salud`, `http://127.0.0.1:18080/vlm/salud` y `http://127.0.0.1:18080/gpu/salud`. Con el token correspondiente en `.env`, arranca la aplicación local:

```powershell
docker compose -f compose.yaml -f compose.victus-aws.yaml up -d --build vla orquestador gateway
```

La misión continúa en http://localhost:4000/. Comprueba que los servicios estén listos antes de enviar órdenes. Para volver al modo local, levanta el Compose habitual con los perfiles `modelos` y `llm-local`.

La EC2 se cobra mientras está encendida, incluso entre misiones. Detenla desde EC2 al cerrar la sesión. El disco EBS persiste. Si alguien publica mientras está detenida, vuelve a ejecutar el workflow después de iniciarla.
