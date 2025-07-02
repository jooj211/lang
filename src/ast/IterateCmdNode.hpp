#ifndef ITERATE_CMD_NODE_HPP
#define ITERATE_CMD_NODE_HPP
#include "Command.hpp"
#include "Expression.hpp"
#include "Visitor.hpp"
#include <string>
#include <cstdlib>
class IterateCmdNode : public Command {
public:
    std::string loop_variable;
    Expression* condition;
    Command* body;
    IterateCmdNode(const char* var, Expression* cond, Command* b) : loop_variable(var ? var : ""), condition(cond), body(b) {
      if (var && var[0] != '\0') free((void*)var);
    }
    ~IterateCmdNode() {
        delete condition;
        delete body;
    }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif