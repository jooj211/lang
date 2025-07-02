#ifndef RETURN_CMD_NODE_HPP
#define RETURN_CMD_NODE_HPP
#include "Command.hpp"
#include "Expression.hpp"
#include "Visitor.hpp"
#include <vector>
class ReturnCmdNode : public Command {
public:
    std::vector<Expression*> expressions;
    explicit ReturnCmdNode(std::vector<Expression*>* exprs) { if(exprs) { expressions = *exprs; delete exprs; } }
    ~ReturnCmdNode() {
        for(auto expr : expressions) {
            delete expr;
        }
    }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif