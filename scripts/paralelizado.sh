#!/usr/bin/env bash
# Uso: bash scripts/paralelizado.sh [arquivo_de_dados] [threads]
set -euo pipefail

PASTA_PROJETO="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ARQUIVO_DADOS="${1:-$PASTA_PROJETO/dados_exemplo.txt}"
THREADS="${2:-${OMP_NUM_THREADS:-4}}"
MOSTRAR_THREADS="${MOSTRAR_THREADS:-0}"
TABELA="${TABELA:-1}"

if (( $# > 2 )); then
    echo "Uso: $0 [arquivo_de_dados] [threads]" >&2
    exit 1
fi
if [[ ! -f "$ARQUIVO_DADOS" ]]; then
    echo "Arquivo de dados nao encontrado: $ARQUIVO_DADOS" >&2
    exit 1
fi
if [[ ! "$THREADS" =~ ^[1-9][0-9]*$ ]]; then
    echo "Threads deve ser um inteiro positivo." >&2
    exit 1
fi
if [[ ! "$MOSTRAR_THREADS:$TABELA" =~ ^[01]:[01]$ ]]; then
    echo "MOSTRAR_THREADS e TABELA devem ser 0 ou 1." >&2
    exit 1
fi

make -C "$PASTA_PROJETO/codigos" paralelizado
OPCOES=()
if [[ "$MOSTRAR_THREADS" == 1 ]]; then OPCOES+=(--mostrar-threads); fi
if [[ "$TABELA" == 0 ]]; then OPCOES+=(--sem-tabela); fi

ARQUIVO_RESULTADO="$(mktemp "$PASTA_PROJETO/resultados/paralelizado_${THREADS}threads_$(date +%Y%m%d_%H%M%S)_XXXXXX.txt")"
OMP_NUM_THREADS="$THREADS" OMP_DYNAMIC=FALSE \
    "$PASTA_PROJETO/resultados/paralelizado" "$ARQUIVO_DADOS" "${OPCOES[@]}" 2>&1 | tee "$ARQUIVO_RESULTADO"
printf '\nResultado salvo em: %s\n' "$ARQUIVO_RESULTADO"
