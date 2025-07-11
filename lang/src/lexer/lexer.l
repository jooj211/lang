%{
#include <cstdlib>
#include <cstring>
#include "parser.tab.hpp"

void yyerror(const char* s);

#define RESET() BEGIN(INITIAL)
%}

%x COMMENT
%x AFTER_RPAREN

%option noyywrap
%option yylineno
%%

"print"     { RESET(); return T_PRINT; }
"read"      { RESET(); return T_READ; }
"return"    { RESET(); return T_RETURN; }
"if"        { RESET(); return T_IF; }
"else"      { RESET(); return T_ELSE; }
"true"      { RESET(); return T_TRUE; }
"false"     { RESET(); return T_FALSE; }
"iterate"   { RESET(); return T_ITERATE; }
"data"      { RESET(); return T_DATA; }
"new"       { RESET(); return T_NEW; }
"Int"       { RESET(); return T_TYPE_INT; }
"Bool"      { RESET(); return T_TYPE_BOOL; }
"Char"      { RESET(); return T_TYPE_CHAR; }
"Float"     { RESET(); return T_TYPE_FLOAT; }
"Void"      { RESET(); return T_TYPE_VOID; } // <<< ADICIONAR
"null"      { RESET(); return T_NULL; }

[a-z][a-zA-Z0-9_]* { RESET(); yylval.sval = strdup(yytext); return T_ID; }
[A-Z][a-zA-Z0-9_]* { RESET(); yylval.sval = strdup(yytext); return T_TYID; }

[0-9]+\.[0-9]+ { RESET(); yylval.fval = atof(yytext); return T_FLOAT_LITERAL; }
[0-9]+         { RESET(); yylval.ival = atoi(yytext); return T_INT_LITERAL; }
'([^'\\]|\\.)' { RESET(); yylval.cval = yytext[1];    return T_CHAR_LITERAL; }

"::"   { RESET(); return T_COLON_COLON; }
"&&"   { RESET(); return T_AND; }
"=="   { RESET(); return T_EQ; }
"!="   { RESET(); return T_NEQ; }

"<"    { RESET(); return '<'; }

":"    { RESET(); return ':'; }
","    { RESET(); return ','; }
"."    { RESET(); return '.'; }
">"    { RESET(); return '>'; }
"!"    { RESET(); return '!'; }
"="    { RESET(); return '='; }
"+"    { RESET(); return '+'; }
"-"    { RESET(); return '-'; }
"*"    { RESET(); return '*'; }
"%"    { RESET(); return '%'; }
"/"    { RESET(); return '/'; }
";"    { RESET(); return ';'; }
"{"    { RESET(); return '{'; }
"}"    { RESET(); return '}'; }
"["    { RESET(); return '['; }
"]"    { RESET(); return ']'; }

"("    { RESET(); return '('; }
")"    { BEGIN(AFTER_RPAREN); return ')'; }

<AFTER_RPAREN>[ \t\r\n]+     { }
<AFTER_RPAREN>"<"            { RESET(); return LT_CAPTURE; }
<AFTER_RPAREN>.              { RESET(); unput(yytext[0]); }

"//".*            { }
"--".*            { }
"{-"              { BEGIN(COMMENT); }
<COMMENT>"-}"     { RESET(); }
<COMMENT>.        { }
<COMMENT>\n       { }

[ \t\r\n]+        { }

.                 { RESET(); yyerror("Caractere inesperado"); }
%%
