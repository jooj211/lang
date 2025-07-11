#pragma once
#include "Expression.hpp" // ou o caminho relativo correto
#include "Visitor.hpp"

class NullLiteralNode : public Expression
{
public:
    NullLiteralNode() = default;
    void accept(Visitor *v) override { v->visit(this); }
};
