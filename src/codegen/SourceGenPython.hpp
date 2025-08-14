
#pragma once
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ast/Visitor.hpp"
#include "../ast/ProgramNode.hpp"
#include "../ast/FunDefNode.hpp"
#include "../ast/DataDefNode.hpp"
#include "../ast/BlockCmdNode.hpp"
#include "../ast/Expression.hpp"
#include "../ast/FunCallNode.hpp"
#include "../ast/NewExprNode.hpp"
#include "../ast/FieldAccessNode.hpp"
#include "../ast/ArrayAccessNode.hpp"
#include "../ast/VarAccessNode.hpp"
#include "../ast/UnaryOpNode.hpp"
#include "../ast/BinaryOpNode.hpp"
#include "../ast/TypeNode.hpp"
#include "../ast/IntLiteral.hpp"
#include "../ast/FloatLiteralNode.hpp"
#include "../ast/CharLiteralNode.hpp"
#include "../ast/BoolLiteralNode.hpp"
#include "../ast/NullLiteralNode.hpp"

#include "../ast/AssignCmdNode.hpp"
#include "../ast/PrintCmd.hpp"
#include "../ast/ReadCmdNode.hpp"
#include "../ast/ReturnCmdNode.hpp"
#include "../ast/IfCmdNode.hpp"
#include "../ast/IterateCmdNode.hpp"
#include "../ast/FunCallCmdNode.hpp"
#include "../ast/VarDeclNode.hpp"

class SourceGenPython : public Visitor {
public:
    void generate(ProgramNode* node, std::ostream& os);

    // Visitors
    void visit(ProgramNode* node) override;
    void visit(FunDefNode* node) override;
    void visit(DataDefNode* node) override;
    void visit(BlockCmdNode* node) override;

    void visit(FunCallNode* node) override;
    void visit(FunCallCmdNode* node) override;
    void visit(NewExprNode* node) override;
    void visit(FieldAccessNode* node) override;
    void visit(ArrayAccessNode* node) override;

    void visit(PrintCmd* node) override;
    void visit(ReadCmdNode* node) override;
    void visit(ReturnCmdNode* node) override;
    void visit(VarDeclNode* node) override;
    void visit(AssignCmdNode* node) override;
    void visit(IfCmdNode* node) override;
    void visit(IterateCmdNode* node) override;

    void visit(IntLiteral* node) override;
    void visit(FloatLiteralNode* node) override;
    void visit(CharLiteralNode* node) override;
    void visit(BoolLiteralNode* node) override;
    void visit(VarAccessNode* node) override;
    void visit(UnaryOpNode* node) override;
    void visit(BinaryOpNode* node) override;
    void visit(TypeNode* node) override;
    void visit(NullLiteralNode* node) override;

private:
    // helpers
    void emit(const std::string& s){ out_ << s; }
    void emitln(const std::string& s=""){ out_ << s << "\n"; }
    void indent(){ for(int i=0;i<indent_;++i) out_ << "    "; }
    std::string py_tmp();

    // expr helpers
    void emit_expr(Expression* e);
    std::string default_value_for(TypeNode* t);

    // type helpers for typed read
    std::string read_fn_for(TypeNode* t);
    TypeNode* field_type_of(const std::string& record_name, const std::string& field) const;
    std::string record_name_of_expr(Expression* e) const;
    TypeNode* type_of_expr(Expression* e) const;

    // state
    std::stringstream out_;
    int indent_ = 0;
    int tmp_counter_ = 0;

    // record name -> fields (name -> type)
    std::unordered_map<std::string, std::vector<std::pair<std::string, TypeNode*>>> records_;

    // locals types for current function scope (including params & var decls)
    std::unordered_map<std::string, TypeNode*> locals_types_;

    // current function: for future use
    std::string cur_fun_;

    // function name -> return types
    std::unordered_map<std::string, std::vector<TypeNode*>> fun_returns_;
};
