#ifndef FUN_DEF_NODE_HPP
#define FUN_DEF_NODE_HPP
#include "Node.hpp"
#include "BlockCmdNode.hpp"
#include "TypeNode.hpp"
#include "Visitor.hpp"
#include <string>
#include <vector>
#include <cstdlib>
class FunDefNode : public Node {
public:
    struct Param {
        std::string name;
        TypeNode* type;
    };
    std::string name;
    std::vector<Param> params;
    std::vector<TypeNode*> return_types;
    BlockCmdNode* body;
    FunDefNode(char* s, std::vector<Param>* p, std::vector<TypeNode*>* r, BlockCmdNode* b) : name(s), body(b) {
        if (s) free(s);
        if (p) { params = *p; delete p; }
        if (r) { return_types = *r; delete r; }
    }
    ~FunDefNode() {
        for (auto const& param : params) { delete param.type; }
        for (auto const& ret_type : return_types) { delete ret_type; }
        delete body;
    }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif