%code requires {
    #include <vector>
    #include <string>
    #include "ast/AST.hpp"
}

%{
#include <iostream>
#include <cstdio>
#include "ast/ProgramNode.hpp"
#include "ast/FunDefNode.hpp"
#include "ast/DataDefNode.hpp"
#include "ast/UnaryOpNode.hpp"
#include "ast/BinaryOpNode.hpp"
#include "ast/FunCallNode.hpp"
#include "ast/FunCallCmdNode.hpp"
#include "ast/NewExprNode.hpp"
#include "ast/FieldAccessNode.hpp"
#include "ast/ArrayAccessNode.hpp" // Adicionado para acesso a arrays

extern int yylex();
extern int yylineno;
void yyerror(const char* s);
bool parse_error_detected = false;

ProgramNode* ast_root = nullptr;
%}

%union {
    int                 ival;
    float               fval;
    char                cval;
    bool                bval;
    char* sval;
    ProgramNode* program_node;
    Command* command_node;
    Expression* expression_node;
    TypeNode* type_node;
    std::vector<Command*>* command_list;
    std::vector<Expression*>* expression_list;
    Node* def_node;
    std::vector<Node*>* def_list;
    FunDefNode::Param* param_node;
    std::vector<FunDefNode::Param>* param_list;
    std::vector<TypeNode*>* type_list;
    VarDeclNode* field_decl_node;
    std::vector<VarDeclNode*>* field_list;
}

%token LT_CAPTURE "<"

%define parse.trace
%define parse.error verbose

%token <ival> T_INT_LITERAL
%token <fval> T_FLOAT_LITERAL
%token <cval> T_CHAR_LITERAL
%token <sval> T_ID T_TYID
%token <bval> T_TRUE T_FALSE
%token T_NULL "null"
%token T_PRINT "print"
%token T_READ "read"
%token T_RETURN "return"
%token T_IF "if"
%token T_ELSE "else"
%token T_ITERATE "iterate"
%token T_DATA "data"
%token T_NEW "new"
%token T_TYPE_INT "Int"
%token T_TYPE_BOOL "Bool"
%token T_TYPE_CHAR "Char"
%token T_TYPE_FLOAT "Float"
%token T_TYPE_VOID "Void"
%token T_COLON_COLON "::"
%token T_AND "&&"
%token T_EQ "=="
%token T_NEQ "!="

%type <program_node> program
%type <def_list> def_list
%type <def_node> def fun_def data_def
%type <command_list> command_list
%type <command_node> command var_decl assign_cmd if_cmd iterate_cmd block read_cmd return_cmd fun_call_cmd
%type <expression_node> expression lvalue primary_expression postfix_expression new_expression
%type <type_node> type atomic_type
%type <expression_list> expression_list optional_expression_list lvalue_list array_dim_list
%type <param_node> param
%type <param_list> params param_list_non_empty
%type <type_list> optional_return_types type_list_non_empty
%type <field_decl_node> field_decl
%type <field_list> field_list


/*---------------------------------------------------------------
 * PRECEDÊNCIAS
 *--------------------------------------------------------------*/
%nonassoc T_ELSE
%left     T_AND
%nonassoc '<' T_EQ T_NEQ '>'
%left     LT_CAPTURE
%left     '+' '-'
%left     '*' '/' '%'
%right    '!'
%right    UMINUS
%left     '.'
%left     '['

%% /* ===================  GRAMMAR  =================== */

program:
    def_list                      { ast_root = new ProgramNode($1); }
    ;

def_list:
      /* empty */                { $$ = new std::vector<Node*>(); }
    | def_list def               { $1->push_back($2); $$ = $1; }
    ;

def:
      fun_def                    { $$ = $1; }
    | data_def                   { $$ = $1; }
    ;

fun_def:
    T_ID '(' params ')' optional_return_types block
                                  { $$ = new FunDefNode($1, $3, $5, dynamic_cast<BlockCmdNode*>($6)); }
    ;

data_def:
    T_DATA T_TYID '{' field_list '}'  { $$ = new DataDefNode($2, $4); }
    ;

field_list:
      /* empty */                { $$ = new std::vector<VarDeclNode*>(); }
    | field_list field_decl      { $1->push_back($2); $$ = $1; }
    ;

field_decl:
    T_ID T_COLON_COLON type ';'  { $$ = new VarDeclNode($1, $3); }
    ;

params:
      /* empty */                { $$ = new std::vector<FunDefNode::Param>(); }
    | param_list_non_empty       { $$ = $1; }
    ;

param_list_non_empty:
      param                      { $$ = new std::vector<FunDefNode::Param>(); $$->push_back(*$1); delete $1; }
    | param_list_non_empty ',' param  { $1->push_back(*$3); delete $3; $$ = $1; }
    ;

param:
    T_ID T_COLON_COLON type      { $$ = new FunDefNode::Param({std::string($1), $3}); free($1); }
    ;

optional_return_types:
      /* empty */                { $$ = nullptr; }
    | ':' type_list_non_empty    { $$ = $2; }
    ;

type_list_non_empty:
      type                       { $$ = new std::vector<TypeNode*>(); $$->push_back($1); }
    | type_list_non_empty ',' type { $1->push_back($3); $$ = $1; }
    ;

command_list:
      /* empty */                { $$ = new std::vector<Command*>(); }
    | command_list command       { $1->push_back($2); $$ = $1; }
    ;

command:
      var_decl
    | assign_cmd
    | if_cmd
    | iterate_cmd
    | block
    | read_cmd
    | return_cmd
    | T_PRINT expression ';'     { $$ = new PrintCmd($2); }
    | fun_call_cmd ';'           { $$ = $1; }
    ;

fun_call_cmd:
    T_ID '(' optional_expression_list ')'
      { $$ = new FunCallCmdNode($1, $3, nullptr); }
  | T_ID '(' optional_expression_list ')' LT_CAPTURE lvalue_list '>'
      { $$ = new FunCallCmdNode($1, $3, $6); }
  ;

lvalue_list:
      lvalue                     { $$ = new std::vector<Expression*>(); $$->push_back($1); }
    | lvalue_list ',' lvalue     { $1->push_back($3); $$ = $1; }
    ;

block:
    '{' command_list '}'         { $$ = new BlockCmdNode($2); }
    ;

read_cmd:
    T_READ lvalue ';'            { $$ = new ReadCmdNode($2); }
    ;

return_cmd:
    T_RETURN expression_list ';' { $$ = new ReturnCmdNode($2); }
    ;

expression_list:
      expression
        { $$ = new std::vector<Expression*>(); $$->push_back($1); }
    | expression_list ',' expression
        { $1->push_back($3); $$ = $1; }
    ;

optional_expression_list:
      /* empty */                { $$ = new std::vector<Expression*>(); }
    | expression_list            { $$ = $1; }
    ;

if_cmd:
    T_IF '(' expression ')' command %prec T_IF
        { $$ = new IfCmdNode($3, $5, nullptr); }
  | T_IF '(' expression ')' command T_ELSE command
        { $$ = new IfCmdNode($3, $5, $7); }
  ;

iterate_cmd:
    T_ITERATE '(' expression ')' command
        { $$ = new IterateCmdNode(nullptr, $3, $5); }
  | T_ITERATE '(' T_ID ':' expression ')' command
        { $$ = new IterateCmdNode($3, $5, $7); }
  ;

var_decl:
    T_ID T_COLON_COLON type ';'  { $$ = new VarDeclNode($1, $3); }
    ;

assign_cmd:
    lvalue '=' expression ';' { $$ = new AssignCmdNode($1, $3); }
    ;

lvalue:
      T_ID
        { $$ = new VarAccessNode($1); }
    | postfix_expression '.' T_ID
        { $$ = new FieldAccessNode($1, $3); }
    | postfix_expression '[' expression ']'
        { $$ = new ArrayAccessNode($1, $3); }
    ;

type:
      atomic_type
    | type '[' ']'               { $$ = new TypeNode($1); }
    ;

atomic_type:
      T_TYPE_INT                 { $$ = new TypeNode(Primitive::INT); }
    | T_TYPE_BOOL                { $$ = new TypeNode(Primitive::BOOL); }
    | T_TYPE_CHAR                { $$ = new TypeNode(Primitive::CHAR); }
    | T_TYPE_FLOAT               { $$ = new TypeNode(Primitive::FLOAT); }
    | T_TYPE_VOID                { $$ = new TypeNode(Primitive::VOID); }
    | T_TYID                     { $$ = new TypeNode($1); }
    ;

expression:
      postfix_expression
    | '!' expression             { $$ = new UnaryOpNode('!', $2); }
    | '-' expression %prec UMINUS{ $$ = new UnaryOpNode('-', $2); }
    | expression T_AND expression{ $$ = new BinaryOpNode($1, '&', $3); }
    | expression T_EQ  expression{ $$ = new BinaryOpNode($1, '=', $3); }
    | expression T_NEQ expression{ $$ = new BinaryOpNode($1, 'n', $3); }
    | expression '<' expression  { $$ = new BinaryOpNode($1, '<', $3); }
    | expression '>' expression  { $$ = new BinaryOpNode($1, '>', $3); }
    | expression '+' expression  { $$ = new BinaryOpNode($1, '+', $3); }
    | expression '-' expression  { $$ = new BinaryOpNode($1, '-', $3); }
    | expression '*' expression  { $$ = new BinaryOpNode($1, '*', $3); }
    | expression '/' expression  { $$ = new BinaryOpNode($1, '/', $3); }
    | expression '%' expression  { $$ = new BinaryOpNode($1, '%', $3); }
    ;

postfix_expression:
      primary_expression
    | postfix_expression '[' expression ']' { $$ = new ArrayAccessNode($1, $3); }
    | postfix_expression '.' T_ID           { $$ = new FieldAccessNode($1, $3); }
    | T_ID '(' optional_expression_list ')' '[' expression ']' { $$ = new FunCallNode(std::string($1), $3, $6); free($1); }
    ;

primary_expression:
      lvalue
    | T_INT_LITERAL                        { $$ = new IntLiteral($1); }
    | T_FLOAT_LITERAL                      { $$ = new FloatLiteralNode($1); }
    | T_CHAR_LITERAL                       { $$ = new CharLiteralNode($1); }
    | T_TRUE                               { $$ = new BoolLiteralNode(true); }
    | T_FALSE                              { $$ = new BoolLiteralNode(false); }
    | T_NULL                               { $$ = new NullLiteralNode(); }
    | '(' expression ')'                   { $$ = $2; }
    | new_expression                       { $$ = $1; }
    ;

new_expression:
      T_NEW atomic_type
        { $$ = new NewExprNode($2, nullptr); }
    | T_NEW atomic_type array_dim_list
        { $$ = new NewExprNode($2, $3); }
    ;

array_dim_list:
      '[' expression ']'
        { $$ = new std::vector<Expression*>(); $$->push_back($2); }
    | array_dim_list '[' expression ']'
        { $1->push_back($3); $$ = $1; }
    ;

%% /* =================== C-code =================== */

void yyerror(const char* s)
{
    parse_error_detected = true;
    extern char* yytext;
    std::fprintf(stderr,
                 "Erro sintático na linha %d: %s perto do texto '%s'\n",
                 yylineno, s, yytext);
}