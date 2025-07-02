// Em src/ast/NewExprNode.hpp
#ifndef NEW_EXPR_NODE_HPP
#define NEW_EXPR_NODE_HPP

#include <vector>
#include "Expression.hpp"
#include "TypeNode.hpp"
#include "Visitor.hpp"

class NewExprNode : public Expression
{
public:
    TypeNode *base_type;            // O tipo base, ex: Transition
    std::vector<Expression *> dims; // Lista de expressões de dimensão

    NewExprNode(TypeNode *base, std::vector<Expression *> *d) : base_type(base)
    {
        if (d)
        {
            dims = *d;
            delete d;
        }
    }
    ~NewExprNode()
    {
        delete base_type;
        for (auto dim : dims)
        {
            if (dim)
                delete dim;
        }
    }
    void accept(Visitor *v) override { v->visit(this); }
};
#endif