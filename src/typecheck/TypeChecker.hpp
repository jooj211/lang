#ifndef TYPECHECKER_HPP
#define TYPECHECKER_HPP

#include "../ast/Visitor.hpp"
#include "Type.hpp"
#include <map>
#include <string>
#include <vector>
#include <memory>

// Forward declarations de todos os nós da AST
class ProgramNode;
class FunDefNode;
class DataDefNode;
class AssignCmdNode;
class IfCmdNode;
class PrintCmd;
class VarDeclNode;
class BlockCmdNode;
class FunCallNode;
class FunCallCmdNode;
class ReturnCmdNode;
class BinaryOpNode;
class UnaryOpNode;
class IntLiteral;
class FloatLiteralNode;
class CharLiteralNode;
class BoolLiteralNode;
class VarAccessNode;
class ReadCmdNode;
class IterateCmdNode;
class NewExprNode;
class FieldAccessNode;
class TypeNode;

class TypeChecker : public Visitor
{
public:
    virtual ~TypeChecker() override = default;

private:
    // Contexto de tipos de variáveis (Γ - Gamma)
    // Uma pilha de escopos para lidar com blocos e funções.
    std::vector<std::map<std::string, std::shared_ptr<Type>>> variable_types;

    // Contexto de tipos de registros (Δ - Delta)
    std::map<std::string, std::shared_ptr<RecordType>> record_types;

    // Contexto de tipos de funções (Θ - Theta)
    std::map<std::string, std::shared_ptr<FunctionType>> function_types;

    // Armazena o tipo da última expressão inferida
    std::shared_ptr<Type> last_inferred_type;

    // Armazena o tipo de retorno esperado da função atual
    std::shared_ptr<FunctionType> current_function_type;

    // Funções auxiliares de gerenciamento de escopo
    void push_scope();
    void pop_scope();
    void add_variable(const std::string &name, std::shared_ptr<Type> type);
    std::shared_ptr<Type> get_variable_type(const std::string &name);

    // Converte um nó de tipo da AST para a nossa representação interna
    std::shared_ptr<Type> type_from_node(TypeNode *node);

public:
    TypeChecker();
    void check(ProgramNode *ast);

    // Sobrescreve um método visit() para cada nó concreto da AST
    void visit(ProgramNode *node) override;
    void visit(FunDefNode *node) override;
    void visit(DataDefNode *node) override;
    void visit(AssignCmdNode *node) override;
    void visit(IfCmdNode *node) override;
    void visit(PrintCmd *node) override;
    void visit(VarDeclNode *node) override;
    void visit(BlockCmdNode *node) override;
    void visit(FunCallNode *node) override;
    void visit(FunCallCmdNode *node) override;
    void visit(ReturnCmdNode *node) override;
    void visit(BinaryOpNode *node) override;
    void visit(UnaryOpNode *node) override;
    void visit(IntLiteral *node) override;
    void visit(FloatLiteralNode *node) override;
    void visit(CharLiteralNode *node) override;
    void visit(BoolLiteralNode *node) override;
    void visit(VarAccessNode *node) override;
    void visit(ReadCmdNode *node) override;
    void visit(IterateCmdNode *node) override;
    void visit(NewExprNode *node) override;
    void visit(FieldAccessNode *node) override;
    void visit(TypeNode *node) override;
    void visit(NullLiteralNode *node) override;
    void visit(ArrayAccessNode *node) override;
};

#endif // TYPECHECKER_HPP