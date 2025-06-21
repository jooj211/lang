%option noyywrap
%option c++
%option header-file="lexer.hpp"

%{
#include <parser.hpp>
#include <string>
#include <iostream>

void lexerError(const char *msg, int line, int col);
int yylineno = 1;
int yycolumn = 1;
%}

/* ---------- macros de padrões ---------- */
DIGIT           [0-9]
LETTER_LOWER    [a-z]
LETTER_UPPER    [A-Z]
ID_CONT         [a-zA-Z0-9_]

COMMENT_LINE    --.*
COMMENT_BLOCK   \\{-(.|\\n)*-\\}
WHITESPACE      [ \\t\\r\\n]+
CHAR_ESC        \\\\(n|t|\\\\|'|\\"|[0-7]{1,3})
CHAR_LITERAL    \\'(\\\\.|[^\\\\'])\\'
FLOAT_LIT       ({DIGIT}+\\.{DIGIT}*|\\.{DIGIT}+)
INT_LIT         {DIGIT}+
ID              {LETTER_LOWER}{ID_CONT}*
TYPE_ID         {LETTER_UPPER}{ID_CONT}*

%%          /* ---------- regras ---------- */

{WHITESPACE}  {
                for (char c : yytext) {
                    if (c == '\n') { yylineno++; yycolumn = 1; }
                    else yycolumn++;
                }
              }

{COMMENT_LINE} { /* ignora */ }

{COMMENT_BLOCK} {
                  for (char c : yytext)
                      if (c == '\n') { yylineno++; yycolumn = 1; }
                }

/* Literais numéricos */
{FLOAT_LIT}   { yylval.stringVal = new std::string(yytext); return FLOAT_LITERAL; }
{INT_LIT}     { yylval.stringVal = new std::string(yytext); return INT_LITERAL; }

/* Literal de caractere */
{CHAR_LITERAL} { yylval.stringVal = new std::string(yytext); return CHAR_LITERAL; }

/* Identificadores e palavras-chave */
{ID} {
        std::string s(yytext);
        if (s == "true" || s == "false")   { yylval.stringVal = new std::string(s); return BOOL_LITERAL; }
        else if (s == "null")              { return NULL_LITERAL; }

        /* palavras-chave */
        else if (s == "if")      return KW_IF;
        else if (s == "iterate") return KW_ITERATE;
        else if (s == "data")    return KW_DATA;
        else if (s == "abstract")return KW_ABSTRACT;
        else if (s == "read")    return KW_READ;
        else if (s == "print")   return KW_PRINT;
        else if (s == "return")  return KW_RETURN;
        else if (s == "Int")     return KW_TYPE_INT;
        else if (s == "Char")    return KW_TYPE_CHAR;
        else if (s == "Bool")    return KW_TYPE_BOOL;
        else if (s == "Float")   return KW_TYPE_FLOAT;

        yylval.stringVal = new std::string(s);
        return IDENTIFIER;
     }

{TYPE_ID}      { yylval.stringVal = new std::string(yytext); return TYPE_IDENTIFIER; }

/* Operadores compostos */
"&&"           { return AND_OP; }
"=="           { return EQ_OP; }
"!="           { return NEQ_OP; }
"::"           { return DOUBLE_COLON; }

/* Símbolos simples */
"<"            { return LT_OP; }
">"            { return GT_OP; }
"="            { return ASSIGN_OP; }
"!"            { return NOT_OP; }
"+"            { return PLUS_OP; }
"-"            {
