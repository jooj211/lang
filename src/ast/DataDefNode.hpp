#ifndef DATA_DEF_NODE_HPP
#define DATA_DEF_NODE_HPP
#include "Node.hpp"
#include "VarDeclNode.hpp"
#include "Visitor.hpp"
#include <string>
#include <vector>
#include <cstdlib>
class DataDefNode : public Node {
public:
    std::string name;
    std::vector<VarDeclNode*> fields;
    DataDefNode(char* s, std::vector<VarDeclNode*>* f) : name(s) {
        if (s) free(s);
        if (f) { fields = *f; delete f; }
    }
    ~DataDefNode() { for (auto field : fields) { delete field; } }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif