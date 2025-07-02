#ifndef FIELD_ACCESS_NODE_HPP
#define FIELD_ACCESS_NODE_HPP
#include "Expression.hpp"
#include "Visitor.hpp"
#include <string>
#include <cstdlib>
class FieldAccessNode : public Expression {
public:
    Expression* record_expr;
    std::string field_name;
    FieldAccessNode(Expression* rec, char* field) : record_expr(rec), field_name(field) {
        if (field) free(field);
    }
    ~FieldAccessNode() { delete record_expr; }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif