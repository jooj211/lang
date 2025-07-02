#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

#include "typecheck/TypeChecker.hpp"
#include "ast/ProgramNode.hpp"
#include "interpreter/Interpreter.hpp"

// Símbolos gerados por Flex/Bison
extern FILE *yyin;
extern int yyparse();
extern int yydebug;
extern ProgramNode *ast_root;
extern bool parse_error_detected;

// Enum para representar as ações do compilador
enum class CompilerAction
{
    SYNTACTIC_ANALYSIS, // Para a flag -syn
    INTERPRET           // Para a flag -i
};

// Função de ajuda atualizada
static void usage(const char *exe)
{
    std::cerr << "Uso: " << exe << " <diretiva> <arquivo.lang>\n\n"
              << "Diretivas disponíveis:\n"
              << "  -syn   Executa apenas a análise sintática e retorna 'accept' ou 'reject'.\n"
              << "  -i     Interpreta o programa após a checagem de tipos.\n";
}

int main(int argc, char *argv[])
{
    /* ---------- Bloco de Argumentos de Teste (mantido) ---------- */
    // Para testar, basta descomentar a linha da diretiva desejada.

    // Teste para -syn (análise sintática)
    /* const char *fake_argv[] = {"lang", "-i", "test.lang"}; */

    // Teste para -i (interpretação)
    const char *fake_argv[] = {"lang", "-i", "test.lang"};

    argc = 3;
    argv = const_cast<char **>(fake_argv);
    /* ------------------------------------------------------------- */

    yydebug = 1; // Mude para 1 para ver o traço do parser

    /* ---------- 1. Processa argumentos ---------- */
    if (argc < 3)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    CompilerAction action;
    if (strcmp(argv[1], "-syn") == 0)
    {
        action = CompilerAction::SYNTACTIC_ANALYSIS;
    }
    else if (strcmp(argv[1], "-i") == 0)
    {
        action = CompilerAction::INTERPRET;
    }
    else
    {
        std::cerr << "Erro: Diretiva '" << argv[1] << "' desconhecida.\n\n";
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string filename = argv[2];

    /* ---------- 2. Abre arquivo ---------- */
    yyin = std::fopen(filename.c_str(), "r");
    if (!yyin)
    {
        std::perror(("Erro ao abrir " + filename).c_str());
        return EXIT_FAILURE;
    }

    /* ---------- 3. Análise Sintática (Parser) ------------ */
    if (yyparse() != 0 || !ast_root || parse_error_detected)
    {
        // Se a sintaxe estiver incorreta, o resultado é 'reject'.
        if (action == CompilerAction::SYNTACTIC_ANALYSIS)
        {
            std::cout << "reject" << std::endl;
        }
        return EXIT_FAILURE;
    }

    /* ---------- 4. Executa a ação solicitada ---------- */

    // Ação: Apenas análise sintática
    if (action == CompilerAction::SYNTACTIC_ANALYSIS)
    {
        std::cout << "accept" << std::endl;
        return EXIT_SUCCESS; // Encerra com sucesso após imprimir 'accept'
    }

    // Ação: Checagem de tipos e interpretação
    if (action == CompilerAction::INTERPRET)
    {
        try
        {
            TypeChecker tc;
            tc.check(ast_root);

            Interpreter itp;
            itp.interpret(ast_root);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Erro: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}