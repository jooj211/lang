%define api.prefix {lang}
%define parser_class_name {Parser}
%language "c++"

%union {
    std::string *stringVal;
    // AstNode *astNode;
}

%token <stringVal> IDENTIFIER TYPE_IDENTIFIER
%token <stringVal> INT_LITERAL FLOAT_LITERAL CHAR_LITERAL BOOL_LITERAL NULL_LITERAL

%token KW_IF KW_ITERATE KW_DATA KW_ABSTRACT KW_READ KW_PRINT KW_RETURN
%token KW_TYPE_INT KW_TYPE_CHAR KW_TYPE_BOOL KW_TYPE_FLOAT

%token AND_OP EQ_OP NEQ_OP LT_OP GT_OP ASSIGN_OP NOT_OP
%token PLUS_OP MINUS_OP MULT_OP DIV_OP MOD_OP

%token LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE
%token SEMICOLON COLON DOUBLE_COLON DOT COMMA

%token END_OF_FILE

%left OR_OP
%left AND_OP
%left EQ_OP NEQ_OP
%left LT_OP GT_OP
%left PLUS_OP MINUS_OP
%left MULT_OP DIV_OP MOD_OP
%right NOT_OP

%type <stringVal> IDENTIFIER INT_LITERAL FLOAT_LITERAL CHAR_LITERAL BOOL_LITERAL

%{
#include <iostream>
#include <string>
#include "ast.hpp"
#include "utils.hpp"

void yyerror(const char *msg);
extern int yylineno;
}%

%start program

%%

/* ------------- Regras da gramática de lang ------------- */

program:
      declarations
    | 
    ;

declarations:
      declaration declarations
    | 
    ;

declaration:
      KW_DATA TYPE_IDENTIFIER LBRACE /* ... corpo ... */ RBRACE
    | KW_ABSTRACT KW_DATA TYPE_IDENTIFIER LBRACE /* ... */ RBRACE
    | 
    ;


expression:
      INT_LITERAL
    | FLOAT_LITERAL
    | CHAR_LITERAL
    | IDENTIFIER
    | expression PLUS_OP expression
    | LPAREN expression RPAREN
    ;

%%

/* --------------------------------------------------------------------------------------------------------------------------------- */

void yyerror(const char *msg) {
    std::cerr << "Erro sintático na linha " << yylineno << ": " << msg << std::endl;
}

void lexerError(const char *msg, int line, int col) {
    std::cerr << "Erro léxico na linha " << line << ", coluna " << col << ": " << msg << std::endl;
    exit(EXIT_FAILURE);
}
