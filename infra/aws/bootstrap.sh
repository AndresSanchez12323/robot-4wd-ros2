#!/bin/bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
install -d -m 0700 /opt/robot
apt-get update
apt-get install -y ca-certificates curl git unzip
if ! docker compose version >/dev/null 2>&1; then
  install -d -m 0755 /etc/apt/keyrings
  curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
  echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu jammy stable" >/etc/apt/sources.list.d/docker.list
  apt-get update
  apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
fi
if ! command -v aws >/dev/null; then
  curl -fsSL https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip -o /tmp/robot-aws.zip
  unzip -q /tmp/robot-aws.zip -d /tmp/robot-aws-cli
  /tmp/robot-aws-cli/aws/install
fi
nvidia-ctk runtime configure --runtime=docker
systemctl enable docker
systemctl restart docker
nvidia-smi
