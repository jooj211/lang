#ifndef VAR_DECL_NODE_HPP
#define VAR_DECL_NODE_HPP
#include "Command.hpp"
#include "TypeNode.hpp"
#include "Visitor.hpp"
#include <string>
#include <cstdlib>
class VarDeclNode : public Command {
public:
    std::string name;
    TypeNode* type;
    VarDeclNode(char* s, TypeNode* t) : name(s), type(t) { if(s) free(s); }
    ~VarDeclNode() { delete type; }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif