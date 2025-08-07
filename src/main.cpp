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

// Função de ajuda
static void usage(const char *exe)
{
    std::cerr << "Uso: " << exe << " [--test] [--debug] <diretiva> <arquivo.lang>\n\n"
              << "Opções:\n"
              << "  --test   Ativa argumentos falsos para teste (compile com -DFAKE_ARGS).\n"
              << "  --debug  Habilita o yydebug para traço do parser.\n\n"
              << "Diretivas disponíveis:\n"
              << "  -syn     Executa apenas a análise sintática e retorna 'accept' ou 'reject'.\n"
              << "  -i       Interpreta o programa após a checagem de tipos.\n";
}

int main(int argc, char *argv[])
{
    bool use_fake = false;
    bool enable_debug = false;

#ifdef FAKE_ARGS
    use_fake = true;
#endif

    // Verifica flags de runtime (--test e --debug)
    int idx = 1;
    while (idx < argc && argv[idx][0] == '-')
    {
        if (std::strcmp(argv[idx], "--test") == 0)
        {
            use_fake = true;
        }
        else if (std::strcmp(argv[idx], "--debug") == 0)
        {
            enable_debug = true;
        }
        else
        {
            break;
        }
        ++idx;
    }

    // Se estiver em modo fake, defina fake argv antes de mais nada
    if (use_fake)
    {
        const char *fake_argv[] = {"lang", "-i", "test.lang"};
        argc = 3;
        argv = const_cast<char **>(fake_argv);
    }
    else
    {
        // Remove as flags de runtime do argv real
        if (idx > 1)
        {
            for (int i = idx; i < argc; ++i)
                argv[i - (idx - 1)] = argv[i];
            argc -= (idx - 1);
        }
    }

    // Configura yydebug conforme flag
    yydebug = enable_debug ? 1 : 0;

    // Processa argumentos principais
    if (argc < 3)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    CompilerAction action;
    if (std::strcmp(argv[1], "-syn") == 0)
    {
        action = CompilerAction::SYNTACTIC_ANALYSIS;
    }
    else if (std::strcmp(argv[1], "-i") == 0)
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

    // Abre arquivo
    yyin = std::fopen(filename.c_str(), "r");
    if (!yyin)
    {
        std::perror(("Erro ao abrir " + filename).c_str());
        return EXIT_FAILURE;
    }

    // Análise sintática
    if (yyparse() != 0 || !ast_root || parse_error_detected)
    {
        if (action == CompilerAction::SYNTACTIC_ANALYSIS)
        {
            std::cout << "reject" << std::endl;
        }
        return EXIT_FAILURE;
    }

    // Ação de análise sintática apenas
    if (action == CompilerAction::SYNTACTIC_ANALYSIS)
    {
        std::cout << "accept" << std::endl;
        return EXIT_SUCCESS;
    }

    // Checagem de tipos e interpretação
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
