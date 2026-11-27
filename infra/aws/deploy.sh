#!/bin/bash
set -euo pipefail
umask 077
cloud-init status --wait
cd /opt/robot
exec 9>deployment.lock
flock -w 900 9
commit="${SSM_Commit:?}"
[[ "$commit" =~ ^[0-9a-f]{40}$ ]]
remote=https://github.com/AndresSanchez12323/robot-4wd-ros2.git
if [[ ! -d repo/.git ]]; then
  git clone --no-checkout "$remote" repo
fi
git -C repo fetch "$remote" +refs/heads/main:refs/remotes/origin/main
git -C repo merge-base --is-ancestor "$commit" refs/remotes/origin/main
# Una ejecución que se quedó en cola no debe desplegar un commit viejo.
if [[ "$commit" != "$(git -C repo rev-parse refs/remotes/origin/main)" ]]; then
  echo "Omitido: ya hay otro commit publicado en main"
  exit 0
fi
previous=$(cat revision 2>/dev/null || true)
git -C repo checkout --detach "$commit"
token=$(aws ssm get-parameter --name /easo-robot-modelos/token --region us-east-1 --with-decryption --query Parameter.Value --output text)
[[ "$token" =~ ^[a-zA-Z0-9._~-]{16,128}$ ]]
printf 'TOKEN=%s\n' "$token" >runtime.env
unset token
export MODELOS_DIR=/opt/robot/modelos
install -d -m 0750 "$MODELOS_DIR"
model=Qwen3VL-30B-A3B-Instruct-Q4_K_M.gguf
projector=mmproj-Qwen3VL-30B-A3B-Instruct-Q8_0.gguf
base=https://huggingface.co/Qwen/Qwen3-VL-30B-A3B-Instruct-GGUF/resolve/f54435e6cc31258f04b0969105c3f6badb197931
for name in "$model" "$projector"; do
  if [[ ! -s "$MODELOS_DIR/$name" ]]; then
    curl --fail --location --retry 4 --retry-delay 2 "$base/$name" -o "$MODELOS_DIR/$name.partial"
    case "$name" in
      "$model") sha=87bb374d849f80ebdfabb304189fac9e0bd35a0f74506e6a59c51b206cbe863b ;;
      "$projector") sha=82a9966edfdbc1b18a27fd90af96d50cbf111a51484f6290e946dccf461c8150 ;;
    esac
    printf '%s  %s\n' "$sha" "$MODELOS_DIR/$name.partial" | sha256sum -c -
    mv "$MODELOS_DIR/$name.partial" "$MODELOS_DIR/$name"
  fi
done
compose() {
  docker compose --env-file /opt/robot/runtime.env -f /opt/robot/repo/servicios/compose.aws.yaml "$@"
}
health() {
  for ((attempt=0;attempt<120;attempt++)); do
    if curl -fsS --max-time 4 http://127.0.0.1:8080/llm/salud >/dev/null &&
       curl -fsS --max-time 4 http://127.0.0.1:8080/vlm/salud >/dev/null &&
       curl -fsS --max-time 4 http://127.0.0.1:8080/gpu/salud >/dev/null; then
      return 0
    fi
    sleep 3
  done
  return 1
}
if compose up -d --build && health; then
  printf '%s\n' "$commit" >revision
else
  compose logs --tail 40 modelo
  if [[ -n "$previous" && "$previous" != "$commit" ]]; then
    git -C repo checkout --detach "$previous"
    compose up -d --build
    health
  fi
  exit 1
fi
