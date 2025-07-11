#ifndef FUN_CALL_NODE_HPP
#define FUN_CALL_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
#include <string>
#include <vector>
#include <cstdlib>
class FunCallNode : public Expression {
public:
    std::string name;
    std::vector<Expression*> args;
    Expression* return_index;
    FunCallNode(const std::string& s, std::vector<Expression*>* a, Expression* idx) : name(s), return_index(idx) {
        if (a) { args = *a; delete a; }
    }
    ~FunCallNode() {
        for (auto arg : args) { delete arg; }
        delete return_index;
    }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif