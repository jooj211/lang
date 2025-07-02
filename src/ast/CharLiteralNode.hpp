#ifndef CHAR_LITERAL_NODE_HPP
#define CHAR_LITERAL_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
class CharLiteralNode : public Expression {
public:
    char value;
    explicit CharLiteralNode(char val) : value(val) {}
    void accept(Visitor* v) override { v->visit(this); }
};
#endif