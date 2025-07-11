#ifndef BINARY_OP_NODE_HPP
#define BINARY_OP_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
class BinaryOpNode : public Expression {
public:
    Expression* left;
    char op;
    Expression* right;
    BinaryOpNode(Expression* l, char o, Expression* r) : left(l), op(o), right(r) {}
    ~BinaryOpNode() { delete left; delete right; }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif