#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

// Includes que faltavam
#include <cstddef> // Para o tipo size_t
#include <map>
#include <string>
#include <vector>

// Headers do seu projeto
#include "../ast/Visitor.hpp"
#include "../runtime/Value.hpp"

// Forward declarations para os nós da AST usados nos parâmetros
// Isso avisa ao compilador que essas classes existem, sem precisar incluir o header inteiro.
class TypeNode;
class Expression;
class FunDefNode;
class DataDefNode;

class Interpreter : public Visitor
{
private:
    std::vector<std::map<std::string, Value *>> memory_stack;
    std::map<std::string, FunDefNode *> functions;
    std::map<std::string, DataDefNode *> data_types;
    Value *last_value = nullptr;
    std::vector<Value *> value_pool;

    void push_scope();
    void pop_scope();
    void set_variable(const std::string &name, Value *value, bool is_decl = false);
    void update_variable(const std::string &name, Value *new_value_obj);
    Value *get_variable(const std::string &name);

    // --- Métodos Auxiliares para Arrays/Matrizes ---
    Value *create_default_value(TypeNode *type);
    Value *create_nested_array(TypeNode *base_elem_type,
                               const std::vector<Expression *> &dims,
                               size_t dim_index);

public:
    ~Interpreter();
    void interpret(ProgramNode *ast);

    // Métodos visit() ...
    void visit(ProgramNode *node) override;
    void visit(FunDefNode *node) override;
    void visit(DataDefNode *node) override;
    void visit(BlockCmdNode *node) override;
    void visit(FunCallNode *node) override;
    void visit(FunCallCmdNode *node) override;
    void visit(NewExprNode *node) override;
    void visit(FieldAccessNode *node) override;
    void visit(ArrayAccessNode *node) override; // Não se esqueça de adicionar este
    void visit(PrintCmd *node) override;
    void visit(ReadCmdNode *node) override;
    void visit(ReturnCmdNode *node) override;
    void visit(VarDeclNode *node) override;
    void visit(AssignCmdNode *node) override;
    void visit(IfCmdNode *node) override;
    void visit(IterateCmdNode *node) override;
    void visit(IntLiteral *node) override;
    void visit(FloatLiteralNode *node) override;
    void visit(CharLiteralNode *node) override;
    void visit(BoolLiteralNode *node) override;
    void visit(VarAccessNode *node) override;
    void visit(UnaryOpNode *node) override;
    void visit(BinaryOpNode *node) override;
    void visit(TypeNode *node) override;
    void visit(NullLiteralNode *node) override;
};
#endif