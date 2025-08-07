# Define cores para a saída
GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NC=$(tput sgr0) # Sem Cor
BLUE=$(tput setaf 4)
BOLD=$(tput bold)
NORMAL=$(tput sgr0)

# --- Definição dos Casos de Teste ---

DESCRIPTIONS=(
  "Teste 1: Expressão Aritmética Simples"
  "Teste 2: Expressão Aritmética com Precedência"
  "Teste 3: Declaração e Atribuição Simples"
  "Teste 4: Controle de Fluxo (if-then-else) e Lógica"
  "Teste 5: Loop Iterate Simples"
  "Teste 6: Loop Iterate com Variável de Escopo"
  "Teste 7: Literais Float e Char"
  "Teste 8: Comando Read (Input simulado)"
  "Teste 9: Sintaxe de Função Vazia"
  "Teste 10: Chamada de Procedimento"
  "Teste 11: Chamada de Função com Retorno"
  "Teste 12: Passagem de Parâmetros para Função"
  "Teste 13: Atribuição de Múltiplos Retornos"
  "Teste 14: Sintaxe de Definição de 'data'"
)

CODES=(
  "fun main() { print 10 + 5; }"
  "fun main() { print 10 * (2 + 5); }"
  "fun main() { x :: Int;\\nx = 21;\\nprint x + x; }"
  "fun main() { x :: Int;\\ny :: Bool;\\nx = 10;\\ny = x < 20;\\nif (y && true) { print 1; } else { print 0; } }"
  "fun main() { i :: Int; i = 0; iterate (5) { i = i + 1; } print i; }"
  "fun main() { i :: Int; i = 10; iterate (i: 3) { print i; } print i; }"
  "fun main() { f :: Float;\\nc :: Char;\\nf = 1.5;\\nc = 'z';\\nprint f;\\nprint c; }"
  "fun main() { val :: Int;\\nread val;\\nprint val + 5; }"
  "fun my_func() {}"
  "fun my_print() { print 42; }\\nfun main() { my_print(); }"
  "fun get_num() : Int { return 99; }\\nfun main() { print get_num()[0]; }"
  "fun add(a :: Int, b :: Int) { print a + b; }\\nfun main() { add(10, 32); }"
  "fun get_vals() : Int, Int { return 120, 5; }\\nfun main() { a :: Int; r :: Int; get_vals()<a,r>; print a; print r; }"
  "data Point { x :: Int; y :: Int; }\\nfun main() {}"
)

FLAGS=(
  "-i" "-i" "-i" "-i" "-i" "-i" "-i" "-i" "-syn" "-i" "-i" "-i" "-i" "-syn"
)

EXPECTED_OUTPUTS=(
  "15"
  "70"
  "42"
  "1"
  "5"
  $'0\n1\n2\n10'
  $'1.5\nz'
  "104"
  "accept"
  "42"
  "99"
  "42"
  $'120\n5'
  "accept"
)

# --- Execução dos Testes ---

echo "${BOLD}--- Iniciando Testes de Regressão ---${NORMAL}"
echo ""

PASSED_COUNT=0
FAILED_COUNT=0
TOTAL_TESTS=${#DESCRIPTIONS[@]}
COMPILER_PATH="./build/lang"
TEST_DIR="regression_tests"

# Garante que o diretório de testes está limpo
rm -rf "$TEST_DIR"
mkdir "$TEST_DIR"

# Loop principal
for i in "${!DESCRIPTIONS[@]}"; do
  DESCRIPTION="${DESCRIPTIONS[$i]}"
  CODE="${CODES[$i]}"
  FLAG="${FLAGS[$i]}"
  EXPECTED_OUTPUT_RAW="${EXPECTED_OUTPUTS[$i]}"
  TEST_FILE="$TEST_DIR/test_$((i+1)).lang"

  echo "${BLUE}===== Executando Teste $((i+1)): $DESCRIPTION =====${NC}"

  # Cria o arquivo com o código-fonte
  echo -e "$CODE" > "$TEST_FILE"
  
  # Lógica de execução para o teste
  if [[ "$DESCRIPTION" == "Teste 8: Comando Read (Input simulado)" ]]; then
    ACTUAL_OUTPUT=$(echo "99" | $COMPILER_PATH $FLAG "$TEST_FILE")
  else
    ACTUAL_OUTPUT=$($COMPILER_PATH $FLAG "$TEST_FILE")
  fi
  
  # Remove a quebra de linha final que o shell pode adicionar
  ACTUAL_OUTPUT=$(echo -n "$ACTUAL_OUTPUT")
  EXPECTED_OUTPUT_PROCESSED=$(echo -e "$EXPECTED_OUTPUT_RAW")

  # Compara a saída real com a esperada
  if [ "$ACTUAL_OUTPUT" == "$EXPECTED_OUTPUT_PROCESSED" ]; then
    echo "${BOLD}Veredito: ${GREEN}[PASS]${NORMAL}"
    ((PASSED_COUNT++))
  else
    echo "${BOLD}Veredito: ${RED}[FAIL]${NORMAL}"
    echo "       -> Esperado: \"$EXPECTED_OUTPUT_PROCESSED\""
    echo "       -> Recebido: \"$ACTUAL_OUTPUT\""
    ((FAILED_COUNT++))
  fi
  echo "" 
done

# --- Resumo ---
echo ""
echo "${BOLD}--- Resumo dos Testes ---${NORMAL}"
if [ "$FAILED_COUNT" -eq 0 ]; then
  echo "${GREEN}Todos os $TOTAL_TESTS testes passaram com sucesso!${NC}"
else
  echo "${RED}$FAILED_COUNT de $TOTAL_TESTS testes falharam.${NC}"
fi
echo "-------------------------"

# Limpa o diretório de testes
rm -rf "$TEST_DIR"