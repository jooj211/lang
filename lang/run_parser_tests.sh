# Define cores para a saída
GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NC=$(tput sgr0) # Sem Cor
BLUE=$(tput setaf 4)
BOLD=$(tput bold)
NORMAL=$(tput sgr0)

# --- Definição dos Casos de Teste (Extraídos de parser_tests.cpp) ---

# Nomes dos testes (baseado nos nomes dos arquivos originais)
DESCRIPTIONS=(
  "Parser Test 1 (Pessoa, soma, main)"
  "Parser Test 2 (Apenas comentários)"
  "Parser Test 3 (Completo com abstract data, new, etc.)"
  "Parser Test 4 (Fatorial e Múltiplas Funções)"
  "Parser Test 5 (Escopo aninhado)"
  "Parser Test 6 (Múltiplos reads)"
)

# Códigos-fonte dos testes
# Nota: A sintaxe R"()" do C++ foi convertida para strings bash.
CODES=(
  "data Pessoa {\n    nome :: Char[];\n    idade :: Int;\n}\n\nsoma(a :: Int, b :: Int): Int {\n    return a + b;\n}\n\nmain(): Int {\n    var1 :: Int;\n    var1 = soma(5, 10)[0];\n    if (var1 > 10) {\n        print var1;\n    } else {\n        iterate( i: 0 < 5 ){\n            print i;\n        }\n    }\n    return var1;\n}"
  "-- aqui tem apenas comentários\n\n{- São só delírios! -}"
  "abstract data Pessoa {\n    nome :: Char[];\n    idade :: Int;\n    altura :: Float;\n    ativo :: Bool;\n\n    add(pessoa1 :: Pessoa, pessoa2 :: Pessoa) : Pessoa\n    {\n        p3 :: Pessoa;\n        p3 = new Pessoa;\n        p3.idade = pessoa1.idade + pessoa2.idade;\n\n        return p3; \n    }\n}\n\ndata Livro {\n    titulo :: Char[];\n    paginas :: Int;\n}\n\nsoma(a :: Int, b :: Int): Int {\n    return a + b;\n}\n\nmultiplica(a :: Int, b :: Int): Int, Int {\n    return a * b;\n}\n\nimprime_pessoa(p :: Pessoa) {\n    print p.nome[0]; \n    print p.idade;\n    print p.altura;\n    print p.ativo;\n}\n\nmain(): Int {\n    var1 :: Int;\n    resultado :: Int;\n    p1 :: Pessoa;\n    l1 :: Livro;\n\n    var1 = soma(5, 10)[0];\n    multiplica(var1, 3)<resultado>;\n\n    if (resultado > 10) {\n        print resultado;\n    } else {\n        iterate(i: 0 < 5){\n            print i;\n        }\n    }\n\n    p1.nome = new Char[50];\n    p1.idade = 25;\n    p1.altura = 1.75;\n    p1.ativo = true;\n\n    imprime_pessoa(p1);\n\n    return resultado;\n}"
  "main()\n{\n    a :: Int; \n    a = 10; \n    a = 10 + 20; \n    if (!(!p && !q))\n    {\n        print fat(5)[0];\n    }\n\n    print fat(10)[0];\n}\n\nfat (num :: Int) : Int{\n    if (num < 1)\n        return 1 ;\n    else\n        return num*fat(num-1)[0];\n}\n\ndivmod(num :: Int, div :: Int) : Int, Int {\n    q = num / div ;\n    r = num % div ;\n    return q, r ;\n}"
  "main(): Int {\n    outer_var :: Int;\n    outer_var = 100;\n    print outer_var;\n\n    if (true) {\n        inner_var_if :: Int;\n        inner_var_if = 200;\n        print inner_var_if;\n        print outer_var;\n    }\n\n    { \n        another_inner_var :: Int;\n        another_inner_var = 300;\n        print another_inner_var;\n    }\n\n    return 0;\n}"
  "main(): Int {\n    idade :: Int;\n    altura :: Float;\n    aceita :: Bool;\n    inicial :: Char;\n\n    read idade;\n    print idade;\n\n    read altura;\n    print altura;\n\n    read aceita;\n    print aceita;\n\n    read inicial;\n    print inicial;\n\n    return 0;\n}"
)

# --- Execução dos Testes de Parser ---

echo "${BOLD}--- Iniciando Testes de Parser (-syn) ---${NORMAL}"
echo ""

PASSED_COUNT=0
FAILED_COUNT=0
TOTAL_TESTS=${#DESCRIPTIONS[@]}
COMPILER_PATH="./build/lang"
TEMP_FILE="temp_parser_test.lang"
EXPECTED_OUTPUT="accept"

# Garante que o compilador está construído
echo "Verificando o build..."
mkdir -p build
cd build
make
cd ..
echo ""

for i in "${!DESCRIPTIONS[@]}"; do
  DESCRIPTION="${DESCRIPTIONS[$i]}"
  CODE="${CODES[$i]}"
  
  echo "${BLUE}===== Executando: $DESCRIPTION =====${NC}"
  
  # Cria o arquivo de teste temporário
  echo -e "$CODE" > "$TEMP_FILE"
  
  # Executa o compilador com a flag -syn e captura a saída
  ACTUAL_OUTPUT=$($COMPILER_PATH -syn "$TEMP_FILE" | tr -d "[:space:]")
  
  # Compara a saída
  if [ "$ACTUAL_OUTPUT" == "$EXPECTED_OUTPUT" ]; then
    echo "${BOLD}Veredito: ${GREEN}[PASS]${NORMAL}"
    ((PASSED_COUNT++))
  else
    echo "${BOLD}Veredito: ${RED}[FAIL]${NORMAL}"
    echo "       -> Saída recebida: "$ACTUAL_OUTPUT""
    # Mostra a saída de erro do compilador se houver
    $COMPILER_PATH -syn "$TEMP_FILE" > /dev/null
    ((FAILED_COUNT++))
  fi
  echo "" 
done

# --- Resumo ---
echo ""
echo "${BOLD}--- Resumo dos Testes de Parser ---${NORMAL}"
if [ "$FAILED_COUNT" -eq 0 ]; then
  echo "${GREEN}Todos os $TOTAL_TESTS testes passaram com sucesso!${NC}"
else
  echo "${RED}$FAILED_COUNT de $TOTAL_TESTS testes falharam.${NC}"
fi
echo "-------------------------------------"

# Limpa o arquivo de teste
rm "$TEMP_FILE"