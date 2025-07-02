#!/usr/bin/env bash
# Define cores para a saída
BLUE=$(tput setaf 4)
BOLD=$(tput bold)
NORMAL=$(tput sgr0)
NC=$(tput sgr0)

# --- Definição dos Casos de Teste (Extraídos de interpreter_tests.cpp) ---

DESCRIPTIONS=(
  "Interpreter Test 1 (Soma simples)"
  "Interpreter Test 2 (Operadores e Tipos)"
  "Interpreter Test 3 (Condicionais)"
  "Interpreter Test 4 (Iterate)"
  "Interpreter Test 5 (Escopo)"
  "Interpreter Test 6 (Múltiplos reads)"
  "Inferência de tipos – igualdade"
  "Inferência de tipos – booleano"
  "Inferência de tipos – float"
  "Inferência de tipos – resto (%)"
  "Inferência de tipos – multiplicação"
  "Inferência de tipos – negação lógica"
  "Inferência de tipos – null"
  "Inferência de tipos – subtração"
  "Inferência de tipos – constante true"
  "Chamada com Múltiplos Retornos e Atribuição"
)

CODES=(
  # 0 ─ Soma simples
  "main(): Int {\n    a :: Int; \n    b :: Int; \n    soma :: Int;\n    a = 5;\n    b = 20;\n    soma = a + b;\n    print soma;\n    return 0;\n}"
  # 1 ─ Operadores e Tipos
  "main(): Int {\n    a :: Int;\n    b :: Int;\n    c :: Int;\n    a = 20;\n    b = 5;\n    c = a + b;\n    print c;\n    c = a - b;\n    print c;\n    c = a * b;\n    print c;\n    c = a / b;\n    print c;\n    c = a % b;\n    print c;\n    c = -a;\n    print c;\n\n    f1 :: Float;\n    f2 :: Float;\n    f3 :: Float;\n    f1 = 10.5;\n    f2 = 2.0;\n    f3 = f1 + f2;\n    print f3;\n    f3 = f1 - f2;\n    print f3;\n    f3 = f1 * f2;\n    print f3;\n    f3 = f1 / f2;\n    print f3;\n    f3 = -f1;\n    print f3;\n\n    t :: Bool;\n    f :: Bool;\n    r :: Bool;\n    t = true;\n    f = false;\n    r = !t;\n    print r;\n    r = !f;\n    print r;\n\n    ch :: Char;\n    ch = 'A';\n    print ch;\n    return 0;\n}"
  # 2 ─ Condicionais
  "main(): Int {\n    x :: Int;\n    y :: Int;\n    x = 5;\n    y = 10;\n    if (x < y) {\n        print x;\n    } else {\n        print y;\n    }\n    if (y < x) {\n        print x;\n    } else if (y % 2 != 1) {\n        print y;\n    }\n    if (x == 5) {\n        print 100;\n    }\n    if (x == 10 && y == 10) {\n        print 200;\n    }\n    return 0;\n}"
  # 3 ─ Iterate
  "main(): Int {\n    print 1;\n    iterate (3) {\n        print 2;\n    }\n    print 3;\n\n    print 4;\n    iterate (j : 2) {\n        print j;\n    }\n    print 5; \n    return 0;\n}"
  # 4 ─ Escopo
  "main(): Int {\n    outer_var :: Int;\n    outer_var = 100;\n    print outer_var;\n    if (true) {\n        inner_var_if :: Int;\n        inner_var_if = 200;\n        print inner_var_if;\n        print outer_var;\n    }\n    { \n        another_inner_var :: Int;\n        another_inner_var = 300;\n        print another_inner_var;\n    }\n    return 0;\n}"
  # 5 ─ Múltiplos reads
  "main(): Int {\n    idade :: Int;\n    altura :: Float;\n    aceita :: Bool;\n    inicial :: Char;\n    read idade;\n    print idade;\n    read altura;\n    print altura;\n    read aceita;\n    print aceita;\n    read inicial;\n    print inicial;\n    return 0;\n}"
  # 6 ─ Igualdade
  "main() {\n    x = 10 == 10;\n    print x;\n}"
  # 7 ─ Booleano
  "main() {\n    x = false;\n    print x;\n}"
  # 8 ─ Float
  "main() {\n    x = 1.0;\n    print x;\n}"
  # 9 ─ Resto
  "main() {\n    x = 1 % 2;\n    print x;\n}"
  # 10 ─ Multiplicação
  "main() {\n    x = 1 * 2;\n    print x;\n}"
  # 11 ─ Negação lógica
  "main() {\n    x = !(10 < 10);\n    print x;\n}"
  # 12 ─ Null
  "main() {\n    x = null;\n    print x;\n}"
  # 13 ─ Subtração
  "main() {\n    x = 1 - 2;\n    print x;\n}"
  # 14 ─ Constante true
  "main() {\n    x = true;\n    print x;\n}"
  # 15 ─ Múltiplos retornos / atribuição
  "data Ponto {\n   x :: Float;\n   y :: Float;\n}\n\nf(d :: Int ) : Int, Char{\n    return 0, '\\n';\n}\n\nmain(x :: Ponto){\n   print x.x;\n   z = f(0)[0] + 2;\n}"
)

# Entradas para os testes com 'read'
INPUT_DATA=(
  ""  ""  ""  ""  ""  "25\n1.75\ntrue\nZ"
  ""  ""  ""  ""  ""  ""  ""  ""  ""  ""
)

# Caminho para o interpretador
COMPILER_PATH="./build/lang"

TEMP_FILE=$(mktemp /tmp/lang_test_XXXX.lang)

for idx in "${!CODES[@]}"; do
  DESCRIPTION=${DESCRIPTIONS[$idx]}
  CODE=${CODES[$idx]}
  INPUT=${INPUT_DATA[$idx]}

  echo "${BLUE}==============================${NC}"
  echo "${BOLD}Teste $((idx + 1)) – $DESCRIPTION${NORMAL}"
  echo "${BLUE}==============================${NC}"

  # Grava código no arquivo temporário
  echo -e "$CODE" > "$TEMP_FILE"

  echo "${BOLD}Código:${NORMAL}"
  cat "$TEMP_FILE"
  echo ""

  echo "${BOLD}Saída do Interpretador:${NORMAL}"
  echo "------------------------------------------"
  if [ -n "$INPUT" ]; then
    echo -e "$INPUT" | $COMPILER_PATH -i "$TEMP_FILE"
  else
    $COMPILER_PATH -i "$TEMP_FILE"
  fi
  echo "------------------------------------------"
  echo ""
done

rm "$TEMP_FILE"
