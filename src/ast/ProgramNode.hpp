#ifndef PROGRAM_NODE_HPP
#define PROGRAM_NODE_HPP
#include <vector>
#include "Node.hpp"
#include "Visitor.hpp"
class ProgramNode : public Node {
public:
    std::vector<Node*> definitions;
    ProgramNode(std::vector<Node*>* defs) { if(defs) { definitions = *defs; delete defs; } }
    ~ProgramNode() { for (auto def : definitions) { delete def; } }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif