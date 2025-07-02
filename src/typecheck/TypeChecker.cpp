#include "TypeChecker.hpp"
#include "../ast/AST.hpp"
#include <stdexcept>
#include <memory>
#include <sstream>

/* =============================================================
 *  Helper para mensagens
 * ===========================================================*/
static std::string idx_str(std::size_t i)
{
    std::ostringstream os;
    os << "[" << i << "]";
    return os.str();
}

// --- Construtor e Métodos Auxiliares ---

TypeChecker::TypeChecker()
{
    push_scope();
}

void TypeChecker::push_scope()
{
    variable_types.emplace_back();
}

void TypeChecker::pop_scope()
{
    if (!variable_types.empty())
    {
        variable_types.pop_back();
    }
}

void TypeChecker::add_variable(const std::string &name, std::shared_ptr<Type> type)
{
    if (!variable_types.empty())
    {
        // Verifica se a variável já foi declarada no escopo atual
        if (variable_types.back().count(name))
        {
            throw std::runtime_error("Erro Semântico: Variável '" + name + "' já declarada neste escopo.");
        }
        variable_types.back()[name] = type;
    }
}

std::shared_ptr<Type> TypeChecker::get_variable_type(const std::string &name)
{
    for (auto it = variable_types.rbegin(); it != variable_types.rend(); ++it)
    {
        if (it->count(name))
        {
            return it->at(name);
        }
    }
    return nullptr;
}

std::shared_ptr<Type> TypeChecker::type_from_node(TypeNode *node)
{
    // Verificação de segurança
    if (!node)
    {
        throw std::logic_error("type_from_node foi chamado com um TypeNode nulo.");
    }

    // --- Lógica Corrigida ---

    // 1. Primeiro, verifica se o nó representa um array.
    if (node->is_array)
    {
        // Se for um array, chama a função recursivamente para descobrir o tipo do elemento.
        std::shared_ptr<Type> element_type = type_from_node(node->element_type);

        // Cria e retorna o tipo semântico 'ArrayType', que encapsula o tipo do elemento.
        return std::make_shared<ArrayType>(element_type);
    }

    // 2. Se não for um array, usa a lógica antiga para tipos base.
    if (node->is_primitive)
    {
        return std::make_shared<PrimitiveType>(node->p_type);
    }
    else
    {
        if (!record_types.count(node->user_type_name))
        {
            throw std::runtime_error("Erro de Tipo: Tipo '" + node->user_type_name + "' não definido.");
        }
        return record_types[node->user_type_name];
    }
}

// --- Ponto de Entrada ---

void TypeChecker::check(ProgramNode *ast)
{
    if (ast)
        ast->accept(this);
}

// --- Implementação dos Métodos Visit() ---

void TypeChecker::visit(ProgramNode *node)
{
    // Passadas de registro...
    for (Node *def : node->definitions)
    {
        if (auto data_def = dynamic_cast<DataDefNode *>(def))
        {
            data_def->accept(this);
        }
    }
    for (Node *def : node->definitions)
    {
        if (auto fun_def = dynamic_cast<FunDefNode *>(def))
        {
            fun_def->accept(this);
        }
    }
    // Passada de verificação dos corpos
    for (Node *def : node->definitions)
    {
        if (auto fun_def = dynamic_cast<FunDefNode *>(def))
        {
            current_function_type = function_types[fun_def->name];
            push_scope();
            for (const auto &param : fun_def->params)
            {
                add_variable(param.name, type_from_node(param.type));
            }
            fun_def->body->accept(this);
            pop_scope();
        }
    }
}

void TypeChecker::visit(DataDefNode *node)
{
    if (record_types.count(node->name))
    {
        throw std::runtime_error("Erro Semântico: Tipo '" + node->name + "' já definido.");
    }
    auto rec_type = std::make_shared<RecordType>(node->name);
    for (VarDeclNode *field : node->fields)
    {
        rec_type->fields[field->name] = type_from_node(field->type);
    }
    record_types[node->name] = rec_type;
}

void TypeChecker::visit(FunDefNode *node)
{
    if (function_types.count(node->name))
    {
        throw std::runtime_error("Erro Semântico: Função '" + node->name + "' já definida.");
    }
    auto func_type = std::make_shared<FunctionType>();
    for (const auto &param : node->params)
    {
        func_type->param_types.push_back(type_from_node(param.type));
    }
    for (TypeNode *ret_type_node : node->return_types)
    {
        func_type->return_types.push_back(type_from_node(ret_type_node));
    }
    function_types[node->name] = func_type;
}

void TypeChecker::visit(BlockCmdNode *node)
{
    push_scope();
    for (Command *cmd : node->commands)
    {
        cmd->accept(this);
    }
    pop_scope();
}

void TypeChecker::visit(VarDeclNode *node)
{
    add_variable(node->name, type_from_node(node->type));
}

void TypeChecker::visit(AssignCmdNode *node)
{
    /* 1. Verifica se é um lvalue simples (acesso a variável, ex: x = ...) */
    if (auto *va = dynamic_cast<VarAccessNode *>(node->lvalue))
    {
        /* ── a) Se a variável ainda não existe, cria um placeholder 'Unknown' para inferência. */
        if (!get_variable_type(va->name))
        {
            add_variable(va->name, std::make_shared<UnknownType>());
        }

        /* ── b) Avalia o tipo do lado direito (RHS) da atribuição. */
        node->expr->accept(this);
        auto rhs_type = last_inferred_type;

        /* ── c) Obtém o tipo do lado esquerdo (LHS) que já está no contexto. */
        auto lhs_type = get_variable_type(va->name);

        /* ── d) Se o tipo do LHS é 'Unknown', realiza a unificação. */
        if (lhs_type->is_unknown())
        {
            // CORREÇÃO: Atualiza o ponteiro no mapa de tipos do escopo atual.
            // A abordagem anterior (*lhs_type = *rhs_type) causava "object slicing"
            // e não alterava o tipo dinâmico do objeto, que permanecia 'UnknownType'.
            variable_types.back()[va->name] = rhs_type;
            return;
        }

        /* ── e) Se o LHS já tem um tipo, verifica a compatibilidade da atribuição. */

        // Permite atribuir `null` a qualquer tipo não primitivo (registros, arrays).
        if (rhs_type->is_null() && !lhs_type->is_primitive())
        {
            return;
        }

        // Se os tipos não forem idênticos, lança um erro.
        // Uma melhoria futura poderia ser permitir atribuição de Int para Float (promoção).
        if (lhs_type->to_string() != rhs_type->to_string())
        {
            throw std::runtime_error("Erro de Tipo: não é possível atribuir " + rhs_type->to_string() + " a variável do tipo " + lhs_type->to_string() + ".");
        }
        return;
    }

    /* 2. Lida com lvalues mais complexos (ex: p.x = ..., arr[i] = ...) */
    node->lvalue->accept(this);
    auto lhs_type = last_inferred_type;

    node->expr->accept(this);
    auto rhs_type = last_inferred_type;

    // Permite atribuir `null` a qualquer tipo não primitivo.
    if (rhs_type->is_null() && !lhs_type->is_primitive())
    {
        return;
    }

    // Verifica se os tipos são compatíveis para a atribuição.
    if (lhs_type->to_string() != rhs_type->to_string())
    {
        throw std::runtime_error("Erro de Tipo: não é possível atribuir " + rhs_type->to_string() + " a " + lhs_type->to_string() + ".");
    }
}

void TypeChecker::visit(BinaryOpNode *node)
{
    /* Tipos dos operandos ------------------------------------ */
    node->left->accept(this);
    auto L = last_inferred_type;

    node->right->accept(this);
    auto R = last_inferred_type;

    /* Helpers ------------------------------------------------- */
    auto is_prim = [](std::shared_ptr<Type> t)
    {
        return t && t->kind() == TypeKind::PRIMITIVE;
    };
    auto as_prim = [](std::shared_ptr<Type> t) -> Primitive
    {
        return std::static_pointer_cast<PrimitiveType>(t)->p_type;
    };
    auto is_num = [&](std::shared_ptr<Type> t)
    {
        return is_prim(t) &&
               (as_prim(t) == Primitive::INT || as_prim(t) == Primitive::FLOAT);
    };

    char op = node->op;

    /* ---------------------------------------------------------
     *  Operadores aritméticos (+, -, *, /, %)
     * --------------------------------------------------------*/
    if (op == '+' || op == '-' || op == '*' || op == '/' || op == '%')
    {
        if (!is_num(L) || !is_num(R))
            throw std::runtime_error(
                "Erro de Tipo: operador '" + std::string(1, op) +
                "' requer Int/Float, mas recebeu (" +
                L->to_string() + ", " + R->to_string() + ").");

        /* promoção Int → Float */
        if (as_prim(L) == Primitive::FLOAT || as_prim(R) == Primitive::FLOAT)
            last_inferred_type = std::make_shared<PrimitiveType>(Primitive::FLOAT);
        else
            last_inferred_type = std::make_shared<PrimitiveType>(Primitive::INT);
        return;
    }

    /* ---------------------------------------------------------
     *  Operador relacional (<)
     * --------------------------------------------------------*/
    if (op == '<' || op == '>')
    {
        if (!is_num(L) || !is_num(R))
            throw std::runtime_error(
                "Erro de Tipo: operador '<' requer Int/Float, mas recebeu (" +
                L->to_string() + ", " + R->to_string() + ").");
        last_inferred_type = std::make_shared<PrimitiveType>(Primitive::BOOL);
        return;
    }

    /* ---------------------------------------------------------
     *  Operador lógico '&&'
     * --------------------------------------------------------*/
    if (op == '&')
    {
        if (!is_prim(L) || !is_prim(R) ||
            as_prim(L) != Primitive::BOOL || as_prim(R) != Primitive::BOOL)
            throw std::runtime_error(
                "Erro de Tipo: operador '&&' requer Bool, mas recebeu (" +
                L->to_string() + ", " + R->to_string() + ").");
        last_inferred_type = std::make_shared<PrimitiveType>(Primitive::BOOL);
        return;
    }

    /* ---------------------------------------------------------
     *  Igualdade (==) e diferença (!=)
     * --------------------------------------------------------*/
    if (op == '=' || op == 'n')
    {
        /*  Int ↔ Float é permitido; null ↔ registro/array também.  */
        bool ok =
            (L->to_string() == R->to_string()) || // mesmos tipos
            (is_num(L) && is_num(R)) ||           // promoção num.
            (L->is_null() && !R->is_primitive()) ||
            (R->is_null() && !L->is_primitive());

        if (!ok)
            throw std::runtime_error(
                "Erro de Tipo: operador '" + std::string(1, op) +
                "' aplicado a tipos incompatíveis (" +
                L->to_string() + ", " + R->to_string() + ").");

        last_inferred_type = std::make_shared<PrimitiveType>(Primitive::BOOL);
        return;
    }

    throw std::logic_error("BinaryOpNode: operador desconhecido.");
}

void TypeChecker::visit(IntLiteral *node)
{
    last_inferred_type = std::make_shared<PrimitiveType>(Primitive::INT);
}
void TypeChecker::visit(FloatLiteralNode *node)
{
    last_inferred_type = std::make_shared<PrimitiveType>(Primitive::FLOAT);
}
void TypeChecker::visit(CharLiteralNode *node)
{
    last_inferred_type = std::make_shared<PrimitiveType>(Primitive::CHAR);
}
void TypeChecker::visit(BoolLiteralNode *node)
{
    last_inferred_type = std::make_shared<PrimitiveType>(Primitive::BOOL);
}

void TypeChecker::visit(VarAccessNode *node)
{
    auto type = get_variable_type(node->name);
    if (!type)
    {
        throw std::runtime_error("Erro Semântico: Variável '" + node->name + "' não declarada.");
    }
    last_inferred_type = type;
}

void TypeChecker::visit(PrintCmd *node)
{
    node->expr->accept(this);
}

void TypeChecker::visit(IfCmdNode *node)
{
    node->condition->accept(this);
    if (last_inferred_type->to_string() != "Bool")
    {
        throw std::runtime_error("Erro de Tipo: Condição do 'if' deve ser do tipo Bool, mas recebeu " + last_inferred_type->to_string() + ".");
    }
    node->then_branch->accept(this);
    if (node->else_branch)
    {
        node->else_branch->accept(this);
    }
}
// Implementações vazias ou simples para os demais nós
void TypeChecker::visit(FunCallNode *node)
{
    // 1. Encontra a assinatura da função no contexto.
    if (!function_types.count(node->name))
    {
        throw std::runtime_error("Erro Semântico: Função '" + node->name + "' não definida.");
    }
    auto func_type = function_types.at(node->name);

    // 2. Checa se o número de argumentos passados corresponde ao esperado.
    if (func_type->param_types.size() != node->args.size())
    {
        throw std::runtime_error("Erro Semântico: Aridade incorreta na chamada de '" + node->name + "'.");
    }

    // 3. Checa o tipo de cada argumento.
    for (std::size_t i = 0; i < node->args.size(); ++i)
    {
        node->args[i]->accept(this);
        auto arg_type = last_inferred_type;
        auto param_type = func_type->param_types[i];

        // Compara os tipos (uma melhoria seria permitir promoção de Int para Float).
        if (arg_type->to_string() != param_type->to_string())
        {
            throw std::runtime_error("Erro de Tipo: Argumento " + idx_str(i) + " incompatível na chamada de '" + node->name + "'.");
        }
    }

    // 4. Checa o tipo da expressão do índice de retorno (ex: o `1` em `[1]`).
    node->return_index->accept(this);
    if (last_inferred_type->to_string() != "Int")
    {
        throw std::runtime_error("Erro de Tipo: O índice de retorno de uma função deve ser do tipo Int.");
    }

    // 5. Tenta obter o valor do índice se ele for uma constante literal.
    if (auto *idx_literal = dynamic_cast<IntLiteral *>(node->return_index))
    {
        int idx = idx_literal->value;

        // 6. Verifica se o índice é válido (não está fora dos limites).
        if (idx < 0 || idx >= func_type->return_types.size())
        {
            throw std::runtime_error("Erro Semântico: Índice de retorno " + std::to_string(idx) + " fora dos limites para a função '" + node->name + "'.");
        }

        // 7. SUCESSO: O tipo da expressão toda é o tipo do valor de retorno selecionado.
        last_inferred_type = func_type->return_types[idx];
    }
    else
    {
        // Se o índice não for um literal (ex: uma variável como `[i]`), não podemos
        // determinar o tipo em tempo de compilação. A abordagem mais segura é lançar um erro.
        throw std::runtime_error("Erro Semântico: Para checagem estática de tipo, o índice de retorno de uma função deve ser um literal inteiro constante.");
    }
}

void TypeChecker::visit(FunCallCmdNode *n)
{
    if (!function_types.count(n->name))
        throw std::runtime_error("Função '" + n->name + "' não definida.");
    auto f = function_types[n->name];

    // ----- argumentos -----
    if (f->param_types.size() != n->args.size())
        throw std::runtime_error("Aridade incorreta na chamada de '" + n->name + "'.");
    for (std::size_t i = 0; i < n->args.size(); ++i)
    {
        n->args[i]->accept(this);
        auto arg = last_inferred_type;
        auto param = f->param_types[i];
        if (param->is_unknown())
            *param = *arg;
        else if (arg->is_unknown())
            *arg = *param;
        else if (arg->to_string() != param->to_string())
            throw std::runtime_error("Tipo do argumento " + idx_str(i) + " incompatível na chamada de '" + n->name + "'.");
    }

    // ----- captura -----
    if (f->return_types.size() != n->lvalues.size())
        throw std::runtime_error("Captura de retorno não coincide com quantidade de valores de saída de '" + n->name + "'.");

    for (std::size_t i = 0; i < n->lvalues.size(); ++i)
    {
        n->lvalues[i]->accept(this); // tipo do lvalue colocado em last_inferred_type
        auto lhs = last_inferred_type;
        auto rhs = f->return_types[i];

        if (lhs->is_unknown())
            *lhs = *rhs;
        else if (rhs->is_unknown())
            *rhs = *lhs;
        else if (lhs->to_string() != rhs->to_string())
            throw std::runtime_error("Tipo incompatível ao capturar retorno " + idx_str(i) + " de '" + n->name + "'.");
    }
}
void TypeChecker::visit(ReturnCmdNode *n)
{
    if (!current_function_type)
        throw std::runtime_error("'return' fora de função.");

    if (current_function_type->return_types.size() != n->expressions.size())
        throw std::runtime_error("Retorno tem quantidade de valores diferente do declarado.");

    for (std::size_t i = 0; i < n->expressions.size(); ++i)
    {
        n->expressions[i]->accept(this);
        auto expr = last_inferred_type;
        auto expected = current_function_type->return_types[i];

        if (expected->is_unknown())
            *expected = *expr;
        else if (expr->is_null() && !expected->is_primitive())
            continue;
        else if (expr->to_string() != expected->to_string())
            throw std::runtime_error("Tipo de retorno " + idx_str(i) + " incompatível (esperado " + expected->to_string() + ", obteve " + expr->to_string() + ").");
    }
}
void TypeChecker::visit(UnaryOpNode *node)
{
    /* 1. Visita o operando ------------------------------------ */
    node->expr->accept(this);
    std::shared_ptr<Type> op_ty = last_inferred_type; // nunca nullptr

    /* 2. Verifica o operador ---------------------------------- */
    switch (node->op)
    {
    /* ---------- negação lógica '!' -------------------------- */
    case '!':
    {
        auto prim = std::dynamic_pointer_cast<PrimitiveType>(op_ty);
        if (!prim || prim->p_type != Primitive::BOOL)
        {
            throw std::runtime_error(
                "Erro de Tipo: operador '!' requer Bool, mas recebeu " + op_ty->to_string() + ".");
        }
        last_inferred_type = op_ty; // resultado continua Bool
        break;
    }

    /* ---------- negação aritmética '-' ---------------------- */
    case '-':
    {
        auto prim = std::dynamic_pointer_cast<PrimitiveType>(op_ty);
        if (!prim ||
            (prim->p_type != Primitive::INT && prim->p_type != Primitive::FLOAT))
        {
            throw std::runtime_error(
                "Erro de Tipo: operador unário '-' só pode ser aplicado a "
                "Int ou Float, mas recebeu " +
                op_ty->to_string() + ".");
        }
        last_inferred_type = op_ty; // resultado continua Int ou Float
        break;
    }

    default:
        throw std::logic_error("UnaryOpNode: operador desconhecido.");
    }
}
void TypeChecker::visit(ReadCmdNode *node) { /* TODO */ }
void TypeChecker::visit(IterateCmdNode *node) { /* TODO */ }
void TypeChecker::visit(NewExprNode *node)
{
    // 1. Começa com o tipo base (ex: Int, Ponto)
    std::shared_ptr<Type> current_type = type_from_node(node->base_type);

    // 2. Para cada dimensão encontrada (ex: [5], [10]), "envolve" o tipo
    //    atual com um ArrayType.
    for (auto dim_expr : node->dims)
    {
        // Valida que a expressão da dimensão é um Int
        dim_expr->accept(this);
        if (last_inferred_type->to_string() != "Int")
        {
            throw std::runtime_error("Erro de Tipo: Dimensões de um array devem ser do tipo Int.");
        }
        // Envolve o tipo atual
        current_type = std::make_shared<ArrayType>(current_type);
    }

    // O tipo final da expressão é o tipo base com todas as dimensões
    last_inferred_type = current_type;
}
void TypeChecker::visit(FieldAccessNode *node)
{
    /* avalia o tipo da expressão antes do ponto --------------- */
    node->record_expr->accept(this);
    auto rec_type = last_inferred_type;

    if (!rec_type || rec_type->kind() != TypeKind::RECORD)
        throw std::runtime_error("Tentativa de acessar campo em algo que não é registro");

    auto rec = std::static_pointer_cast<RecordType>(rec_type);

    if (!rec->fields.count(node->field_name))
        throw std::runtime_error("Campo '" + node->field_name +
                                 "' não existe em '" + rec->name + "'");

    /* tipo resultante do acesso é o tipo do campo -------------- */
    last_inferred_type = rec->fields[node->field_name];
}

void TypeChecker::visit(TypeNode *node) { /* Não faz nada */ }

void TypeChecker::visit(NullLiteralNode * /*node*/)
{
    last_inferred_type = std::make_shared<NullType>(); // ou UnknownType
}

void TypeChecker::visit(ArrayAccessNode *node)
{
    // Visita a expressão base do array para descobrir seu tipo
    node->array_expr->accept(this);
    auto base_type = last_inferred_type;

    // Checa se o tipo base é de fato um array
    auto array_type = std::dynamic_pointer_cast<ArrayType>(base_type);
    if (!array_type)
    {
        throw std::runtime_error("Erro de Tipo: Tentativa de indexar um tipo que não é um array ('" + base_type->to_string() + "').");
    }

    // Checa se a expressão do índice é um Int
    node->index_expr->accept(this);
    if (last_inferred_type->to_string() != "Int")
    {
        throw std::runtime_error("Erro de Tipo: O índice de um array deve ser do tipo Int.");
    }

    // O tipo da expressão toda é o tipo do elemento do array
    last_inferred_type = array_type->elem_type;
}
