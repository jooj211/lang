#ifndef TYPE_NODE_HPP
#define TYPE_NODE_HPP

#include "Node.hpp"
#include "Visitor.hpp"
#include "../typecheck/Primitive.hpp" // INCLUÍDO
#include <string>

class TypeNode : public Node
{
public:
    Primitive p_type;
    std::string user_type_name;
    bool is_primitive;
    // --- Novos campos para suportar arrays ---
    bool is_array = false;
    TypeNode *element_type = nullptr; // Se is_array=true, aponta para o tipo do elemento

    explicit TypeNode(Primitive t) : p_type(t), is_primitive(true) {}
    explicit TypeNode(char *s) : user_type_name(s), is_primitive(false)
    {
        if (s)
            free(s);
    }

    // --- Novo construtor para tipos de array ---
    explicit TypeNode(TypeNode *elem_t) : is_array(true), element_type(elem_t) {}

    // Destrutor para limpar a memória do tipo do elemento
    ~TypeNode()
    {
        delete element_type;
    }

    void accept(Visitor *v) override
    {
        v->visit(this);
    }
};

#endif