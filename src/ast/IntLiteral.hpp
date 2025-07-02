#ifndef INT_LITERAL_HPP
#define INT_LITERAL_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
class IntLiteral : public Expression {
public:
    int value;
    explicit IntLiteral(int val) : value(val) {}
    void accept(Visitor* v) override { v->visit(this); }
};
#endif