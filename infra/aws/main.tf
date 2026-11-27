terraform {
  required_version = ">= 1.6"
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 6.0"
    }
  }
}
provider "aws" {
  profile             = "bot-trending"
  region              = "us-east-1"
  allowed_account_ids = ["473309510385"]
  default_tags {
    tags = { Project = "robot-4wd-ros2", ManagedBy = "Terraform" }
  }
}
variable "provision_gpu" {
  type        = bool
  default     = false
  description = "Habilitar después de la aprobación de cuota y configuración del token."
}
locals {
  name     = "easo-robot-modelos"
  repo     = "AndresSanchez12323/robot-4wd-ros2"
  instance = "arn:aws:ec2:us-east-1:473309510385:instance/*"
}
data "aws_vpc" "default" {
  default = true
}
data "aws_subnet" "gpu" {
  vpc_id = data.aws_vpc.default.id
  filter {
    name   = "availability-zone"
    values = ["us-east-1a"]
  }
  filter {
    name   = "default-for-az"
    values = ["true"]
  }
}
data "aws_ssm_parameter" "ami" {
  name = "/aws/service/deeplearning/ami/x86_64/base-oss-nvidia-driver-gpu-ubuntu-22.04/latest/ami-id"
}
resource "aws_security_group" "gpu" {
  name        = local.name
  description = "Solo salida web para SSM y descarga de modelos"
  vpc_id      = data.aws_vpc.default.id
  egress {
    from_port   = 443
    to_port     = 443
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  egress {
    from_port   = 80
    to_port     = 80
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
}
resource "aws_iam_role" "ec2" {
  name = "${local.name}-ec2"
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect = "Allow", Principal = { Service = "ec2.amazonaws.com" }, Action = "sts:AssumeRole"
    }]
  })
}
resource "aws_iam_role_policy" "ssm" {
  name = "ssm-agent"
  role = aws_iam_role.ec2.name
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect = "Allow"
      Action = [
        "ssm:DescribeAssociation", "ssm:GetDocument", "ssm:DescribeDocument",
        "ssm:ListAssociations", "ssm:ListInstanceAssociations",
        "ssm:UpdateAssociationStatus", "ssm:UpdateInstanceAssociationStatus",
        "ssm:UpdateInstanceInformation",
        "ssmmessages:CreateControlChannel", "ssmmessages:CreateDataChannel",
        "ssmmessages:OpenControlChannel", "ssmmessages:OpenDataChannel",
        "ec2messages:AcknowledgeMessage", "ec2messages:DeleteMessage",
        "ec2messages:FailMessage", "ec2messages:GetEndpoint",
        "ec2messages:GetMessages", "ec2messages:SendReply"
      ]
      Resource = "*"
    }]
  })
}
resource "aws_iam_role_policy" "token" {
  name = "token"
  role = aws_iam_role.ec2.name
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect   = "Allow", Action = "ssm:GetParameter"
      Resource = "arn:aws:ssm:us-east-1:473309510385:parameter/${local.name}/token"
    }]
  })
}
resource "aws_iam_instance_profile" "gpu" {
  name = local.name
  role = aws_iam_role.ec2.name
}
resource "aws_instance" "gpu" {
  count                       = var.provision_gpu ? 1 : 0
  ami                         = data.aws_ssm_parameter.ami.value
  instance_type               = "g6.xlarge"
  subnet_id                   = data.aws_subnet.gpu.id
  associate_public_ip_address = true
  vpc_security_group_ids      = [aws_security_group.gpu.id]
  iam_instance_profile        = aws_iam_instance_profile.gpu.name
  tags                        = { Name = local.name }
  metadata_options {
    http_tokens                 = "required"
    http_put_response_hop_limit = 1
  }
  root_block_device {
    volume_type = "gp3"
    volume_size = 80
    encrypted   = true
  }
  user_data = file("${path.module}/bootstrap.sh")
  lifecycle {
    ignore_changes = [ami]
  }
  depends_on = [aws_iam_role_policy.ssm, aws_iam_role_policy.token]
}
resource "aws_ssm_document" "deploy" {
  name          = "${local.name}-deploy"
  document_type = "Command"
  content = jsonencode({
    schemaVersion = "2.2"
    parameters = {
      Commit = { type = "String", allowedPattern = "^[0-9a-f]{40}$", interpolationType = "ENV_VAR" }
    }
    mainSteps = [{
      action = "aws:runShellScript", name = "Deploy"
      inputs = {
        timeoutSeconds = "3600"
        runCommand     = [file("${path.module}/deploy.sh")]
      }
    }]
  })
}
resource "aws_ssm_document" "tunnel" {
  name          = "${local.name}-tunnel"
  document_type = "Session"
  content = jsonencode({
    schemaVersion = "1.0", sessionType = "Port"
    properties    = { portNumber = "8080", localPortNumber = "18080", type = "LocalPortForwarding" }
  })
}
resource "aws_iam_openid_connect_provider" "github" {
  url            = "https://token.actions.githubusercontent.com"
  client_id_list = ["sts.amazonaws.com"]
}
resource "aws_iam_role" "github" {
  name = "${local.name}-github"
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow", Action = "sts:AssumeRoleWithWebIdentity"
      Principal = { Federated = aws_iam_openid_connect_provider.github.arn }
      Condition = { StringEquals = {
        "token.actions.githubusercontent.com:aud" = "sts.amazonaws.com"
        "token.actions.githubusercontent.com:sub" = "repo:${local.repo}:ref:refs/heads/main"
      } }
    }]
  })
}
data "aws_iam_policy_document" "deploy" {
  statement {
    actions   = ["ssm:SendCommand"]
    resources = [aws_ssm_document.deploy.arn]
  }
  statement {
    actions   = ["ssm:SendCommand"]
    resources = [local.instance]
    condition {
      test     = "StringEquals"
      variable = "ssm:resourceTag/Name"
      values   = [local.name]
    }
  }
  statement {
    actions   = ["ssm:GetCommandInvocation"]
    resources = ["*"]
  }
}
resource "aws_iam_role_policy" "github" {
  name   = "deploy"
  role   = aws_iam_role.github.name
  policy = data.aws_iam_policy_document.deploy.json
}
resource "aws_iam_user" "developer" {
  name = "${local.name}-colaborador"
}
resource "aws_iam_user_policy" "deploy" {
  name   = "deploy"
  user   = aws_iam_user.developer.name
  policy = data.aws_iam_policy_document.deploy.json
}
resource "aws_iam_user_policy" "tunnel" {
  name = "tunnel"
  user = aws_iam_user.developer.name
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      { Effect = "Allow", Action = "ssm:StartSession", Resource = aws_ssm_document.tunnel.arn },
      {
        Effect    = "Allow", Action = "ssm:StartSession", Resource = local.instance
        Condition = { StringEquals = { "ssm:resourceTag/Name" = local.name } }
      },
      {
        Effect   = "Allow", Action = ["ssm:ResumeSession", "ssm:TerminateSession", "ssmmessages:OpenDataChannel"]
        Resource = "arn:aws:ssm:us-east-1:473309510385:session/$${aws:username}-*"
      }
    ]
  })
}
output "instance_id" {
  value = one(aws_instance.gpu[*].id)
}
output "github_role" {
  value = aws_iam_role.github.arn
}
output "developer" {
  value = aws_iam_user.developer.name
}
