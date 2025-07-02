#ifndef ASSIGN_CMD_NODE_HPP
#define ASSIGN_CMD_NODE_HPP
#include "Command.hpp"
#include "Expression.hpp"
#include "Visitor.hpp"
class AssignCmdNode : public Command {
public:
    Expression* lvalue;
    Expression* expr;
    AssignCmdNode(Expression* l, Expression* e) : lvalue(l), expr(e) {}
    ~AssignCmdNode() { delete lvalue; delete expr; }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif