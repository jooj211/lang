#!/bin/bash

# ==============================================================================
# Script para Testar a Análise Sintática do Compilador Lang
# ==============================================================================
# Este script executa o compilador com a flag -syn para todos os arquivos
# em um diretório específico e verifica se a saída é "accept".
# ==============================================================================

# --- CONFIGURAÇÕES ---
# Altere esta variável para o caminho correto do seu executável compilado.
COMPILER_PATH="../build/lang" 
# Diretório contendo os casos de teste que devem passar.
TEST_DIR="../instances/sintaxe/certo"


# --- CORES PARA A SAÍDA ---
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # Sem Cor

# --- INICIALIZAÇÃO ---
passed_count=0
total_count=0

# Verifica se o compilador existe e é executável
if [ ! -x "$COMPILER_PATH" ]; then
    echo -e "${RED}Erro: Compilador não encontrado ou não é executável em '$COMPILER_PATH'.${NC}"
    echo "Por favor, compile o projeto e/ou ajuste a variável COMPILER_PATH no script."
    exit 1
fi

# Verifica se o diretório de testes existe
if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}Erro: Diretório de testes '$TEST_DIR' não encontrado.${NC}"
    exit 1
fi

# --- EXECUÇÃO DOS TESTES ---
echo -e "${YELLOW}Iniciando testes de sintaxe para a pasta '$TEST_DIR'...${NC}"
echo "------------------------------------------------------------------"

# Itera sobre todos os arquivos no diretório de teste
for test_file in "$TEST_DIR"/*; do
    # Garante que estamos processando apenas arquivos
    if [ -f "$test_file" ]; then
        ((total_count++))

        # Executa o compilador com a flag -syn e captura a saída
        output=$("$COMPILER_PATH" -syn "$test_file")

        # Verifica se a saída é exatamente "accept"
        if [ "$output" = "accept" ]; then
            ((passed_count++))
            # Imprime o resultado formatado e em cores
            printf "${GREEN}%-10s${NC} ✔ %s\n" "[PASSOU]" "$test_file"
        else
            printf "${RED}%-10s${NC} ✖ %s\n" "[FALHOU]" "$test_file"
            echo -e "    └─ Esperado: 'accept', Recebido: '${RED}$output${NC}'"
        fi
    fi
done

# --- SUMÁRIO ---
echo "------------------------------------------------------------------"
echo -e "${YELLOW}Testes finalizados.${NC}"
echo ""
echo -e "Resumo: ${GREEN}$passed_count${NC} de ${YELLOW}$total_count${NC} testes passaram."
echo ""

# Mensagem final de sucesso ou falha
if [ "$passed_count" -eq "$total_count" ]; then
    echo -e "${GREEN}🎉 Todos os testes foram aprovados com sucesso!${NC}"
else
    echo -e "${RED}⚠️ Alguns testes falharam.${NC}"
fi
