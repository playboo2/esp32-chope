#!/usr/bin/env bash
set -euo pipefail

# Nome do ambiente definido em platformio.ini, ex:
# [env:esp32dev]  -> ENV_NAME="esp32dev"
ENV_NAME="esp32dev"   # TODO: ajuste para o seu env

# Vai para o diretório do projeto (onde está o platformio.ini)
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

echo "==> Build do ambiente '$ENV_NAME'..."
pio run -e "$ENV_NAME"

BIN_SRC=".pio/build/${ENV_NAME}/firmware.bin"
BIN_DIR="build"
BIN_DST="${BIN_DIR}/${ENV_NAME}-firmware.bin"

mkdir -p "$BIN_DIR"

if [[ ! -f "$BIN_SRC" ]]; then
  echo "ERRO: binário não encontrado em $BIN_SRC"
  exit 1
fi

cp "$BIN_SRC" "$BIN_DST"

echo "==> Binário de flash gerado em: $BIN_DST"