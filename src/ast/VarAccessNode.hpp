#ifndef VAR_ACCESS_NODE_HPP
#define VAR_ACCESS_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
#include <string>
#include <cstdlib>
class VarAccessNode : public Expression {
public:
    std::string name;
    explicit VarAccessNode(char* s) : name(s) { if(s) free(s); }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif