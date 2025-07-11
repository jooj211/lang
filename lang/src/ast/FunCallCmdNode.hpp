#ifndef FUN_CALL_CMD_NODE_HPP
#define FUN_CALL_CMD_NODE_HPP
#include "Command.hpp"
#include "Expression.hpp"
#include "Visitor.hpp"
#include <string>
#include <vector>
#include <cstdlib>
class FunCallCmdNode : public Command {
public:
    std::string name;
    std::vector<Expression*> args;
    std::vector<Expression*> lvalues;
    FunCallCmdNode(char* s, std::vector<Expression*>* a, std::vector<Expression*>* lvals) : name(s) {
        if (s) free(s);
        if (a) { args = *a; delete a; }
        if (lvals) { lvalues = *lvals; delete lvals; }
    }
    ~FunCallCmdNode() {
        for (auto arg : args) { delete arg; }
        for (auto lval : lvalues) { delete lval; }
    }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif