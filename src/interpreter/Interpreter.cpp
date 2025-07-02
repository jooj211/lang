#include "Interpreter.hpp"
#include "ReturnSignal.hpp"
#include "../ast/AST.hpp"
#include "../runtime/Values.hpp"
#include <iostream>
#include <cstring>
#include <cstdio>

Value *Interpreter::create_default_value(TypeNode *type)
{
    // Se o tipo base for primitivo
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
            return nullptr; // Void ou outros tipos não tem valor padrão
        }
        value_pool.push_back(val);
        return val;
    }
    // Se for um tipo de registro (ex: Ponto)
    else
    {
        std::string type_name = type->user_type_name;
        if (data_types.count(type_name))
        {
            DataDefNode *def = data_types.at(type_name);
            auto *record = new RecordValue(type_name);
            value_pool.push_back(record);

            // Inicializa todos os campos do registro com seus próprios valores padrão
            for (VarDeclNode *field : def->fields)
            {
                record->fields[field->name] = create_default_value(field->type);
            }
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
    // Caso base: Se todas as dimensões foram processadas, cria e retorna o elemento final.
    if (dim_index >= dims.size())
    {
        return create_default_value(base_elem_type);
    }

    // Passo recursivo: Processa a dimensão atual.
    dims[dim_index]->accept(this);
    auto *size_val = dynamic_cast<IntValue *>(last_value);
    if (!size_val || size_val->value < 0)
    {
        throw std::runtime_error("Erro de Execução: Dimensão de array inválida ou não-inteira.");
    }
    size_t size = size_val->value;

    // Cria o array para a dimensão atual.
    auto *arr_val = new ArrayValue();
    value_pool.push_back(arr_val);
    arr_val->elements.resize(size);

    // Preenche o array chamando a si mesma para a próxima dimensão.
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
void Interpreter::update_variable(const std::string &name, Value *new_value_obj)
{
    for (auto it = memory_stack.rbegin(); it != memory_stack.rend(); ++it)
    {
        if (it->count(name))
        {
            Value *ev = it->at(name);
            if (auto i_ev = dynamic_cast<IntValue *>(ev))
            {
                if (auto i_nv = dynamic_cast<IntValue *>(new_value_obj))
                    i_ev->value = i_nv->value;
            }
            else if (auto b_ev = dynamic_cast<BoolValue *>(ev))
            {
                if (auto b_nv = dynamic_cast<BoolValue *>(new_value_obj))
                    b_ev->value = b_nv->value;
            }
            else if (auto f_ev = dynamic_cast<FloatValue *>(ev))
            {
                if (auto f_nv = dynamic_cast<FloatValue *>(new_value_obj))
                    f_ev->value = f_nv->value;
            }
            else if (auto c_ev = dynamic_cast<CharValue *>(ev))
            {
                if (auto c_nv = dynamic_cast<CharValue *>(new_value_obj))
                    c_ev->value = c_nv->value;
            }
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

// Interpreter.cpp
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
void Interpreter::visit(NewExprNode *node)
{
    // Usa os novos membros do NewExprNode: base_type e dims

    // Caso 1: Alocação de objeto único (ex: new Ponto), o vetor de dimensões está vazio.
    if (node->dims.empty())
    {
        if (node->base_type->is_primitive)
        {
            throw std::runtime_error("Erro de Execução: 'new' em tipo primitivo requer um tamanho de array.");
        }
        last_value = create_default_value(node->base_type);
    }
    // Caso 2: Alocação de array/matriz (ex: new Int[5] ou new Ponto[10][2]).
    else
    {
        last_value = create_nested_array(node->base_type, node->dims, 0);
    }
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
    // 1. Avalia a expressão do lado direito (RHS) para obter o valor a ser atribuído.
    node->expr->accept(this);
    Value *rhs_value = last_value;

    // 2. Lida com l-value de ACESSO A VARIÁVEL (ex: x = 10)
    if (auto *va = dynamic_cast<VarAccessNode *>(node->lvalue))
    {
        if (get_variable(va->name))
        {
            update_variable(va->name, rhs_value);
        }
        else
        {
            set_variable(va->name, rhs_value, true);
        }
    }
    // 3. Lida com l-value de ACESSO A CAMPO (ex: p.x = 3.0)
    else if (auto *fa = dynamic_cast<FieldAccessNode *>(node->lvalue))
    {
        fa->record_expr->accept(this);
        auto *record_val = dynamic_cast<RecordValue *>(last_value);
        if (!record_val)
        {
            throw std::runtime_error("Erro de Execução: Tentativa de acessar campo em uma referência nula ou não-registro.");
        }

        // A lógica para atualizar o valor do campo...
        Value *field_to_update = record_val->fields.at(fa->field_name);
        // (A cópia de valor para campos que implementamos anteriormente vai aqui)
        if (auto i_dest = dynamic_cast<IntValue *>(field_to_update))
        {
            if (auto i_src = dynamic_cast<IntValue *>(rhs_value))
            {
                i_dest->value = i_src->value;
            }
        }
        else if (auto f_dest = dynamic_cast<FloatValue *>(field_to_update))
        {
            if (auto f_src = dynamic_cast<FloatValue *>(rhs_value))
            {
                f_dest->value = f_src->value;
            }
        }
        else if (auto c_dest = dynamic_cast<CharValue *>(field_to_update))
        {
            if (auto c_src = dynamic_cast<CharValue *>(rhs_value))
            {
                c_dest->value = c_src->value;
            }
        }
        else if (auto b_dest = dynamic_cast<BoolValue *>(field_to_update))
        {
            if (auto b_src = dynamic_cast<BoolValue *>(rhs_value))
            {
                b_dest->value = b_src->value;
            }
        }
        else
        {
            record_val->fields[fa->field_name] = rhs_value;
        }
    }
    // 4. Lida com l-value de ACESSO A ARRAY (ex: intArray[0] = 100) -- LÓGICA NOVA
    else if (auto *aa = dynamic_cast<ArrayAccessNode *>(node->lvalue))
    {
        // a. Avalia a expressão para obter o objeto array
        aa->array_expr->accept(this);
        auto *arr_val = dynamic_cast<ArrayValue *>(last_value);

        // b. Se a expressão não resultou em um array, lança o erro.
        if (!arr_val)
        {
            throw std::runtime_error("Erro de Execução: Tentativa de atribuição a um tipo que não é um array.");
        }

        // c. Avalia a expressão do índice
        aa->index_expr->accept(this);
        auto *idx_val = dynamic_cast<IntValue *>(last_value);
        if (!idx_val)
        {
            throw std::runtime_error("Erro de Execução: O índice de um array deve ser do tipo Int.");
        }
        int index = idx_val->value;
        if (index < 0 || index >= arr_val->elements.size())
        {
            throw std::runtime_error("Erro de Execução: Índice de array (" + std::to_string(index) + ") fora dos limites.");
        }

        // d. Atribui o valor na posição do array
        // (Isso assume que o array já foi inicializado com objetos de valor,
        // mesmo que com valores padrão, o que é feito em visit(VarDeclNode*))
        Value *dest_val = arr_val->elements.at(index);
        if (!dest_val)
        { // Se a posição era nula (ex: array de registros)
            arr_val->elements[index] = rhs_value;
        }
        else
        { // Se já existe um valor, atualiza-o
            if (auto i_dest = dynamic_cast<IntValue *>(dest_val))
            {
                if (auto i_src = dynamic_cast<IntValue *>(rhs_value))
                    i_dest->value = i_src->value;
            }
            else if (auto f_dest = dynamic_cast<FloatValue *>(dest_val))
            {
                if (auto f_src = dynamic_cast<FloatValue *>(rhs_value))
                    f_dest->value = f_src->value;
            } // ... outros tipos primitivos
            else
            { // Para tipos complexos, substitui o ponteiro
                arr_val->elements[index] = rhs_value;
            }
        }
    }
    else
    {
        // Mantém o erro para outros casos não implementados
        throw std::runtime_error("Erro de Execução: Tipo de atribuição à esquerda desconhecido.");
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
    node->left->accept(this);
    Value *lv = last_value;
    node->right->accept(this);
    Value *rv = last_value;

    auto li = dynamic_cast<IntValue *>(lv);
    auto ri = dynamic_cast<IntValue *>(rv);
    auto lf = dynamic_cast<FloatValue *>(lv);
    auto rf = dynamic_cast<FloatValue *>(rv);
    auto lb = dynamic_cast<BoolValue *>(lv);
    auto rb = dynamic_cast<BoolValue *>(rv);

    Value *res = nullptr;

    auto toFloat = [](IntValue *iv)
    { return static_cast<float>(iv->value); };

    switch (node->op)
    {
    /* --------- aritméticos ---------------------------------- */
    case '+':
        if (li && ri)
            res = new IntValue(li->value + ri->value);
        else if (lf && rf)
            res = new FloatValue(lf->value + rf->value);
        else if (li && rf)
            res = new FloatValue(toFloat(li) + rf->value);
        else if (lf && ri)
            res = new FloatValue(lf->value + toFloat(ri));
        break;

    case '-':
        if (li && ri)
            res = new IntValue(li->value - ri->value);
        else if (lf && rf)
            res = new FloatValue(lf->value - rf->value);
        else if (li && rf)
            res = new FloatValue(toFloat(li) - rf->value);
        else if (lf && ri)
            res = new FloatValue(lf->value - toFloat(ri));
        break;

    case '*':
        if (li && ri)
            res = new IntValue(li->value * ri->value);
        else if (lf && rf)
            res = new FloatValue(lf->value * rf->value);
        else if (li && rf)
            res = new FloatValue(toFloat(li) * rf->value);
        else if (lf && ri)
            res = new FloatValue(lf->value * toFloat(ri));
        break;

    case '/':
        if (li && ri && ri->value != 0)
            res = new IntValue(li->value / ri->value);
        else if (lf && rf && rf->value != 0.0f)
            res = new FloatValue(lf->value / rf->value);
        else if (li && rf && rf->value != 0.0f)
            res = new FloatValue(toFloat(li) / rf->value);
        else if (lf && ri && ri->value != 0)
            res = new FloatValue(lf->value / toFloat(ri));
        break;

    case '%':
        if (li && ri && ri->value != 0)
            res = new IntValue(li->value % ri->value);
        break;

    /* --------- relacional < --------------------------------- */
    case '<':
        if (li && ri)
            res = new BoolValue(li->value < ri->value);
        else if (lf && rf)
            res = new BoolValue(lf->value < rf->value);
        else if (li && rf)
            res = new BoolValue(toFloat(li) < rf->value);
        else if (lf && ri)
            res = new BoolValue(lf->value < toFloat(ri));
        break;

    case '>':
        if (li && ri)
            res = new BoolValue(li->value > ri->value);
        else if (lf && rf)
            res = new BoolValue(lf->value > rf->value);
        else if (li && rf)
            res = new BoolValue(toFloat(li) > rf->value);
        else if (lf && ri)
            res = new BoolValue(lf->value > toFloat(ri));
        break;

    /* --------- lógico && ------------------------------------ */
    case '&':
        if (lb && rb)
            res = new BoolValue(lb->value && rb->value);
        break;

    /* --------- igualdade / diferença ------------------------ */
    case '=':
        if (li && ri)
            res = new BoolValue(li->value == ri->value);
        else if (lf && rf)
            res = new BoolValue(lf->value == rf->value);
        else if (li && rf)
            res = new BoolValue(toFloat(li) == rf->value);
        else if (lf && ri)
            res = new BoolValue(lf->value == toFloat(ri));
        else if (lb && rb)
            res = new BoolValue(lb->value == rb->value);
        else
            res = new BoolValue(lv == rv); // ponteiros/null
        break;

    case 'n': /* != */
        if (li && ri)
            res = new BoolValue(li->value != ri->value);
        else if (lf && rf)
            res = new BoolValue(lf->value != rf->value);
        else if (li && rf)
            res = new BoolValue(toFloat(li) != rf->value);
        else if (lf && ri)
            res = new BoolValue(lf->value != toFloat(ri));
        else if (lb && rb)
            res = new BoolValue(lb->value != rb->value);
        else
            res = new BoolValue(lv != rv);
        break;
    }

    if (res)
    {
        last_value = res;
        value_pool.push_back(res);
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