#!/bin/bash

# ==============================================================================
# Script para Testar a Análise Semântica (Casos de Erro)
# ==============================================================================
# Este script executa o compilador com a flag -i para todos os arquivos
# em um diretório de testes que deveriam falhar. Um teste passa se o
# compilador retornar um código de saída de erro (diferente de 0).
# ==============================================================================

# --- CONFIGURAÇÕES ---
# Altere esta variável para o caminho correto do seu executável compilado.
COMPILER_PATH="./build/lang" 
# Diretório contendo os casos de teste que devem FALHAR.
TEST_DIR="instances/types/errado"


# --- CORES PARA A SAÍDA ---
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # Sem Cor

# --- INICIALIZAÇÃO ---
passed_count=0
total_count=0

# Validações iniciais (compilador e diretório)
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

# Itera sobre todos os arquivos no diretório de teste
for test_file in "$TEST_DIR"/*; do
    if [ -f "$test_file" ]; then
        ((total_count++))

        # Executa o compilador, redirecionando toda a saída para o "buraco negro" (/dev/null)
        # para manter a tela limpa. O que nos importa é o código de saída.
        "$COMPILER_PATH" -i "$test_file" &> /dev/null
        exit_code=$? # Captura o código de saída do último comando executado

        # Para estes testes, um CÓDIGO DE SAÍDA DIFERENTE DE 0 significa SUCESSO.
        if [ $exit_code -ne 0 ]; then
            ((passed_count++))
            printf "${GREEN}%-10s${NC} ✔ %s (Compilador rejeitou corretamente)\n" "[PASSOU]" "$test_file"
        else
            # Se o código de saída for 0, o compilador aceitou um programa inválido.
            printf "${RED}%-10s${NC} ✖ %s (Compilador aceitou incorretamente)\n" "[FALHOU]" "$test_file"
        fi
    fi
done

# --- SUMÁRIO ---
echo "----------------------------------------------------------------------"
echo -e "${YELLOW}Testes finalizados.${NC}\n"
echo -e "Resumo: ${GREEN}$passed_count${NC} de ${YELLOW}$total_count${NC} testes de erro foram validados com sucesso."
echo ""

# Mensagem final
if [ "$passed_count" -eq "$total_count" ]; then
    echo -e "${GREEN}🎉 Perfeito! Todos os programas inválidos foram corretamente rejeitados.${NC}"
else
    echo -e "${RED}⚠️ Atenção: O compilador aceitou alguns programas que deveriam ter falhado.${NC}"
fi