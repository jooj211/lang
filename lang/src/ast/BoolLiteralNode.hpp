#ifndef BOOL_LITERAL_NODE_HPP
#define BOOL_LITERAL_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
class BoolLiteralNode : public Expression {
public:
    bool value;
    explicit BoolLiteralNode(bool val) : value(val) {}
    void accept(Visitor* v) override { v->visit(this); }
};
#endif