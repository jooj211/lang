-----

# Compilador para a Linguagem `lang`

## Sobre o Projeto

Este repositório contém o código-fonte de um compilador completo para a `lang`, uma linguagem de programação com fins educacionais. O projeto foi desenvolvido como parte do trabalho prático da disciplina **DCC045 – Teoria dos Compiladores**.

O compilador foi implementado em C++ e abrange todas as fases clássicas de compilação:

  * Análise Léxica e Sintática  
  * Construção da Árvore Sintática Abstrata (AST)  
  * Análise Semântica e Checagem de Tipos  
  * Interpretação e Execução de Código  

## A Linguagem `lang`

A `lang` é uma linguagem imperativa e estaticamente tipada, projetada para incluir funcionalidades presentes em linguagens modernas:

  * **Tipagem Estática Forte** – valida erros antes da execução.  
  * **Inferência de Tipo** – variáveis podem ser declaradas implicitamente (ex.: `x = 10;`).  
  * **Tipos de Dados**  
      * Primitivos: `Int`, `Float`, `Char`, `Bool`, `Void`.  
      * Registros: tipos compostos definidos pelo usuário com `data`.  
      * Arrays e Matrizes: suporte a arrays multidimensionais (ex.: `Int[]`, `Transition[][]`).  
  * **Estruturas de Dados Recursivas** – listas ligadas, árvores etc.  
  * **Funções com Múltiplos Retornos** – acesso por índice (ex.: `val = f()[0];`).  
  * **Semântica de Referência** – registros e arrays são passados por referência.  
  * **Controle de Fluxo** – `if-else` e o laço `iterate`.  

## Arquitetura do Compilador

1. **Analisador Léxico e Sintático** (`/src/parser`) – construído com *Flex* e *Bison*, gera a AST.  
2. **AST** (`/src/ast`) – representação hierárquica do código-fonte.  
3. **Analisador Semântico** (`/src/typecheck`) – padrão *Visitor* para checagem de tipos (inclusive recursivos).  
4. **Interpretador** (`/src/interpreter`) – percorre a AST validada, gerenciando memória e controle de fluxo.  

## Como Compilar o Projeto

O professor deve seguir os passos abaixo **após baixar e descompactar o arquivo ZIP do projeto**:

```bash
# 1. Navegue até a pasta extraída
cd lang-compiler-main      # ajuste o nome se necessário

# 2. Crie e acesse um diretório de build
mkdir build
cd build

# 3. Gere os arquivos de build com o CMake
cmake ..

# 4. Compile o projeto
make

# O executável será criado em ./build/lang
````

> **Pré-requisitos**: CMake ≥ 3.16, compilador C++17, Flex e Bison instalados no sistema.

## Como Executar

O compilador é executado via linha de comando:

```
./build/lang <flag> <caminho_para_o_arquivo.lang>
```

### Análise Sintática (`-syn`)

Verifica apenas a sintaxe do programa:

```bash
./build/lang -syn ../instances/sintaxe/certo/01_data.lang
# Saída: accept
```

### Interpretação (`-i`)

Executa todas as fases e interpreta o programa:

```bash
./build/lang -i ../instances/semantico/correto/llist.lang
# Saída esperada:
# 6:65->66->67->68->69->70->NULL
# 5:66->67->68->69->70->NULL
```

## Testes Automatizados

Scripts inclusos garantem a correção do compilador:

```bash
# Torne os scripts executáveis (uma única vez)
chmod +x testar_sintaxe.sh
chmod +x testar_erros.sh

# Testes de sintaxe (devem aceitar)
./testar_sintaxe.sh

# Testes de erros semânticos (devem rejeitar)
./testar_erros.sh
```

---
```
