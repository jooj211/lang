#ifndef READ_CMD_NODE_HPP
#define READ_CMD_NODE_HPP
#include "Command.hpp"
#include "Expression.hpp"
#include "Visitor.hpp"
class ReadCmdNode : public Command {
public:
    Expression* lvalue;
    explicit ReadCmdNode(Expression* l) : lvalue(l) {}
    ~ReadCmdNode() { delete lvalue; }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif