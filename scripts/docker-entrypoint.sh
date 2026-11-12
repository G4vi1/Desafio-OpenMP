#!/usr/bin/env bash
set -euo pipefail
cd /lab

# A montagem de resultados pode ocultar o executavel criado na imagem.
# Recompila no container para usar seu compilador e bibliotecas, fora das medicoes.
make -B -C codigos all
exec "$@"
