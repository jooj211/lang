#ifndef UNARY_OP_NODE_HPP
#define UNARY_OP_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
class UnaryOpNode : public Expression {
public:
    char op;
    Expression* expr;
    UnaryOpNode(char o, Expression* e) : op(o), expr(e) {}
    ~UnaryOpNode() { delete expr; }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif