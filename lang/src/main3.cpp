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
extern ProgramNode *ast_root; // definido em parser.y

static void usage(const char *exe)
{
    std::cerr << "Uso: " << exe << " [-i <arquivo.lang>]\n"
              << "      Se nenhum arquivo for passado, lê do stdin.\n";
}

int main(int argc, char *argv[])
{
    /* ---------- Simulação de argumentos (opcional) ---------- */

    const char *fake_argv[] = {"lang", "-i", "../test.lang"};
    argc = 3;
    argv = const_cast<char **>(fake_argv);

    yydebug = 1; // coloque 1 para ver o traço LR

    /* ---------- 1. Processa argumentos ---------- */
    std::string filename;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
        {
            filename = argv[++i];
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else
        {
            filename = argv[i]; // ./lang prog.lang
        }
    }

    /* ---------- 2. Abre arquivo / stdin ---------- */
    if (!filename.empty())
    {
        yyin = std::fopen(filename.c_str(), "r");
        if (!yyin)
        {
            std::perror(("Erro ao abrir " + filename).c_str());
            return EXIT_FAILURE;
        }
    }
    else
    {
        yyin = stdin;
    }

    /* ---------- 3. Parse & build AST ------------ */
    if (yyparse() != 0 || !ast_root)
    {
        std::cerr << "Erro de sintaxe.\n";
        return EXIT_FAILURE;
    }

    /* ---------- 4. Checagem de tipos + execução -- */
    try
    {
        TypeChecker tc;
        tc.check(ast_root); // verificação estática

        Interpreter itp;
        itp.interpret(ast_root); // executa (uma vez só!)
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
