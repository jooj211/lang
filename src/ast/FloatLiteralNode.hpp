#ifndef FLOAT_LITERAL_NODE_HPP
#define FLOAT_LITERAL_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
class FloatLiteralNode : public Expression {
public:
    float value;
    explicit FloatLiteralNode(float val) : value(val) {}
    void accept(Visitor* v) override { v->visit(this); }
};
#endif