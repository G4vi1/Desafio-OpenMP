#!/usr/bin/env bash
# Uso: ./scripts/sequencial.sh [arquivo_de_dados]
set -euo pipefail

# Localiza o projeto mesmo quando o script e chamado de outra pasta.
PASTA_PROJETO="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ARQUIVO_DADOS="${1:-$PASTA_PROJETO/dados_exemplo.txt}"

if (( $# > 1 )); then
    echo "Uso: $0 [arquivo_de_dados]" >&2
    exit 1
fi

if [[ ! -f "$ARQUIVO_DADOS" ]]; then
    echo "Arquivo de dados nao encontrado: $ARQUIVO_DADOS" >&2
    exit 1
fi

# Compila antes de executar.
make -C "$PASTA_PROJETO/codigos"
mkdir -p "$PASTA_PROJETO/resultados"

# Cria uma saida exclusiva por execucao e exibe tambem no terminal.
ARQUIVO_RESULTADO="$(mktemp "$PASTA_PROJETO/resultados/sequencial_$(date +%Y%m%d_%H%M%S)_XXXXXX.txt")"
"$PASTA_PROJETO/codigos/sequencial" "$ARQUIVO_DADOS" 2>&1 | tee "$ARQUIVO_RESULTADO"

printf '\nResultado salvo em: %s\n' "$ARQUIVO_RESULTADO"
