# Define cores para a saída
GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NC=$(tput sgr0) # Sem Cor
BLUE=$(tput setaf 4)
BOLD=$(tput bold)
NORMAL=$(tput sgr0)

# --- Definição dos Casos de Teste ---

DESCRIPTIONS=(
  "Teste de Sintaxe 1: Definição simples de 'data'"
  "Teste de Sintaxe 2: Usando o novo tipo em uma declaração"
  "Teste de Sintaxe 3: Usando o operador 'new'"
  "Teste de Sintaxe 4: Acesso a campo como l-value (atribuição)"
  "Teste de Sintaxe 5: Acesso a campo como expressão (leitura)"
  "Teste de Sintaxe 6: Teste combinado completo"
  "Teste de Sintaxe 7: Rejeição de nome de tipo inválido"
)

CODES=(
  "data Point { x :: Int; } fun main() {}"
  "data Point {} fun main() { p :: Point; }"
  "data Point {} fun main() { p :: Point; p = new Point; }"
  "data Point { x :: Int; } fun main() { p :: Point; p.x = 10; }"
  "data Point { x :: Int; } fun main() { p :: Point; print p.x; }"
  "data Point { x :: Int; } fun main() { p :: Point; p = new Point; p.x = 100; print p.x; }"
  "data point {} fun main() {}"
)

# Todos os testes usam a flag -syn
FLAGS=( "-syn" "-syn" "-syn" "-syn" "-syn" "-syn" "-syn" )

# Saídas esperadas para cada teste
EXPECTED_OUTPUTS=(
  "accept"
  "accept"
  "accept"
  "accept"
  "accept"
  "accept"
  "reject"
)

# --- Execução dos Testes ---

echo "${BOLD}--- Iniciando Testes de Sintaxe para 'data', 'new' e '.' ---${NORMAL}"
echo ""

PASSED_COUNT=0
FAILED_COUNT=0
TOTAL_TESTS=${#DESCRIPTIONS[@]}
COMPILER_PATH="./build/lang"
TEMP_DIR="temp_data_tests"

# Garante que o compilador está construído
echo "Verificando o build..."
mkdir -p build
cd build
make > /dev/null # Suprime a saída do make se não houver erros
cd ..
echo "Build verificado."
echo ""

rm -rf "$TEMP_DIR"
mkdir "$TEMP_DIR"

for i in "${!DESCRIPTIONS[@]}"; do
  DESCRIPTION="${DESCRIPTIONS[$i]}"
  CODE="${CODES[$i]}"
  FLAG="${FLAGS[$i]}"
  EXPECTED_OUTPUT_RAW="${EXPECTED_OUTPUTS[$i]}"
  TEST_FILE="$TEMP_DIR/test_$((i+1)).lang"

  echo "${BLUE}===== Executando: $DESCRIPTION =====${NC}"

  echo -e "$CODE" > "$TEST_FILE"
  
  ACTUAL_OUTPUT=$($COMPILER_PATH $FLAG "$TEST_FILE" 2> /dev/null | tr -d '[:space:]')
  
  if [ "$ACTUAL_OUTPUT" == "$EXPECTED_OUTPUT_RAW" ]; then
    echo "${BOLD}Veredito: ${GREEN}[PASS]${NORMAL}"
    ((PASSED_COUNT++))
  else
    echo "${BOLD}Veredito: ${RED}[FAIL]${NORMAL}"
    echo "       -> Esperado: '$EXPECTED_OUTPUT_RAW'"
    echo "       -> Recebido: '$ACTUAL_OUTPUT'"
    ((FAILED_COUNT++))
  fi
  echo "" 
done

# --- Resumo ---
echo ""
echo "${BOLD}--- Resumo dos Testes de Sintaxe ---${NORMAL}"
if [ "$FAILED_COUNT" -eq 0 ]; then
  echo "${GREEN}Todos os $TOTAL_TESTS testes passaram com sucesso!${NC}"
else
  echo "${RED}$FAILED_COUNT de $TOTAL_TESTS testes falharam.${NC}"
fi
echo "------------------------------------"

rm -rf "$TEMP_DIR"