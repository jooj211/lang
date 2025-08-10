#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>
#include <fstream>

#include "typecheck/TypeChecker.hpp"
#include "ast/ProgramNode.hpp"
#include "interpreter/Interpreter.hpp"
#include "codegen/SourceGenPython.hpp"
#include "codegen/JasminGen.hpp"

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
    TYPECHECK,          // Para a flag -t
    SRCGEN,             // Para a flag -src
    JASMINGEN,          // Para a flag -genc
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
              << "  -t       Executa apenas a análise de tipos e retorna 'accept' ou 'reject'.\n"
              << "  -src     Gera código em Python para o programa. Use -o para salvar.\n"
              << "  -gen     Gera código Jasmin (.j) para JVM. Use -o para salvar.\n"
              << "  -i       Interpreta o programa após a checagem de tipos.\n";
}

int main(int argc, char *argv[])
{
    const char* out_path = nullptr;

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

    // Procura por -o <arquivo> após a diretiva e antes do nome do arquivo
    for (int k = 2; k+1 < argc; ++k) {
        if (std::strcmp(argv[k], "-o") == 0) {
            out_path = argv[k+1];
        }
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
    else if (std::strcmp(argv[1], "-t") == 0)
    {
        action = CompilerAction::TYPECHECK;
    }
    else if (std::strcmp(argv[1], "-src") == 0)
    {
        action = CompilerAction::SRCGEN;
    }
    else if (std::strcmp(argv[1], "-gen") == 0)
    {
        action = CompilerAction::JASMINGEN;
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

    
    // Determina o arquivo de entrada ignorando flags (-syn, -t, -src, -i, -o <arq>, --test, --debug)
    std::string filename;
    {
        bool skip_next = false;
        for (int k = 1; k < argc; ++k) {
            if (skip_next) { skip_next = false; continue; }
            std::string argk = argv[k];
            if (argk == "--test" || argk == "--debug") continue;
            if (argk == "-syn" || argk == "-t" || argk == "-src" || argk == "-i") continue;
            if (argk == "-o") { skip_next = true; continue; }
            // considera este como potencial filename
            filename = argk;
        }
    }
    if (filename.empty()) {
        std::cerr << "Erro: Caminho do arquivo de entrada não informado." << std::endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }
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

    // Ação de geração de código-fonte (Python)
    if (action == CompilerAction::SRCGEN)
    {
        try
        {
            // checagem de tipos antes de gerar
            TypeChecker tc;
            tc.check(ast_root);
            SourceGenPython gen;
            if (out_path) {
                std::ofstream ofs(out_path);
                gen.generate(ast_root, ofs);
            } else {
                gen.generate(ast_root, std::cout);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Erro: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    // Ação de geração de Jasmin (.j)
    if (action == CompilerAction::JASMINGEN)
    {
        try
        {
            TypeChecker tc;
            tc.check(ast_root);
            JasminGen gen;
            if (out_path) {
                std::ofstream ofs(out_path);
                gen.generate(ast_root, ofs);
            } else {
                gen.generate(ast_root, std::cout);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Erro: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }


    // Ação de análise de tipos apenas
    if (action == CompilerAction::TYPECHECK)
    {
        try
        {
            TypeChecker tc;
            tc.check(ast_root);
            std::cout << "accept" << std::endl;
        }
        catch (const std::exception &)
        {
            std::cout << "reject" << std::endl;
        }
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
