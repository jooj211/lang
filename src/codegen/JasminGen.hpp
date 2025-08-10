
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

#include "../ast/Visitor.hpp"
#include "../ast/ProgramNode.hpp"
#include "../ast/FunDefNode.hpp"
#include "../ast/DataDefNode.hpp"
#include "../ast/Expression.hpp"
#include "../ast/FunCallNode.hpp"
#include "../ast/UnaryOpNode.hpp"
#include "../ast/BinaryOpNode.hpp"
#include "../ast/VarAccessNode.hpp"
#include "../ast/TypeNode.hpp"
#include "../ast/ArrayAccessNode.hpp"
#include "../ast/FieldAccessNode.hpp"
#include "../ast/NewExprNode.hpp"
#include "../ast/Command.hpp"
#include "../ast/AssignCmdNode.hpp"
#include "../ast/PrintCmd.hpp"
#include "../ast/ReadCmdNode.hpp"
#include "../ast/ReturnCmdNode.hpp"
#include "../ast/IfCmdNode.hpp"
#include "../ast/IterateCmdNode.hpp"
#include "../ast/FunCallCmdNode.hpp"
#include "../ast/IntLiteral.hpp"
#include "../ast/FloatLiteralNode.hpp"
#include "../ast/CharLiteralNode.hpp"
#include "../ast/BoolLiteralNode.hpp"
#include "../ast/NullLiteralNode.hpp"

struct FunSig{
    std::vector<std::string> param_descs;
    std::string ret_desc;
    std::vector<std::string> param_record_names; // '' if not record
    std::string ret_record_name; // '' if not record
};

struct LocalInfo{
    int index;
    std::string desc;
};

class JasminGen : public Visitor {
public:
    void generate(ProgramNode* node, std::ostream& os);

    // Visitor entry points
    void visit(ProgramNode* node);
    void visit(FunDefNode* node);
    void visit(DataDefNode* node);
    void visit(BlockCmdNode* node);
    void visit(FunCallNode* node);
    void visit(FunCallCmdNode* node);
    void visit(NewExprNode* node);
    void visit(FieldAccessNode* node);
    void visit(PrintCmd* node);
    void visit(ReadCmdNode* node);
    void visit(ReturnCmdNode* node);
    void visit(VarDeclNode* node);
    void visit(AssignCmdNode* node);
    void visit(IfCmdNode* node);
    void visit(IterateCmdNode* node);
    void visit(IntLiteral* node);
    void visit(FloatLiteralNode* node);
    void visit(CharLiteralNode* node);
    void visit(BoolLiteralNode* node);
    void visit(VarAccessNode* node);
    void visit(UnaryOpNode* node);
    void visit(BinaryOpNode* node);
    void visit(TypeNode* node);
    void visit(NullLiteralNode* node);
    void visit(ArrayAccessNode* node);

private:
    // emitter
    void emit_line(const std::string& s);
    void emit(const std::string& s);

    // class/method helpers
    void open_class();
    void close_class();
    void open_method(const std::string& name, const std::vector<std::pair<std::string,std::string>>& params, const std::string& ret);
    void close_method();

    // type helpers
    std::string type_desc(TypeNode* t);
    std::string type_desc_prim(Primitive k);
    std::string expr_desc(Expression* e);
    std::string record_name_of(Expression* e);
    std::string field_desc_from_record(Expression* rec_expr, const std::string& field);

    // locals
    void ensure_local(const std::string& name, const std::string& desc);
    int local_index(const std::string& name) const;
    void push_int(int v);
    void emit_load_local(const std::string& name);
    void emit_store_local(const std::string& name);

    // state
    std::stringstream out_;
    std::string cls_ = "Program";
    std::string cur_method_name_;
    std::string cur_method_ret_;
    std::vector<std::pair<std::string,std::string>> cur_params_;
    int stack_limit_ = 64;
    int locals_limit_ = 64;
    int next_local_ = 0;
    int loop_temp_counter_ = 0;
    int label_counter_ = 0;
    int new_label(){ return ++label_counter_; }

    std::unordered_map<std::string, FunSig> fun_sigs_;
    std::unordered_map<std::string, LocalInfo> locals_;

    // Records
    // data type -> (field -> field descriptor)
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> record_sigs_;
    // local var name -> data type name
    std::unordered_map<std::string, std::string> local_record_type_;
    // data type -> (field -> record type name) only for fields that are records
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> record_field_recordname_;
};
