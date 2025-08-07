#ifndef ARRAY_ACCESS_NODE_HPP
#define ARRAY_ACCESS_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"

class ArrayAccessNode : public Expression
{
public:
    Expression *array_expr;
    Expression *index_expr;
    ArrayAccessNode(Expression *arr, Expression *idx) : array_expr(arr), index_expr(idx) {}
    ~ArrayAccessNode()
    {
        delete array_expr;
        delete index_expr;
    }
    void accept(Visitor *v) override { v->visit(this); }
};
#endif