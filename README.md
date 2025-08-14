# README — Compilador da Linguagem **LAN**

## 📌 Sobre
Este projeto é um compilador para a linguagem **LAN**, implementado em C++ utilizando **Flex** e **Bison** para análise léxica e sintática, além de módulos para análise semântica, interpretação e geração de código em diferentes formatos.

O compilador foi desenvolvido como parte da disciplina de **Construção de Compiladores** e segue a especificação definida no documento `lang.pdf`.

---

## ⚙️ Funcionalidades Implementadas
- **Análise Léxica** via Flex
- **Análise Sintática** via Bison
- **Análise Semântica** completa (tipos, variáveis, funções, estruturas `data`, etc.)
- **Interpretação** direta de código-fonte `.lan`
- **Geração de Código Python** (`-src`)
- **Geração de Código Jasmin** (`-gen`)
- Suporte a:
  - Tipos primitivos (`Int`, `Bool`, `Char`)
  - Tipos compostos via `data`
  - Arrays e matrizes
  - Operadores aritméticos, relacionais e lógicos
  - Funções com múltiplos retornos
  - Controle de fluxo (`if`, `else`, `while`, `iterate`)
  - Criação de novos objetos e atribuições
  - Entrada e saída (`print`, `read`)

---

## 📂 Estrutura do Projeto
```plaintext
.
├── src/               # Código-fonte do compilador
├── instances/         # Casos de teste
│   ├── sintaxe/       # Testes de análise sintática
│   ├── semantica/     # Testes de análise semântica
│   ├── ...            # Outros conjuntos de testes
├── CMakeLists.txt     # Configuração de build
└── README.md          # Este arquivo
```

---

## 🚀 Como Compilar
```bash
mkdir build
cd build
cmake ..
make
```

---

## 🖥️ Modo de Uso

### 1. Interpretar um programa
Executa diretamente o código `.lan`:
```bash
./lang -i caminho/para/programa.lan
```

### 2. Gerar código Python
Gera um arquivo `.py` equivalente ao programa:
```bash
./lang -src -o saida.py caminho/para/programa.lan
python saida.py
```

### 3. Gerar código Jasmin
Gera um arquivo `.j` (Jasmin Assembly):
```bash
./lang -gen -o saida.j caminho/para/programa.lan
```
Para compilar o `.j` em bytecode Java:
```bash
java -cp "/usr/share/java/jasmin-sable-2.5.0.jar:/usr/share/java/java-cup-0.11b-runtime.jar" jasmin.Main saida.j
java Main
```

---

## 🧪 Rodando os Testes
Os diretórios `instances/` contêm casos de teste para cada fase do compilador.
Por exemplo, para rodar um teste sintático:
```bash
./lang -i instances/sintaxe/certo/exemplo1.lan
```
Ou para gerar código Python a partir dele:
```bash
./lang -src -o out.py instances/sintaxe/certo/exemplo1.lan
python out.py
```

---

## 📄 Licença
Uso educacional — Trabalho acadêmico.
