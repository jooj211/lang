#ifndef PRINT_CMD_HPP
#define PRINT_CMD_HPP
#include "Command.hpp"
#include "Expression.hpp"
#include "Visitor.hpp"
class PrintCmd : public Command {
public:
    Expression* expr;
    explicit PrintCmd(Expression* e) : expr(e) {}
    ~PrintCmd() { delete expr; }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif