#!/bin/bash

# ============================================================================== 
# Script para Testar a Análise Semântica (Casos de Erro) 
# ============================================================================== 
# Este script executa o compilador com a flag -i para todos os arquivos 
# em um diretório de testes que deveriam falhar. Um teste passa se o 
# compilador retornar um código de saída de erro (diferente de 0). 
# ==============================================================================

# --- CONFIGURAÇÕES --- 
COMPILER_PATH="./build/lang"  
TEST_DIR="instances/types/function"

# --- CORES PARA A SAÍDA --- 
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # Sem Cor

# --- INICIALIZAÇÃO --- 
passed_count=0
total_count=0

# Validações iniciais 
if [ ! -x "$COMPILER_PATH" ]; then
    echo -e "${RED}Erro: Compilador não encontrado ou não é executável em '$COMPILER_PATH'.${NC}"
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}Erro: Diretório de testes '$TEST_DIR' não encontrado.${NC}"
    exit 1
fi

# --- EXECUÇÃO DOS TESTES --- 
echo -e "${YELLOW}Iniciando testes de erro semântico para a pasta '$TEST_DIR'...${NC}"
echo "----------------------------------------------------------------------"

for test_file in "$TEST_DIR"/*; do
    if [ -f "$test_file" ]; then
        ((total_count++))

        # Executa o compilador e captura a saída e o código de saída
        output=$("$COMPILER_PATH" -i "$test_file")
        exit_code=$?

        echo "Arquivo: $test_file"
        echo "Saída do compilador:"
        echo "$output"
        echo "Código de saída: $exit_code"

        # Para estes testes, um CÓDIGO DE SAÍDA IGUAL A 0 significa SUCESSO.
        if [ $exit_code -eq 0 ]; then
            ((passed_count++))
            echo -e "${GREEN}✔ O compilador aceitou corretamente o arquivo.${NC}"
        else
            echo -e "${RED}✖ O compilador rejeitou incorretamente o arquivo.${NC}"
        fi
        echo "----------------------------------------------------------------------"
    fi
done

# --- SUMÁRIO --- 
echo -e "${YELLOW}Testes finalizados.${NC}\n"
echo -e "Resumo: ${GREEN}$passed_count${NC} de ${YELLOW}$total_count${NC} testes foram validados com sucesso."
echo ""

# Mensagem final 
if [ "$passed_count" -eq "$total_count" ]; then
    echo -e "${GREEN}🎉 Perfeito! Todos os programas inválidos foram corretamente rejeitados.${NC}"
else
    echo -e "${RED}⚠️ Atenção: O compilador aceitou alguns programas que deveriam ter falhado.${NC}"
fi
