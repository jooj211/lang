#include "Interpreter.hpp"
#include "ReturnSignal.hpp"
#include "../ast/AST.hpp"
#include "../runtime/Values.hpp"
#include <iostream>
#include <cstring>
#include <cstdio>

Value *Interpreter::create_default_value(TypeNode *type)
{
    std::set<std::string> visited;
    return create_default_value(type, visited);
}

Value *Interpreter::create_default_value(TypeNode *type, std::set<std::string> &visited_records)
{
    if (!type || type->is_array)
    {
        return nullptr;
    }

    if (type->is_primitive)
    {
        Value *val = nullptr;
        switch (type->p_type)
        {
        case Primitive::INT:
            val = new IntValue(0);
            break;
        case Primitive::FLOAT:
            val = new FloatValue(0.0f);
            break;
        case Primitive::CHAR:
            val = new CharValue('\0');
            break;
        case Primitive::BOOL:
            val = new BoolValue(false);
            break;
        default:
            return nullptr;
        }
        if (val)
            value_pool.push_back(val);
        return val;
    }
    else
    { // É um tipo de registro
        std::string type_name = type->user_type_name;

        // ==================================================================
        // ====> COLOQUE A MENSAGEM DE DEBUG EXATAMENTE AQUI <====
        // ==================================================================
        std::cerr << "[DEBUG] Tentando criar valor padrão para o tipo de registro: '" << type_name << "'.\n";

        // VERIFICAÇÃO DE RECURSÃO
        if (visited_records.count(type_name))
        {
            std::cerr << "    └─ DETECTADO CICLO! O tipo '" << type_name << "' já está sendo criado. Retornando 'null' para quebrar a recursão.\n";
            return nullptr;
        }
        // ==================================================================

        if (data_types.count(type_name))
        {
            visited_records.insert(type_name);
            DataDefNode *def = data_types.at(type_name);
            auto *record = new RecordValue(type_name);
            value_pool.push_back(record);

            for (VarDeclNode *field : def->fields)
            {
                record->fields[field->name] = create_default_value(field->type, visited_records);
            }

            visited_records.erase(type_name);
            return record;
        }
    }
    return nullptr;
}

/**
 * @brief Função recursiva para alocar arrays e matrizes de N dimensões.
 * @param base_elem_type O tipo base dos elementos (ex: Int, Ponto).
 * @param dims O vetor com as expressões de tamanho para cada dimensão.
 * @param dim_index O índice da dimensão atual que está sendo alocada.
 */

Value *Interpreter::create_nested_array(TypeNode *base_elem_type,
                                        const std::vector<Expression *> &dims,
                                        size_t dim_index)
{
    if (dim_index >= dims.size())
    {
        return create_default_value(base_elem_type);
    }

    dims[dim_index]->accept(this);
    auto *size_val = dynamic_cast<IntValue *>(last_value);
    if (!size_val || size_val->value < 0)
    {
        throw std::runtime_error("Erro de Execução: Dimensão de array inválida.");
    }
    size_t size = size_val->value;

    auto *arr_val = new ArrayValue();
    value_pool.push_back(arr_val);
    arr_val->elements.resize(size);

    for (size_t i = 0; i < size; ++i)
    {
        arr_val->elements[i] = create_nested_array(base_elem_type, dims, dim_index + 1);
    }

    return arr_val;
}
// ... (todo o topo do arquivo: destrutor, push/pop_scope, etc. continua igual) ...
Interpreter::~Interpreter()
{
    for (Value *v : value_pool)
    {
        delete v;
    }
}
void Interpreter::push_scope() { memory_stack.emplace_back(); }
void Interpreter::pop_scope()
{
    if (!memory_stack.empty())
    {
        memory_stack.pop_back();
    }
}
void Interpreter::set_variable(const std::string &name, Value *value, bool is_decl)
{
    if (is_decl && !memory_stack.empty())
    {
        memory_stack.back()[name] = value;
    }
}

void Interpreter::update_variable(const std::string &name, Value *new_value)
{
    for (auto it = memory_stack.rbegin(); it != memory_stack.rend(); ++it)
    {
        if (it->count(name))
        {
            Value *old_value = it->at(name);

            // Se for um tipo primitivo, copiamos o valor interno.
            if (auto i_old = dynamic_cast<IntValue *>(old_value))
            {
                if (auto i_new = dynamic_cast<IntValue *>(new_value))
                {
                    i_old->value = i_new->value;
                    return;
                }
            }
            if (auto f_old = dynamic_cast<FloatValue *>(old_value))
            {
                if (auto f_new = dynamic_cast<FloatValue *>(new_value))
                {
                    f_old->value = f_new->value;
                    return;
                }
            }
            if (auto c_old = dynamic_cast<CharValue *>(old_value))
            {
                if (auto c_new = dynamic_cast<CharValue *>(new_value))
                {
                    c_old->value = c_new->value;
                    return;
                }
            }
            if (auto b_old = dynamic_cast<BoolValue *>(old_value))
            {
                if (auto b_new = dynamic_cast<BoolValue *>(new_value))
                {
                    b_old->value = b_new->value;
                    return;
                }
            }

            // Para todos os outros casos (registros, arrays, null), trocamos o ponteiro.
            (*it)[name] = new_value;
            return;
        }
    }
}

Value *Interpreter::get_variable(const std::string &name)
{
    for (auto it = memory_stack.rbegin(); it != memory_stack.rend(); ++it)
    {
        if (it->count(name))
        {
            return it->at(name);
        }
    }
    return nullptr;
}

void Interpreter::interpret(ProgramNode *ast)
{
    push_scope();
    if (ast)
        ast->accept(this);
    pop_scope();
}

// ============ Implementação dos Métodos visit() ============

// ------------------------------------------------------------------
static TypeNode *clone_type(TypeNode *src) // utilitário rápido
{
    /*  ❱❱❱  CLONE RASO  ❰❰❰
       Para esse caso só precisamos copiar os flags
       (is_primitive, p_type, user_type_name, etc.).
       Se futuramente TypeNode ganhar sub-estruturas mais complexas,
       transforme isto num clone profundo.                       */
    return new TypeNode(*src);
}

void Interpreter::visit(ProgramNode *node)
{
    /* 1. registra funções, tipos, etc. — já existia */
    for (Node *def : node->definitions)
        def->accept(this);

    /* 2. procura por “main” ------------------------------------------------ */
    auto it = functions.find("main");
    if (it == functions.end())
        return; // não há main()

    FunDefNode *mdef = it->second;

    /* 3. constrói vetor de argumentos se main espera algo ------------------ */
    auto *arg_vec = new std::vector<Expression *>;
    if (mdef->params.size() == 1) // um único parâmetro
    {
        TypeNode *ptype = mdef->params[0].type;

        // se for registro ou array, geramos `new Tipo`
        if (!ptype->is_primitive)
        {
            arg_vec->push_back(
                new NewExprNode(clone_type(ptype), /*array_size=*/nullptr));
        }
        else
        { // primitivo: coloca valor neutro literal
            switch (ptype->p_type)
            {
            case Primitive::INT:
                arg_vec->push_back(new IntLiteral{0});
                break;
            case Primitive::FLOAT:
                arg_vec->push_back(new FloatLiteralNode{0.f});
                break;
            case Primitive::BOOL:
                arg_vec->push_back(new BoolLiteralNode{false});
                break;
            case Primitive::CHAR:
                arg_vec->push_back(new CharLiteralNode{'\0'});
                break;
            default:
                break;
            }
        }
    }

    /* 4. dispara a execução de main ---------------------------------------- */
    visit(new FunCallCmdNode(strdup("main"), arg_vec,
                             new std::vector<Expression *>));
}

// MODIFICADO: Registra a definição do tipo 'data'
void Interpreter::visit(DataDefNode *node)
{
    data_types[node->name] = node;
}

/**
 * @brief Visita um nó de alocação 'new', usando os métodos auxiliares.
 */
// Em Interpreter.cpp
void Interpreter::visit(NewExprNode *node)
{
    // Caso 1: Alocação de objeto único (ex: new AFD ou new Transition)
    if (node->dims.empty())
    {
        if (node->base_type->is_primitive)
        {
            throw std::runtime_error("Erro de Execução: 'new' em tipo primitivo deve ser uma alocação de array.");
        }
        last_value = create_default_value(node->base_type);
        return;
    }

    // Caso 2: Alocação de Array ou Matriz (ex: new Int[5] ou new Transition[][numStates])
    // A lógica abaixo foi adaptada para a sintaxe do professor.

    // A sintaxe `new T[][size]` é incomum. Nossa interpretação é que o tamanho
    // da dimensão mais externa está sendo especificado por último.
    // Vamos procurar pela última expressão de dimensão que não seja nula.
    Expression *size_expr = nullptr;
    for (auto it = node->dims.rbegin(); it != node->dims.rend(); ++it)
    {
        if (*it != nullptr)
        {
            size_expr = *it;
            break;
        }
    }

    // Se nenhuma dimensão foi especificada (ex: new T[][]), é um erro em tempo de execução.
    if (!size_expr)
    {
        throw std::runtime_error("Erro de Execução: Alocação de array requer pelo menos um tamanho de dimensão.");
    }

    // Avalia a expressão de tamanho que encontramos.
    size_expr->accept(this);
    auto *size_val = dynamic_cast<IntValue *>(last_value);
    if (!size_val || size_val->value < 0)
    {
        throw std::runtime_error("Erro de Execução: Tamanho do array inválido.");
    }

    size_t size = size_val->value;

    // Cria o array externo com o tamanho encontrado e o preenche com `nullptr`.
    // As dimensões internas serão alocadas depois (ex: em setNumTransitions).
    auto *arr_val = new ArrayValue();
    value_pool.push_back(arr_val);
    arr_val->elements.resize(size, nullptr); // Redimensiona e preenche com ponteiros nulos

    last_value = arr_val;
}
void Interpreter::visit(FunDefNode *node) { functions[node->name] = node; }
void Interpreter::visit(FunCallNode *node)
{
    if (!functions.count(node->name))
    {
        return;
    }
    FunDefNode *func_def = functions[node->name];
    std::vector<Value *> evaluated_args;
    for (Expression *arg_expr : node->args)
    {
        arg_expr->accept(this);
        evaluated_args.push_back(last_value);
    }
    if (evaluated_args.size() != func_def->params.size())
    {
        last_value = nullptr;
        return;
    }
    push_scope();
    for (size_t i = 0; i < func_def->params.size(); ++i)
    {
        set_variable(func_def->params[i].name, evaluated_args[i], true);
    }
    try
    {
        func_def->body->accept(this);
        last_value = nullptr;
    }
    catch (const ReturnSignal &ret)
    {
        if (!ret.values.empty())
        {
            node->return_index->accept(this);
            if (auto idx_val = dynamic_cast<IntValue *>(last_value))
            {
                if (idx_val->value >= 0 && idx_val->value < ret.values.size())
                {
                    last_value = ret.values[idx_val->value];
                }
                else
                {
                    last_value = nullptr;
                }
            }
            else
            {
                last_value = nullptr;
            }
        }
        else
        {
            last_value = nullptr;
        }
    }
    pop_scope();
}
void Interpreter::visit(FunCallCmdNode *node)
{
    if (!functions.count(node->name))
    {
        return;
    }
    FunDefNode *func_def = functions[node->name];
    std::vector<Value *> evaluated_args;
    for (Expression *arg_expr : node->args)
    {
        arg_expr->accept(this);
        evaluated_args.push_back(last_value);
    }
    if (evaluated_args.size() != func_def->params.size())
    {
        return;
    }
    push_scope();
    for (size_t i = 0; i < func_def->params.size(); ++i)
    {
        set_variable(func_def->params[i].name, evaluated_args[i], true);
    }
    try
    {
        func_def->body->accept(this);
    }
    catch (const ReturnSignal &ret)
    {
        if (node->lvalues.size() > 0 && node->lvalues.size() == ret.values.size())
        {
            for (size_t i = 0; i < node->lvalues.size(); ++i)
            {
                if (auto var_access = dynamic_cast<VarAccessNode *>(node->lvalues[i]))
                {
                    update_variable(var_access->name, ret.values[i]);
                }
            }
        }
    }
    pop_scope();
}
void Interpreter::visit(FieldAccessNode *node)
{
    node->record_expr->accept(this);
    auto *rec_val = dynamic_cast<RecordValue *>(last_value);

    if (!rec_val)
    {
        last_value = nullptr;
        return;
    }

    if (!rec_val->fields.count(node->field_name))
    {
        last_value = nullptr; // campo inexistente
        return;
    }

    last_value = rec_val->fields[node->field_name];
}
void Interpreter::visit(ReturnCmdNode *node)
{
    ReturnSignal ret_signal;
    for (Expression *expr : node->expressions)
    {
        expr->accept(this);
        ret_signal.values.push_back(last_value);
    }
    throw ret_signal;
}
void Interpreter::visit(BlockCmdNode *node)
{
    push_scope();
    for (Command *cmd : node->commands)
    {
        cmd->accept(this);
    }
    pop_scope();
}

void Interpreter::visit(VarDeclNode *node)
{
    Value *default_value = nullptr;

    // Caso A: O tipo da variável é primitivo (Int, Float, etc.)
    if (node->type->is_primitive)
    {
        switch (node->type->p_type)
        {
        case Primitive::INT:
            default_value = new IntValue(0);
            break;
        case Primitive::BOOL:
            default_value = new BoolValue(false);
            break;
        case Primitive::FLOAT:
            default_value = new FloatValue(0.0f);
            break;
        case Primitive::CHAR:
            default_value = new CharValue('\0');
            break;
        }
        // Adiciona o valor primitivo recém-criado ao pool de memória.
        if (default_value)
        {
            value_pool.push_back(default_value);
        }
    }
    // Caso B: O tipo da variável é um registro definido por 'data'
    else
    {
        std::string type_name = node->type->user_type_name;
        if (data_types.count(type_name))
        {
            DataDefNode *def = data_types[type_name];
            RecordValue *record = new RecordValue(type_name);

            // Adiciona o registro ao pool APENAS UMA VEZ, no momento da criação.
            value_pool.push_back(record);

            // Inicializa todos os campos do registro com seus valores padrão.
            for (VarDeclNode *field : def->fields)
            {
                Value *field_val = nullptr;
                if (field->type->is_primitive)
                {
                    switch (field->type->p_type)
                    {
                    case Primitive::INT:
                        field_val = new IntValue(0);
                        break;
                    case Primitive::BOOL:
                        field_val = new BoolValue(false);
                        break;
                    case Primitive::FLOAT:
                        field_val = new FloatValue(0.0f);
                        break;
                    case Primitive::CHAR:
                        field_val = new CharValue('\0');
                        break;
                    }
                    if (field_val)
                    {
                        value_pool.push_back(field_val); // Adiciona o valor do campo ao pool.
                    }
                }
                record->fields[field->name] = field_val; // Associa campo ao valor (ou nullptr se for outro registro)
            }
            default_value = record;
        }
    }

    // Associa o nome da variável ao seu valor padrão (seja um primitivo, um registro ou nulo).
    set_variable(node->name, default_value, true);
}

void Interpreter::visit(AssignCmdNode *node)
{
    // 1. Avalia a expressão do lado direito (RHS) para obter o valor.
    node->expr->accept(this);
    Value *rhs_value = last_value;

    // 2. Determina o tipo do L-Value e realiza a atribuição.

    // --- CASO 1: Atribuição a uma variável simples (ex: x = 10) ---
    if (auto *va = dynamic_cast<VarAccessNode *>(node->lvalue))
    {
        // Se a variável já existe na memória, atualiza seu valor.
        if (get_variable(va->name))
        {
            update_variable(va->name, rhs_value);
        }
        else
        {
            // Se não existe (inferência de tipo), cria no escopo atual.
            set_variable(va->name, rhs_value, /*is_decl=*/true);
        }
    }
    // --- CASO 2: Atribuição a um campo de registro (ex: last.next = no) ---
    else if (auto *fa = dynamic_cast<FieldAccessNode *>(node->lvalue))
    {
        // a. Avalia a expressão antes do ponto (ex: 'last') para obter o RecordValue.
        fa->record_expr->accept(this);
        auto *record = dynamic_cast<RecordValue *>(last_value);

        if (!record)
        {
            throw std::runtime_error("Erro de Execução: Tentativa de acesso a campo em algo que não é um registro.");
        }

        // b. Verifica se o campo existe no registro.
        if (record->fields.find(fa->field_name) == record->fields.end())
        {
            throw std::runtime_error("Erro de Execução: Campo '" + fa->field_name + "' não existe no tipo '" + record->type_name + "'.");
        }

        // c. Atualiza o ponteiro do campo para o novo valor.
        record->fields[fa->field_name] = rhs_value;
    }
    // --- CASO 3: Atribuição a um elemento de array (ex: arr[0] = 5) ---
    else if (auto *aa = dynamic_cast<ArrayAccessNode *>(node->lvalue))
    {
        // a. Avalia a expressão do array para obter o ArrayValue.
        aa->array_expr->accept(this);
        auto *arr_val = dynamic_cast<ArrayValue *>(last_value);
        if (!arr_val)
        {
            throw std::runtime_error("Erro de Execução: Tentativa de acesso por índice em algo que não é um array.");
        }

        // b. Avalia a expressão do índice para obter o valor inteiro.
        aa->index_expr->accept(this);
        auto *idx_val = dynamic_cast<IntValue *>(last_value);
        if (!idx_val)
        {
            throw std::runtime_error("Erro de Execução: Índice de array deve ser um inteiro.");
        }

        int index = idx_val->value;
        if (index < 0 || (size_t)index >= arr_val->elements.size())
        {
            throw std::runtime_error("Erro de Execução: Índice de array fora dos limites.");
        }

        // c. Atualiza o ponteiro do elemento para o novo valor.
        arr_val->elements[index] = rhs_value;
    }
    // --- ERRO: Tipo de l-value não suportado ---
    else
    {
        throw std::runtime_error("Erro de Execução: Atribuição à esquerda para este tipo de expressão não é suportada.");
    }
}
void Interpreter::visit(PrintCmd *node)
{
    node->expr->accept(this);
    if (last_value)
    {
        last_value->print();
        std::cout << std::endl;
    }
}
void Interpreter::visit(ReadCmdNode *node)
{
    if (auto va = dynamic_cast<VarAccessNode *>(node->lvalue))
    {
        Value *tv = get_variable(va->name);
        if (!tv)
            return;
        if (auto ib = dynamic_cast<IntValue *>(tv))
        {
            std::cin >> ib->value;
        }
        else if (auto fb = dynamic_cast<FloatValue *>(tv))
        {
            std::cin >> fb->value;
        }
        else if (auto cb = dynamic_cast<CharValue *>(tv))
        {
            std::cin >> cb->value;
        }
    }
}
void Interpreter::visit(IfCmdNode *node)
{
    node->condition->accept(this);
    if (auto bv = dynamic_cast<BoolValue *>(last_value))
    {
        if (bv->value)
        {
            node->then_branch->accept(this);
        }
        else if (node->else_branch)
        {
            node->else_branch->accept(this);
        }
    }
}
void Interpreter::visit(IterateCmdNode *node)
{
    node->condition->accept(this);
    auto cv = dynamic_cast<IntValue *>(last_value);
    if (!cv)
        return;
    int n = cv->value;
    bool hlv = !node->loop_variable.empty();
    if (hlv)
        push_scope();
    for (int i = 0; i < n; ++i)
    {
        if (hlv)
        {
            Value *lvv = new IntValue(i);
            value_pool.push_back(lvv);
            set_variable(node->loop_variable, lvv, true);
        }
        node->body->accept(this);
    }
    if (hlv)
        pop_scope();
}
void Interpreter::visit(IntLiteral *node)
{
    last_value = new IntValue(node->value);
    value_pool.push_back(last_value);
}
void Interpreter::visit(FloatLiteralNode *node)
{
    last_value = new FloatValue(node->value);
    value_pool.push_back(last_value);
}
void Interpreter::visit(CharLiteralNode *node)
{
    last_value = new CharValue(node->value);
    value_pool.push_back(last_value);
}
void Interpreter::visit(BoolLiteralNode *node)
{
    last_value = new BoolValue(node->value);
    value_pool.push_back(last_value);
}
void Interpreter::visit(VarAccessNode *node) { last_value = get_variable(node->name); }
void Interpreter::visit(UnaryOpNode *node)
{
    node->expr->accept(this);

    switch (node->op)
    {
    case '!':
    {
        if (auto *bv = dynamic_cast<BoolValue *>(last_value))
        {
            last_value = new BoolValue(!bv->value);
            value_pool.push_back(last_value);
        }
        break;
    }
    case '-':
    {
        if (auto *iv = dynamic_cast<IntValue *>(last_value))
        {
            last_value = new IntValue(-iv->value);
        }
        else if (auto *fv = dynamic_cast<FloatValue *>(last_value))
        {
            last_value = new FloatValue(-fv->value);
        }
        else
        {
            throw std::runtime_error("Operador unário '-' requer Int ou Float");
        }
        value_pool.push_back(last_value);
        break;
    }
    }
}

void Interpreter::visit(BinaryOpNode *node)
{
    // Avalia os operandos esquerdo e direito
    node->left->accept(this);
    Value *left_val = last_value;

    node->right->accept(this);
    Value *right_val = last_value;

    Value *result_val = nullptr;

    // --- BLOCO 1: Tratamento de Igualdade (==) e Desigualdade (!=) ---
    // Esta lógica é especial porque precisa funcionar para ponteiros (null) e valores.
    if (node->op == '=' || node->op == 'n')
    {
        bool are_equal;

        // Primeiro, o caso mais geral: um dos lados é null. Comparamos os ponteiros.
        if (left_val == nullptr || right_val == nullptr)
        {
            are_equal = (left_val == right_val);
        }
        // Se ambos não são nulos, tentamos uma comparação de valor mais específica.
        else
        {
            auto li = dynamic_cast<IntValue *>(left_val);
            auto ri = dynamic_cast<IntValue *>(right_val);
            if (li && ri)
            {
                are_equal = (li->value == ri->value);
            }
            else
            {
                auto lf = dynamic_cast<FloatValue *>(left_val);
                auto rf = dynamic_cast<FloatValue *>(right_val);
                if (lf && rf)
                {
                    are_equal = (lf->value == rf->value);
                }
                else
                {
                    auto lb = dynamic_cast<BoolValue *>(left_val);
                    auto rb = dynamic_cast<BoolValue *>(right_val);
                    if (lb && rb)
                    {
                        are_equal = (lb->value == rb->value);
                    }
                    else
                    {
                        auto lc = dynamic_cast<CharValue *>(left_val);
                        auto rc = dynamic_cast<CharValue *>(right_val);
                        if (lc && rc)
                        {
                            are_equal = (lc->value == rc->value);
                        }
                        else
                        {
                            // Para tipos complexos (Record, Array), comparamos o endereço de memória.
                            are_equal = (left_val == right_val);
                        }
                    }
                }
            }
        }

        bool final_result = (node->op == '=') ? are_equal : !are_equal;
        result_val = new BoolValue(final_result);
    }
    // --- BLOCO 2: Tratamento de Operadores Numéricos e Relacionais ---
    else
    {
        // Converte os operandos para os tipos numéricos que nos interessam.
        auto li = dynamic_cast<IntValue *>(left_val);
        auto ri = dynamic_cast<IntValue *>(right_val);
        auto lf = dynamic_cast<FloatValue *>(left_val);
        auto rf = dynamic_cast<FloatValue *>(right_val);

        auto toFloat = [](IntValue *v)
        { return static_cast<float>(v->value); };

        switch (node->op)
        {
        case '+':
            if (li && ri)
                result_val = new IntValue(li->value + ri->value);
            else if (lf && rf)
                result_val = new FloatValue(lf->value + rf->value);
            else if (li && rf)
                result_val = new FloatValue(toFloat(li) + rf->value);
            else if (lf && ri)
                result_val = new FloatValue(lf->value + toFloat(ri));
            break;
        case '-':
            if (li && ri)
                result_val = new IntValue(li->value - ri->value);
            else if (lf && rf)
                result_val = new FloatValue(lf->value - rf->value);
            else if (li && rf)
                result_val = new FloatValue(toFloat(li) - rf->value);
            else if (lf && ri)
                result_val = new FloatValue(lf->value - toFloat(ri));
            break;
        case '*':
            if (li && ri)
                result_val = new IntValue(li->value * ri->value);
            else if (lf && rf)
                result_val = new FloatValue(lf->value * rf->value);
            else if (li && rf)
                result_val = new FloatValue(toFloat(li) * rf->value);
            else if (lf && ri)
                result_val = new FloatValue(lf->value * toFloat(ri));
            break;
        case '/':
            if (li && ri)
                result_val = new IntValue(li->value / ri->value);
            else if (lf && rf)
                result_val = new FloatValue(lf->value / rf->value);
            else if (li && rf)
                result_val = new FloatValue(toFloat(li) / rf->value);
            else if (lf && ri)
                result_val = new FloatValue(lf->value / toFloat(ri));
            break;
        case '%':
            if (li && ri)
                result_val = new IntValue(li->value % ri->value);
            break;
        case '<':
            if (li && ri)
                result_val = new BoolValue(li->value < ri->value);
            else if (lf && rf)
                result_val = new BoolValue(lf->value < rf->value);
            else if (li && rf)
                result_val = new BoolValue(toFloat(li) < rf->value);
            else if (lf && ri)
                result_val = new BoolValue(lf->value < toFloat(ri));
            break;
        case '>':
            if (li && ri)
                result_val = new BoolValue(li->value > ri->value);
            else if (lf && rf)
                result_val = new BoolValue(lf->value > rf->value);
            else if (li && rf)
                result_val = new BoolValue(toFloat(li) > rf->value);
            else if (lf && ri)
                result_val = new BoolValue(lf->value > toFloat(ri));
            break;
        case '&': // Operador lógico '&&'
            auto lb = dynamic_cast<BoolValue *>(left_val);
            auto rb = dynamic_cast<BoolValue *>(right_val);
            if (lb && rb)
                result_val = new BoolValue(lb->value && rb->value);
            break;
        }
    }

    // --- Finalização ---
    if (result_val)
    {
        last_value = result_val;
        value_pool.push_back(result_val);
    }
    else
    {
        // Se nenhuma regra funcionou, os tipos são incompatíveis para a operação.
        throw std::runtime_error("Erro de Execução: Operação binária entre tipos incompatíveis.");
    }
}
void Interpreter::visit(TypeNode *node) {}

void Interpreter::visit(NullLiteralNode * /*node*/)
{
    last_value = nullptr; // ou um NullValue*, se preferir
}

void Interpreter::visit(ArrayAccessNode *node)
{
    node->array_expr->accept(this);
    auto *arr_val = dynamic_cast<ArrayValue *>(last_value);
    if (!arr_val)
    {
        // Lança um erro claro em vez de retornar em silêncio
        throw std::runtime_error("Erro de execução: tentativa de indexar um tipo que não é um array.");
    }

    node->index_expr->accept(this);
    auto *idx_val = dynamic_cast<IntValue *>(last_value);
    if (!idx_val)
    {
        throw std::runtime_error("Erro de execução: o índice de um array deve ser do tipo Int.");
    }

    if (idx_val->value < 0 || idx_val->value >= arr_val->elements.size())
    {
        // Erro de "out-of-bounds"
        throw std::runtime_error("Erro de execução: Índice de array (" + std::to_string(idx_val->value) + ") fora dos limites [0, " + std::to_string(arr_val->elements.size() - 1) + "].");
    }

    last_value = arr_val->elements[idx_val->value];
}