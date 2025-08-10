#!/bin/bash

#-----------------------------------------------------------------------
# Script para testar o analisador sintático (`./build/lang`).
#
# Argumentos:
#   -c: Modo "correto". Roda os testes em 'instances/sintaxe/certo'
#       e espera 'accept' para passar.
#   -f: Modo "falha". Roda os testes em 'instances/sintaxe/errado'
#       e espera 'reject' para passar.
#
# Uso:
#   ./run_tests.sh -c
#   ./run_tests.sh -f
#-----------------------------------------------------------------------

# --- Configuração ---
EXECUTABLE="./build/lang"

# --- Cores para o output ---
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # Sem Cor

# --- Validação dos Argumentos ---
if [[ "$1" != "-c" && "$1" != "-f" ]]; then
    echo "Erro: Parâmetro inválido."
    echo "Uso: $0 [-c|-f]"
    echo "  -c: Testa casos de sucesso (espera 'accept')."
    echo "  -f: Testa casos de falha (espera 'reject')."
    exit 1
fi

# --- Define as variáveis de teste com base no modo ---
MODE="$1"
if [ "$MODE" = "-c" ]; then
    TEST_DIR="instances/sintaxe/certo"
    EXPECTED_OUTPUT="accept"
    FAIL_OUTPUT="reject"
else
    TEST_DIR="instances/sintaxe/errado"
    EXPECTED_OUTPUT="reject"
    FAIL_OUTPUT="accept"
fi

# --- Verificações de Ambiente ---
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Erro: Executável '$EXECUTABLE' não encontrado.${NC}"
    exit 1
fi

if [ ! -x "$EXECUTABLE" ]; then
    echo -e "${RED}Erro: '$EXECUTABLE' não tem permissão de execução.${NC}"
    echo -e "Dica: Rode ${YELLOW}chmod +x $EXECUTABLE${NC}"
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}Erro: Diretório de testes '$TEST_DIR' não encontrado.${NC}"
    exit 1
fi

# --- Execução dos Testes ---
FAIL_COUNT=0
PASS_COUNT=0

if [ "$MODE" = "-c" ]; then
    echo "Modo de Teste: Correto (-c). Rodando em '$TEST_DIR' e esperando 'accept'."
else
    echo "Modo de Teste: Falha (-f). Rodando em '$TEST_DIR' e esperando 'reject'."
fi

echo "-----------------------------------------------------"

# Itera sobre todos os arquivos no diretório de teste
for file in "$TEST_DIR"/*; do
    # Garante que é um arquivo antes de processar
    if [ -f "$file" ]; then
        # Executa o programa com o flag -syn, passando o arquivo como argumento
        # e captura a saída (stdout) na variável 'result'
        result=$( "$EXECUTABLE" -syn "$file" )
        filename=$(basename "$file")

        if [ "$result" = "$EXPECTED_OUTPUT" ]; then
            echo -e "${GREEN}[PASS]${NC} $filename"
            ((PASS_COUNT++))
        else
            echo -e "${RED}[FAIL]${NC} $filename (Esperado: '$EXPECTED_OUTPUT', Recebido: '$result')"
            ((FAIL_COUNT++))
        fi
    fi
done

echo "-----------------------------------------------------"
echo -e "Testes finalizados. Resultados: ${GREEN}${PASS_COUNT} passaram${NC}, ${RED}${FAIL_COUNT} falharam${NC}."

# Retorna um status de saída diferente de zero se algum teste falhar
if [ "$FAIL_COUNT" -gt 0 ]; then
    exit 1
fi

exit 0
