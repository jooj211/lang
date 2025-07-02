#ifndef IF_CMD_NODE_HPP
#define IF_CMD_NODE_HPP
#include "Command.hpp"
#include "Expression.hpp"
#include "Visitor.hpp"
class IfCmdNode : public Command {
public:
    Expression* condition;
    Command* then_branch;
    Command* else_branch;
    IfCmdNode(Expression* c, Command* t, Command* e) : condition(c), then_branch(t), else_branch(e) {}
    ~IfCmdNode() { delete condition; delete then_branch; if (else_branch) delete else_branch; }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif