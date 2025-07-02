#ifndef TYPE_HPP
#define TYPE_HPP

#include "Primitive.hpp" // enum class Primitive { INT, FLOAT, CHAR, BOOL, VOID }
#include <string>
#include <vector>
#include <map>
#include <memory>

/* ============================================================
 *  Enum auxiliar para consultas rápidas (sem RTTI/dynamic_cast)
 * ============================================================*/
enum class TypeKind
{
    PRIMITIVE,
    RECORD,
    ARRAY,
    FUNCTION,
    NULL_T,
    UNKNOWN
};

class Type
{
public:
    virtual ~Type() = default;
    virtual std::string to_string() const = 0;
    virtual TypeKind kind() const = 0;

    /* Helper inline */
    bool is_primitive() const { return kind() == TypeKind::PRIMITIVE; }
    bool is_null() const { return kind() == TypeKind::NULL_T; }
    bool is_unknown() const { return kind() == TypeKind::UNKNOWN; }
};

/* ============================================================
 *  Tipos concretos
 * ============================================================*/
class PrimitiveType : public Type
{
public:
    Primitive p_type;
    explicit PrimitiveType(Primitive type) : p_type(type) {}

    std::string to_string() const override
    {
        switch (p_type)
        {
        case Primitive::INT:
            return "Int";
        case Primitive::FLOAT:
            return "Float";
        case Primitive::CHAR:
            return "Char";
        case Primitive::BOOL:
            return "Bool";
        case Primitive::VOID:
            return "Void";
        default:
            return "UnknownPrimitive";
        }
    }
    TypeKind kind() const override { return TypeKind::PRIMITIVE; }
};

class RecordType : public Type
{
public:
    std::string name;
    std::map<std::string, std::shared_ptr<Type>> fields;

    explicit RecordType(const std::string &n) : name(n) {}

    std::string to_string() const override { return name; }
    TypeKind kind() const override { return TypeKind::RECORD; }
};

class ArrayType : public Type
{
public:
    std::shared_ptr<Type> elem_type;

    explicit ArrayType(std::shared_ptr<Type> elem) : elem_type(std::move(elem)) {}

    std::string to_string() const override
    {
        return "Array[" + elem_type->to_string() + "]";
    }
    TypeKind kind() const override { return TypeKind::ARRAY; }
};

class FunctionType : public Type
{
public:
    std::vector<std::shared_ptr<Type>> param_types;
    std::vector<std::shared_ptr<Type>> return_types;

    std::string to_string() const override
    {
        std::string s = "(";
        for (size_t i = 0; i < param_types.size(); ++i)
        {
            s += param_types[i]->to_string();
            if (i + 1 < param_types.size())
                s += ", ";
        }
        s += ") -> (";
        for (size_t i = 0; i < return_types.size(); ++i)
        {
            s += return_types[i]->to_string();
            if (i + 1 < return_types.size())
                s += ", ";
        }
        s += ")";
        return s;
    }
    TypeKind kind() const override { return TypeKind::FUNCTION; }
};

/* Tipo literal “null” — pode unificar com RECORD ou ARRAY */
class NullType : public Type
{
public:
    std::string to_string() const override { return "null"; }
    TypeKind kind() const override { return TypeKind::NULL_T; }
};

/* Placeholder para inferência — usado enquanto o tipo real é desconhecido */
class UnknownType : public Type
{
public:
    std::string to_string() const override { return "‹unknown›"; }
    TypeKind kind() const override { return TypeKind::UNKNOWN; }
};

#endif /* LANG_TYPE_HPP */
