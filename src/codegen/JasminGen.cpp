
#include "JasminGen.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cstdlib>
static bool _TRACE_ON(){ static bool on = std::getenv("LANG_TRACE")!=nullptr; return on; }
#define TRACE_LN(x) do{ if(_TRACE_ON()) std::cerr << "[gen] " << x << std::endl; }while(0)


void JasminGen::generate(ProgramNode* node, std::ostream& os){
    out_.str(""); out_.clear();
    fun_sigs_.clear();
    record_sigs_.clear();
    record_field_recordname_.clear();

    // Pre-pass: gather function signatures and record schemas
    for (auto def : node->definitions){
        if (auto f = dynamic_cast<FunDefNode*>(def)){
            std::string ret = "V";
            if (f->return_types.size() == 1){
                ret = type_desc(f->return_types[0]);
            } else if (f->return_types.size() > 1){
                bool all_int = true;
                for (auto t : f->return_types){
                    if (!(t && t->is_primitive && t->p_type == Primitive::INT)) { all_int = false; break; }
                }
                if (all_int) ret = "[I";
                else ret = "[Ljava/lang/Object;";
            }
            std::vector<std::pair<std::string,std::string>> params;
            for (auto const& p : f->params){
                params.push_back({p.name, type_desc(p.type)});
            }
            std::vector<std::string> pdesc;
            for (auto &pr : params) pdesc.push_back(pr.second);
            std::vector<std::string> precs;
            for (auto const& p : f->params){
                if (p.type && !p.type->is_primitive && !p.type->is_array) precs.push_back(p.type->user_type_name);
                else precs.push_back("");
            }
            std::string rrec = "";
            if (f->return_types.size()==1 && f->return_types[0] && !f->return_types[0]->is_primitive && !f->return_types[0]->is_array){
                rrec = f->return_types[0]->user_type_name;
            }
            fun_sigs_[f->name] = {pdesc, ret, precs, rrec};
        } else if (auto d = dynamic_cast<DataDefNode*>(def)){
            std::unordered_map<std::string,std::string> fmap;
            for (auto fld : d->fields){
                if (!fld) continue;
                std::string fd = type_desc(fld->type);
                fmap[fld->name] = fd;
                if (fld->type && !fld->type->is_primitive && !fld->type->is_array){
                    record_field_recordname_[d->name][fld->name] = fld->type->user_type_name;
                }
            }
            record_sigs_[d->name] = fmap;
        }
    }

    open_class();

    // Default ctor
    emit_line(".method public <init>()V");
    emit_line("  aload_0");
    emit_line("  invokespecial java/lang/Object/<init>()V");
    emit_line("  return");
    emit_line(".end method");
    emit_line("");

    // Generate methods for function definitions
    for (auto def : node->definitions){
        if (auto f = dynamic_cast<FunDefNode*>(def)) f->accept(this);
    }

    // Java entry main(String[]) that calls lang 'main' if exists
    emit_line(".method public static main([Ljava/lang/String;)V");
    emit_line("  .limit stack 4");
    emit_line("  .limit locals 2");
    if (fun_sigs_.count("main")){
        auto sig = fun_sigs_["main"];
        if (sig.ret_desc == "V"){
            emit_line("  invokestatic " + cls_ + "/main()V");
        } else {
            emit_line("  invokestatic " + cls_ + "/main()" + sig.ret_desc);
            emit_line("  pop");
        }
    }
    emit_line("  return");
    emit_line(".end method");
    emit_line("");

    close_class();
    os << out_.str();
}

// ========== EMIT/TYPE/LOCAL HELPERS ==========

void JasminGen::emit_line(const std::string& s){ out_ << s << "\n"; }
void JasminGen::emit(const std::string& s){ out_ << s; }

std::string JasminGen::type_desc(TypeNode* t){
    if (!t) return "V";
    if (t->is_array){
        int dims = 0;
        TypeNode* e = t;
        while (e && e->is_array){ ++dims; e = e->element_type; }
        std::string prefix(dims, '[');
        if (e && e->is_primitive && e->p_type == Primitive::INT) return prefix + "I";
        return prefix + "Ljava/lang/Object;";
    }
    if (t->is_primitive){
        switch(t->p_type){
            case Primitive::INT:   return "I";
            case Primitive::FLOAT: return "F";
            case Primitive::CHAR:  return "C";
            case Primitive::BOOL:  return "Z";
            case Primitive::VOID:  return "V";
        }
    }
    return "Ljava/util/HashMap;";
}

std::string JasminGen::type_desc_prim(Primitive k){
    switch(k){
        case Primitive::INT: return "I";
        case Primitive::FLOAT: return "F";
        case Primitive::BOOL: return "Z";
        case Primitive::CHAR: return "C";
        case Primitive::VOID: return "V";
    }
    return "Ljava/util/HashMap;";
}

void JasminGen::open_class(){
    emit_line(".class public " + cls_);
    emit_line(".super java/lang/Object");
    emit_line("");
}
void JasminGen::close_class(){}

void JasminGen::open_method(const std::string& name, const std::vector<std::pair<std::string,std::string>>& params, const std::string& ret){
    cur_method_name_ = name;
    cur_method_ret_ = ret;
    cur_params_ = params;
    locals_.clear();
    next_local_ = 0;
    loop_temp_counter_ = 0;

    std::string desc="(";
    for (auto &p : params) desc += p.second;
    desc += ")";
    desc += ret;
    emit_line(".method public static " + name + desc);
    emit_line("  .limit stack " + std::to_string(stack_limit_));
    for (auto &p : params){
        ensure_local(p.first, p.second);
    }
    emit_line("  .limit locals " + std::to_string(locals_limit_));
}

void JasminGen::close_method(){
    if (cur_method_ret_ == "V"){
        emit_line("  return");
    } else if (cur_method_ret_ == "F"){
        emit_line("  fconst_0");
        emit_line("  freturn");
    } else if (!cur_method_ret_.empty() && (cur_method_ret_[0]=='L' || cur_method_ret_[0]=='[')){
        emit_line("  aconst_null");
        emit_line("  areturn");
    } else {
        push_int(0);
        emit_line("  ireturn");
    }
    emit_line(".end method");
    emit_line("");
}

void JasminGen::ensure_local(const std::string& name, const std::string& desc){
    if (locals_.count(name)==0){
        locals_[name] = { next_local_, desc };
        next_local_ += (desc=="J"||desc=="D") ? 2 : 1;
    }
}
int JasminGen::local_index(const std::string& name) const{
    auto it = locals_.find(name);
    if (it==locals_.end()) return -1;
    return it->second.index;
}
void JasminGen::push_int(int v){
    if (v >= -1 && v <= 5){
        static const char* names[] = {"iconst_m1","iconst_0","iconst_1","iconst_2","iconst_3","iconst_4","iconst_5"};
        emit_line(std::string("  ") + names[v+1]);
    } else if (v >= -128 && v <= 127){
        emit_line("  bipush " + std::to_string(v));
    } else if (v >= -32768 && v <= 32767){
        emit_line("  sipush " + std::to_string(v));
    } else {
        emit_line("  ldc " + std::to_string(v));
    }
}
void JasminGen::emit_load_local(const std::string& name){
    auto it = locals_.find(name);
    if (it==locals_.end()){
        emit_line("  iload 0");
        return;
    }
    int idx = it->second.index;
    const std::string& d = it->second.desc;
    if (!d.empty() && (d[0]=='L' || d[0]=='[')){
        emit_line("  aload " + std::to_string(idx));
    } else if (d=="F"){
        emit_line("  fload " + std::to_string(idx));
    } else {
        emit_line("  iload " + std::to_string(idx));
    }
}
void JasminGen::emit_store_local(const std::string& name){
    auto it = locals_.find(name);
    if (it==locals_.end()){
        ensure_local(name, "I");
        emit_line("  istore " + std::to_string(locals_[name].index));
        return;
    }
    int idx = it->second.index;
    const std::string& d = it->second.desc;
    if (!d.empty() && (d[0]=='L' || d[0]=='[')){
        emit_line("  astore " + std::to_string(idx));
    } else if (d=="F"){
        emit_line("  fstore " + std::to_string(idx));
    } else {
        emit_line("  istore " + std::to_string(idx));
    }
}

std::string JasminGen::expr_desc(Expression* e){
  TRACE_LN("enter JasminGen::expr_desc");

    if (!e) return "Ljava/lang/Object;";
    if (dynamic_cast<IntLiteral*>(e))   return "I";
    if (dynamic_cast<FloatLiteralNode*>(e)) return "F";
    if (dynamic_cast<BoolLiteralNode*>(e))  return "Z";
    if (dynamic_cast<CharLiteralNode*>(e))  return "C";
    if (auto va = dynamic_cast<VarAccessNode*>(e)){
        auto it = locals_.find(va->name);
        if (it!=locals_.end()) return it->second.desc;
        return "I";
    }
    if (auto fa = dynamic_cast<FieldAccessNode*>(e)){
        return field_desc_from_record(fa->record_expr, fa->field_name);
    }
    if (auto aa = dynamic_cast<ArrayAccessNode*>(e)){
        bool is_int_array = true;
        if (auto v2 = dynamic_cast<VarAccessNode*>(aa->array_expr)){
            auto it = locals_.find(v2->name);
            if (it != locals_.end()) is_int_array = (it->second.desc == "[I");
        } else if (auto fc = dynamic_cast<FunCallNode*>(aa->array_expr)){
            auto itf = fun_sigs_.find(fc->name);
            if (itf != fun_sigs_.end()){
                std::string rd = itf->second.ret_desc;
                if (!rd.empty() && rd[0]=='[') is_int_array = (rd == "[I");
            }
        }
        return is_int_array ? "I" : "Ljava/lang/Object;";
    }
    if (auto fc = dynamic_cast<FunCallNode*>(e)){
        auto itf = fun_sigs_.find(fc->name);
        if (itf != fun_sigs_.end()) return itf->second.ret_desc;
        return "I";
    }
    if (auto ne = dynamic_cast<NewExprNode*>(e)) {
        if (ne->base_type && !ne->base_type->is_primitive && !ne->base_type->is_array) return "Ljava/util/HashMap;";
        bool is_int = (ne->base_type && ne->base_type->is_primitive && ne->base_type->p_type == Primitive::INT);
        return is_int ? "[I" : "[Ljava/lang/Object;";
    }
    if (dynamic_cast<UnaryOpNode*>(e)) return "I";
    if (dynamic_cast<BinaryOpNode*>(e)) return "I";
    return "Ljava/util/HashMap;";
}

// Helpers for records
std::string JasminGen::record_name_of(Expression* e){
  TRACE_LN("enter JasminGen::record_name_of");

    if (!e) return "";
    if (auto va = dynamic_cast<VarAccessNode*>(e)){
        auto it = local_record_type_.find(va->name);
        if (it != local_record_type_.end()) return it->second;
        return "";
    }
    if (auto fc = dynamic_cast<FunCallNode*>(e)){
        auto itf = fun_sigs_.find(fc->name);
        if (itf != fun_sigs_.end()) return itf->second.ret_record_name;
        return "";
    }
    if (auto fa = dynamic_cast<FieldAccessNode*>(e)){
        std::string base = record_name_of(fa->record_expr);
        if (base.empty()) return "";
        auto it = record_field_recordname_.find(base);
        if (it == record_field_recordname_.end()) return "";
        auto it2 = it->second.find(fa->field_name);
        if (it2 == it->second.end()) return "";
        return it2->second;
    }
    return "";

    if (auto aa = dynamic_cast<ArrayAccessNode*>(e)){
        // Follow to base; if base is a record's array field, return that element record name
        if (auto f = dynamic_cast<FieldAccessNode*>(aa->array_expr)){
            std::string base = record_name_of(f->record_expr);
            if (!base.empty()){
                auto it = record_field_recordname_.find(base);
                if (it != record_field_recordname_.end()){
                    auto it2 = it->second.find(f->field_name);
                    if (it2 != it->second.end()){
                        return it2->second;
                    }
                }
            }
        } else if (auto a2 = dynamic_cast<ArrayAccessNode*>(aa->array_expr)){
            return record_name_of(a2->array_expr);
        }
        return "";
    }
}
std::string JasminGen::field_desc_from_record(Expression* rec_expr, const std::string& field){
  TRACE_LN("enter JasminGen::field_desc_from_record");

    std::string rname = record_name_of(rec_expr);
    if (rname.empty()) return "Ljava/lang/Object;";
    auto it = record_sigs_.find(rname);
    if (it == record_sigs_.end()) return "Ljava/lang/Object;";
    auto it2 = it->second.find(field);
    if (it2 == it->second.end()) return "Ljava/lang/Object;";
    return it2->second;
}

// ========== VISITORS ==========

void JasminGen::visit(ProgramNode* node){
    for (auto def : node->definitions){
        if (auto f = dynamic_cast<FunDefNode*>(def)){
            f->accept(this);
        }
    }
}

void JasminGen::visit(FunDefNode* node){
    std::vector<std::pair<std::string,std::string>> params;
    for (auto const& p : node->params){
        params.push_back({p.name, type_desc(p.type)});
    }
    std::string ret = "V";
    if (node->return_types.size() == 1) ret = type_desc(node->return_types[0]);
    else if (node->return_types.size() > 1){
        bool all_int = true;
        for (auto t : node->return_types){
            if (!(t && t->is_primitive && t->p_type == Primitive::INT)) { all_int = false; break; }
        }
        if (all_int) ret = "[I";
        else ret = "[Ljava/lang/Object;";
    }
    open_method(node->name, params, ret);

    // map record-typed params for field access
    for (auto const& p : node->params){
        if (p.type && !p.type->is_primitive && !p.type->is_array){
            local_record_type_[p.name] = p.type->user_type_name;
        }
    }

    if (node->body) node->body->accept(this);
    close_method();
}

void JasminGen::visit(DataDefNode* /*node*/){}

void JasminGen::visit(BlockCmdNode* node){
    for (auto cmd : node->commands){
        if (cmd) cmd->accept(this);
    }
}

void JasminGen::visit(FunCallNode* node){
    std::string name = node->name;
    auto it = fun_sigs_.find(name);
    std::string desc="(";
    if (it != fun_sigs_.end()){
        for (auto &pd : it->second.param_descs) desc += pd;
    } else {
        for (size_t i=0;i<node->args.size();++i) desc += "I";
    }
    desc += ")";
    std::string ret = (it!=fun_sigs_.end())? it->second.ret_desc : "I";
    desc += ret;
    for (auto e : node->args){
        e->accept(this);
    }
    emit_line("  invokestatic " + cls_ + "/" + name + desc);
}


void JasminGen::visit(FunCallCmdNode* node){
  TRACE_LN("enter JasminGen::visit");

    std::string name = node->name;
    auto it = fun_sigs_.find(name);
    std::string desc="(";
    if (it != fun_sigs_.end()){
        for (auto &pd : it->second.param_descs) desc += pd;
    } else {
        for (size_t i=0;i<node->args.size();++i) desc += "I";
    }
    desc += ")";
    std::string ret = (it!=fun_sigs_.end())? it->second.ret_desc : "I";
    desc += ret;

    // push args
    for (auto e : node->args) e->accept(this);
    emit_line("  invokestatic " + cls_ + "/" + name + desc);

    // No capture: discard non-void result
    if (node->lvalues.empty()){
        if (ret != "V") emit_line("  pop");
        return;
    }

    // Helpers
    auto store_field = [&](FieldAccessNode* fa, const std::string& pushed){
        fa->record_expr->accept(this);
        emit_line("  ldc \"" + fa->field_name + "\"");
        if (pushed == "I"){
            emit_line("  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;");
        } else if (pushed == "F"){
            emit_line("  invokestatic java/lang/Float/valueOf(F)Ljava/lang/Float;");
        } else if (pushed == "Z"){
            emit_line("  invokestatic java/lang/Boolean/valueOf(Z)Ljava/lang/Boolean;");
        } else if (pushed == "C"){
            emit_line("  invokestatic java/lang/Character/valueOf(C)Ljava/lang/Character;");
        }
        emit_line("  invokevirtual java/util/HashMap/put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        emit_line("  pop");
    };
    auto store_array = [&](ArrayAccessNode* aa, const std::string& pushed){
        std::string el = expr_desc(aa);
        // Stash value, then compute address+index, then reload
        std::string tv = "__val_tmp";
        ensure_local(tv, pushed);
        emit_store_local(tv);
        aa->array_expr->accept(this);
        aa->index_expr->accept(this);
        emit_load_local(tv);
        if (el == "I"){
            if (pushed == "I" || pushed == "Z" || pushed == "C"){
                emit_line("  iastore");
            } else if (pushed == "F"){
                // narrow not supported: drop store (pop 3)
                emit_line("  pop"); emit_line("  pop"); emit_line("  pop");
            } else {
                emit_line("  checkcast java/lang/Integer");
                emit_line("  invokevirtual java/lang/Integer/intValue()I");
                emit_line("  iastore");
            }
        } else {
            if (pushed == "I"){
                emit_line("  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;");
            } else if (pushed == "F"){
                emit_line("  invokestatic java/lang/Float/valueOf(F)Ljava/lang/Float;");
            } else if (pushed == "Z"){
                emit_line("  invokestatic java/lang/Boolean/valueOf(Z)Ljava/lang/Boolean;");
            } else if (pushed == "C"){
                emit_line("  invokestatic java/lang/Character/valueOf(C)Ljava/lang/Character;");
            }
            emit_line("  aastore");
        }
    };

    // Single capture
    if (node->lvalues.size() == 1){
        Expression* lv = node->lvalues[0];
        if (auto va = dynamic_cast<VarAccessNode*>(lv)){
            if (it!=fun_sigs_.end() && !it->second.ret_record_name.empty()){
                local_record_type_[va->name] = it->second.ret_record_name;
            }
            emit_store_local(va->name);
            return;
        }
        if (auto aa = dynamic_cast<ArrayAccessNode*>(lv)){
            store_array(aa, ret);
            return;
        }
        if (auto fa = dynamic_cast<FieldAccessNode*>(lv)){
            store_field(fa, ret);
            return;
        }
        if (ret != "V") emit_line("  pop");
        return;
    }

    // Multiple capture: expect [I or [Ljava/lang/Object;
    std::string arr = "__ret_arr";
    ensure_local(arr, ret);
    emit_store_local(arr);
    for (size_t i=0;i<node->lvalues.size();++i){
        Expression* lv = node->lvalues[i];
        emit_load_local(arr);
        push_int((int)i);
        if (ret == "[I"){
            emit_line("  iaload");
            if (auto va = dynamic_cast<VarAccessNode*>(lv)){
                emit_store_local(va->name);
                continue;
            }
            if (auto aa = dynamic_cast<ArrayAccessNode*>(lv)){
                std::string tv="__elt_i";
                ensure_local(tv,"I");
                emit_store_local(tv);
                aa->array_expr->accept(this);
                aa->index_expr->accept(this);
                emit_load_local(tv);
                std::string el = expr_desc(aa);
                if (el == "I"){
                    emit_line("  iastore");
                } else {
                    emit_line("  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;");
                    emit_line("  aastore");
                }
                continue;
            }
            if (auto fa = dynamic_cast<FieldAccessNode*>(lv)){
                std::string tv="__elt_i2";
                ensure_local(tv,"I");
                emit_store_local(tv);
                fa->record_expr->accept(this);
                emit_line("  ldc \"" + fa->field_name + "\"");
                emit_load_local(tv);
                emit_line("  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;");
                emit_line("  invokevirtual java/util/HashMap/put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
                emit_line("  pop");
                continue;
            }
            emit_line("  pop");
        } else {
            emit_line("  aaload");
            if (auto va = dynamic_cast<VarAccessNode*>(lv)){
                std::string d = expr_desc(lv);
                if (d == "I"){
                    emit_line("  checkcast java/lang/Integer");
                    emit_line("  invokevirtual java/lang/Integer/intValue()I");
                    emit_store_local(va->name);
                } else if (d == "F"){
                    emit_line("  checkcast java/lang/Float");
                    emit_line("  invokevirtual java/lang/Float/floatValue()F");
                    emit_store_local(va->name);
                } else if (d == "Z"){
                    emit_line("  checkcast java/lang/Boolean");
                    emit_line("  invokevirtual java/lang/Boolean/booleanValue()Z");
                    emit_store_local(va->name);
                } else if (d == "C"){
                    emit_line("  checkcast java/lang/Character");
                    emit_line("  invokevirtual java/lang/Character/charValue()C");
                    emit_store_local(va->name);
                } else {
                    emit_store_local(va->name);
                }
                continue;
            }
            std::string to="__elt_o";
            ensure_local(to,"Ljava/lang/Object;");
            emit_store_local(to);
            if (auto aa = dynamic_cast<ArrayAccessNode*>(lv)){
                std::string el = expr_desc(aa);
                aa->array_expr->accept(this);
                aa->index_expr->accept(this);
                emit_load_local(to);
                if (el == "I"){
                    emit_line("  checkcast java/lang/Integer");
                    emit_line("  invokevirtual java/lang/Integer/intValue()I");
                    emit_line("  iastore");
                } else {
                    emit_line("  aastore");
                }
                continue;
            }
            if (auto fa = dynamic_cast<FieldAccessNode*>(lv)){
                fa->record_expr->accept(this);
                emit_line("  ldc \"" + fa->field_name + "\"");
                emit_load_local(to);
                emit_line("  invokevirtual java/util/HashMap/put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
                emit_line("  pop");
                continue;
            }
            emit_line("  pop");
        }
    }
}



void JasminGen::visit(NewExprNode* node){
  TRACE_LN("enter JasminGen::visit");

    // Handle array allocations (may have unspecified dimensions like [] [N]).
    if (node && !node->dims.empty()){
        // Find a specified dimension to use for the outer allocation.
        Expression* sz = nullptr;
        for (int i = (int)node->dims.size() - 1; i >= 0; --i){
            if (node->dims[i]) { sz = node->dims[i]; break; }
        }
        if (!sz){
            // No dimension explicitly provided: allocate zero-length as a safe default.
            // (Caller code is expected to assign inner arrays later.)
            push_int(0);
        } else {
            sz->accept(this);
        }

        // Decide array kind from the *atomic* base type (new_expression uses atomic_type).
        bool is_int = (node->base_type && node->base_type->is_primitive && node->base_type->p_type == Primitive::INT);
        if (is_int){
            emit_line("  newarray int");
        } else {
            emit_line("  anewarray java/lang/Object");
        }
        return;
    }

    // Handle 'new <RecordType>' allocation (records are represented as HashMap).
    if (node && node->base_type && !node->base_type->is_primitive && !node->base_type->is_array){
        emit_line("  new java/util/HashMap");
        emit_line("  dup");
        emit_line("  invokespecial java/util/HashMap/<init>()V");
        return;
    }
    emit_line("  aconst_null");
}


void JasminGen::visit(FieldAccessNode* node){
  TRACE_LN("enter JasminGen::visit");

    std::string fdesc = field_desc_from_record(node->record_expr, node->field_name);
    node->record_expr->accept(this);
    
    emit_line("  checkcast java/util/HashMap");
emit_line("  ldc \"" + node->field_name + "\"");
    emit_line("  invokevirtual java/util/HashMap/get(Ljava/lang/Object;)Ljava/lang/Object;");
    if (fdesc == "I"){
        emit_line("  checkcast java/lang/Integer");
        emit_line("  invokevirtual java/lang/Integer/intValue()I");
    } else if (fdesc == "F"){
        emit_line("  checkcast java/lang/Float");
        emit_line("  invokevirtual java/lang/Float/floatValue()F");
    } else if (fdesc == "Z"){
        emit_line("  checkcast java/lang/Boolean");
        emit_line("  invokevirtual java/lang/Boolean/booleanValue()Z");
    } else if (fdesc == "C"){
        emit_line("  checkcast java/lang/Character");
        emit_line("  invokevirtual java/lang/Character/charValue()C");
    
    } else if (!fdesc.empty()){
        if (fdesc[0] == 'L'){
            // Convert descriptor 'Ljava/lang/Foo;' -> 'java/lang/Foo' for Jasmin checkcast
            std::string cname = fdesc.substr(1, fdesc.size()-2);
            emit_line("  checkcast " + cname);
        } else if (fdesc[0] == '['){
            // Arrays may be used with descriptor form (e.g., [I, [Ljava/lang/Object;)
            emit_line("  checkcast " + fdesc);
        }
    }
}


void JasminGen::visit(PrintCmd* node){
    // Evaluate expression first
    std::string d;
    if (auto fa = dynamic_cast<FieldAccessNode*>(node->expr)){
        d = field_desc_from_record(fa->record_expr, fa->field_name);
    } else {
        d = expr_desc(node->expr);
    }
    node->expr->accept(this);
    // Ensure a String on stack
    if (d == "I"){
        emit_line("  invokestatic java/lang/String/valueOf(I)Ljava/lang/String;");
    } else if (d == "F"){
        emit_line("  invokestatic java/lang/String/valueOf(F)Ljava/lang/String;");
    } else if (d == "Z"){
        emit_line("  invokestatic java/lang/String/valueOf(Z)Ljava/lang/String;");
    } else if (d == "C"){
        emit_line("  invokestatic java/lang/String/valueOf(C)Ljava/lang/String;");
    } else {
        emit_line("  invokestatic java/lang/String/valueOf(Ljava/lang/Object;)Ljava/lang/String;");
    }
    // Now load System.out and swap so that (objectref, arg) are in the right order
    emit_line("  getstatic java/lang/System/out Ljava/io/PrintStream;");
    emit_line("  swap");
    emit_line("  invokevirtual java/io/PrintStream/print(Ljava/lang/String;)V");
}


void JasminGen::visit(ReadCmdNode* node){
    auto gen_read = [&](const std::string& kind){
        emit_line("  new java/util/Scanner");
        emit_line("  dup");
        emit_line("  getstatic java/lang/System/in Ljava/io/InputStream;");
        emit_line("  invokespecial java/util/Scanner/<init>(Ljava/io/InputStream;)V");
        if (kind == "I"){
            emit_line("  invokevirtual java/util/Scanner/nextInt()I");
        } else if (kind == "F"){
            emit_line("  invokevirtual java/util/Scanner/nextFloat()F");
        } else if (kind == "Z"){
            emit_line("  invokevirtual java/util/Scanner/next()Ljava/lang/String;");
            emit_line("  invokestatic java/lang/Boolean/parseBoolean(Ljava/lang/String;)Z");
        } else if (kind == "C"){
            emit_line("  invokevirtual java/util/Scanner/next()Ljava/lang/String;");
            emit_line("  iconst_0");
            emit_line("  invokevirtual java/lang/String/charAt(I)C");
        } else {
            emit_line("  invokevirtual java/util/Scanner/nextInt()I");
        }
    };

    if (auto va = dynamic_cast<VarAccessNode*>(node->lvalue)){
        std::string n = va->name;
        std::string d = "I";
        auto it = locals_.find(n);
        if (it != locals_.end()) d = it->second.desc;
        if (d.empty() || d=="V") d = "I";
        gen_read(d=="[I" ? "I" : (d=="[Ljava/lang/Object;" ? "I" : d));
        if (locals_.count(n)==0) ensure_local(n, d);
        emit_store_local(n);
        return;
    }
    if (auto aa = dynamic_cast<ArrayAccessNode*>(node->lvalue)){
        aa->array_expr->accept(this);
        aa->index_expr->accept(this);
        gen_read("I");
        emit_line("  iastore");
        return;
    }
    if (auto fa = dynamic_cast<FieldAccessNode*>(node->lvalue)){
        std::string fdesc = field_desc_from_record(fa->record_expr, fa->field_name);
        fa->record_expr->accept(this);
        emit_line("  ldc \"" + fa->field_name + "\"");
        std::string k = (fdesc=="I"||fdesc=="F"||fdesc=="Z"||fdesc=="C") ? fdesc : "I";
        gen_read(k);
        if (fdesc == "I"){
            emit_line("  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;");
        } else if (fdesc == "F"){
            emit_line("  invokestatic java/lang/Float/valueOf(F)Ljava/lang/Float;");
        } else if (fdesc == "Z"){
            emit_line("  invokestatic java/lang/Boolean/valueOf(Z)Ljava/lang/Boolean;");
        } else if (fdesc == "C"){
            emit_line("  invokestatic java/lang/Character/valueOf(C)Ljava/lang/Character;");
        }
        emit_line("  invokevirtual java/util/HashMap/put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        emit_line("  pop");
        return;
    }
}

void JasminGen::visit(ReturnCmdNode* node){
    if (node->expressions.empty()){
        emit_line("  return");
        return;
    }
    if (node->expressions.size() == 1){
        node->expressions[0]->accept(this);
        if (!cur_method_ret_.empty() && (cur_method_ret_[0]=='L' || cur_method_ret_[0]=='[')){
            emit_line("  areturn");
        } else if (cur_method_ret_ == "F"){
            emit_line("  freturn");
        } else {
            emit_line("  ireturn");
        }
        return;
    }
    int n = (int)node->expressions.size();
    if (cur_method_ret_ == "[I"){
        push_int(n);
        emit_line("  newarray int");
        for (int i = 0; i < n; ++i){
            emit_line("  dup");
            push_int(i);
            node->expressions[i]->accept(this);
            emit_line("  iastore");
        }
        emit_line("  areturn");
    } else {
        push_int(n);
        emit_line("  anewarray java/lang/Object");
        for (int i = 0; i < n; ++i){
            emit_line("  dup");
            push_int(i);
            auto *expr = node->expressions[i];
            std::string d = expr_desc(expr);
            expr->accept(this);
            if (d == "I"){
                emit_line("  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;");
            } else if (d == "F"){
                emit_line("  invokestatic java/lang/Float/valueOf(F)Ljava/lang/Float;");
            } else if (d == "Z"){
                emit_line("  invokestatic java/lang/Boolean/valueOf(Z)Ljava/lang/Boolean;");
            } else if (d == "C"){
                emit_line("  invokestatic java/lang/Character/valueOf(C)Ljava/lang/Character;");
            }
            emit_line("  aastore");
        }
        emit_line("  areturn");
    }
}

void JasminGen::visit(VarDeclNode* node){
    std::string n = node->name;
    std::string d = type_desc(node->type);
    if (!node->type->is_primitive && !node->type->is_array){
        local_record_type_[n] = node->type->user_type_name;
    }
    ensure_local(n, d);
    if (!d.empty() && (d[0]=='L' || d[0]=='[')){
        emit_line("  aconst_null");
        emit_store_local(n);
    } else if (d=="F"){
        emit_line("  fconst_0");
        emit_store_local(n);
    } else {
        push_int(0);
        emit_store_local(n);
    }
}

void JasminGen::visit(AssignCmdNode* node){
  TRACE_LN("enter JasminGen::visit");

    if (auto va = dynamic_cast<VarAccessNode*>(node->lvalue)){
        std::string n = va->name;
        std::string desc = "";
        if (auto fc = dynamic_cast<FunCallNode*>(node->expr)){
            auto itf = fun_sigs_.find(fc->name);
            if (itf != fun_sigs_.end()){
                desc = itf->second.ret_desc;
                if (!itf->second.ret_record_name.empty()){
                    local_record_type_[n] = itf->second.ret_record_name;
                }
            }
        
        } else if (auto aa = dynamic_cast<ArrayAccessNode*>(node->expr)){
            if (auto fc2 = dynamic_cast<FunCallNode*>(aa->array_expr)){
                auto itf2 = fun_sigs_.find(fc2->name);
                if (itf2 != fun_sigs_.end() && !itf2->second.ret_record_name.empty()){
                    local_record_type_[n] = itf2->second.ret_record_name;
                    desc = "Ljava/util/HashMap;";
                }
            }
    } else if (auto ne = dynamic_cast<NewExprNode*>(node->expr)){
            if (ne->base_type && !ne->base_type->is_primitive && !ne->base_type->is_array){
                desc = "Ljava/util/HashMap;";
                local_record_type_[n] = ne->base_type->user_type_name;
            } else {
                desc = "[I";
            }
        }
        if (locals_.count(n)==0){
            ensure_local(n, desc.empty() ? "I" : desc);
        } else if (!desc.empty()){
            locals_[n].desc = desc;
        }
        node->expr->accept(this);
        emit_store_local(n);
        return;
    }
    if (auto aa = dynamic_cast<ArrayAccessNode*>(node->lvalue)){
        aa->array_expr->accept(this);
        aa->index_expr->accept(this);
        node->expr->accept(this);
        emit_line("  iastore");
        return;
    }
    if (auto fa = dynamic_cast<FieldAccessNode*>(node->lvalue)){
        std::string fdesc = field_desc_from_record(fa->record_expr, fa->field_name);
        fa->record_expr->accept(this);
        emit_line("  ldc \"" + fa->field_name + "\"");
        node->expr->accept(this);
        if (fdesc == "I"){
            emit_line("  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;");
        } else if (fdesc == "F"){
            emit_line("  invokestatic java/lang/Float/valueOf(F)Ljava/lang/Float;");
        } else if (fdesc == "Z"){
            emit_line("  invokestatic java/lang/Boolean/valueOf(Z)Ljava/lang/Boolean;");
        } else if (fdesc == "C"){
            emit_line("  invokestatic java/lang/Character/valueOf(C)Ljava/lang/Character;");
        }
        emit_line("  invokevirtual java/util/HashMap/put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        emit_line("  pop");
        return;
    }
    node->expr->accept(this);
    emit_line("  pop");
}

void JasminGen::visit(IfCmdNode* node){
    node->condition->accept(this);
    int L_else = new_label();
    int L_end  = new_label();
    emit_line("  ifeq L" + std::to_string(L_else));
    if (node->then_branch) node->then_branch->accept(this);
    emit_line("  goto L" + std::to_string(L_end));
    emit_line("L" + std::to_string(L_else) + ":");
    if (node->else_branch) node->else_branch->accept(this);
    emit_line("L" + std::to_string(L_end) + ":");
}

void JasminGen::visit(IterateCmdNode* node){
    if (!node->loop_variable.empty()){
        int idxnum = ++loop_temp_counter_;
        std::string arrn = "_arr" + std::to_string(idxnum);
        std::string in = "_i" + std::to_string(idxnum);
        ensure_local(arrn, "[I");
        ensure_local(in, "I");
        node->condition->accept(this);
        emit_store_local(arrn);
        push_int(0);
        emit_store_local(in);
        int L_cond = new_label();
        int L_end  = new_label();
        emit_line("L" + std::to_string(L_cond) + ":");
        emit_load_local(in);
        emit_load_local(arrn);
        emit_line("  arraylength");
        emit_line("  if_icmpge L" + std::to_string(L_end));
        emit_load_local(arrn);
        emit_load_local(in);
        emit_line("  iaload");
        if (locals_.count(node->loop_variable)==0){
            ensure_local(node->loop_variable, "I");
        }
        emit_store_local(node->loop_variable);
        if (node->body) node->body->accept(this);
        emit_line("  iinc " + std::to_string(local_index(in)) + " 1");
        emit_line("  goto L" + std::to_string(L_cond));
        emit_line("L" + std::to_string(L_end) + ":");
        return;
    }
    int idx = ++loop_temp_counter_;
    std::string ivar = "_it" + std::to_string(idx);
    ensure_local(ivar, "I");
    push_int(0);
    emit_store_local(ivar);
    int L_cond = new_label();
    int L_end  = new_label();
    emit_line("L" + std::to_string(L_cond) + ":");
    emit_load_local(ivar);
    node->condition->accept(this);
    emit_line("  if_icmpge L" + std::to_string(L_end));
    if (node->body) node->body->accept(this);
    emit_line("  iinc " + std::to_string(local_index(ivar)) + " 1");
    emit_line("  goto L" + std::to_string(L_cond));
    emit_line("L" + std::to_string(L_end) + ":");
}

void JasminGen::visit(IntLiteral* node){ push_int(node->value); }
void JasminGen::visit(FloatLiteralNode* node){ emit_line("  ldc " + std::to_string(node->value)); }
void JasminGen::visit(CharLiteralNode* node){ push_int((int)node->value); }
void JasminGen::visit(BoolLiteralNode* node){ push_int(node->value?1:0); }

void JasminGen::visit(VarAccessNode* node){
    if (locals_.count(node->name)==0){
        ensure_local(node->name, "I");
    }
    emit_load_local(node->name);
}

void JasminGen::visit(UnaryOpNode* node){
    node->expr->accept(this);
    if (node->op == '-') {
        emit_line("  ineg");
    } else if (node->op == '!') {
        int Lt = new_label(), Lend = new_label();
        emit_line("  ifeq L" + std::to_string(Lt));
        push_int(0);
        emit_line("  goto L" + std::to_string(Lend));
        emit_line("L" + std::to_string(Lt) + ":");
        push_int(1);
        emit_line("L" + std::to_string(Lend) + ":");
    }
}



void JasminGen::visit(BinaryOpNode* node){
    // Logical AND '&&' encoded as '&' with short-circuit
    if (node->op == '&') {
        int L_false = new_label();
        int L_end   = new_label();
        // evaluate left
        node->left->accept(this);
        emit_line("  ifeq L" + std::to_string(L_false));
        // evaluate right only if left was true
        node->right->accept(this);
        emit_line("  ifeq L" + std::to_string(L_false));
        // both true -> push 1
        push_int(1);
        emit_line("  goto L" + std::to_string(L_end));
        // false path
        emit_line("L" + std::to_string(L_false) + ":");
        push_int(0);
        emit_line("L" + std::to_string(L_end) + ":");
        return;
    }

    // Normal binary ops evaluate both sides
    node->left->accept(this);
    node->right->accept(this);
    switch(node->op){
        case '+': emit_line("  iadd"); break;
        case '-': emit_line("  isub"); break;
        case '*': emit_line("  imul"); break;
        case '/': emit_line("  idiv"); break;
        case '%': emit_line("  irem"); break;
        case '=': {
    int Lt = new_label(), Lend = new_label();
    std::string dl = expr_desc(node->left);
    std::string dr = expr_desc(node->right);
    bool l_intlike = (dl=="I" || dl=="Z" || dl=="C");
    bool r_intlike = (dr=="I" || dr=="Z" || dr=="C");
    bool use_icmp = l_intlike && r_intlike;
    emit_line(std::string("  ") + (use_icmp ? "if_icmpeq" : "if_acmpeq") + " L" + std::to_string(Lt));
    push_int(0);
    emit_line("  goto L" + std::to_string(Lend));
    emit_line("L" + std::to_string(Lt) + ":");
    push_int(1);
    emit_line("L" + std::to_string(Lend) + ":");
    break;
}
        case 'n': {
    int Lt = new_label(), Lend = new_label();
    std::string dl = expr_desc(node->left);
    std::string dr = expr_desc(node->right);
    bool l_intlike = (dl=="I" || dl=="Z" || dl=="C");
    bool r_intlike = (dr=="I" || dr=="Z" || dr=="C");
    bool use_icmp = l_intlike && r_intlike;
    emit_line(std::string("  ") + (use_icmp ? "if_icmpne" : "if_acmpne") + " L" + std::to_string(Lt));
    push_int(0);
    emit_line("  goto L" + std::to_string(Lend));
    emit_line("L" + std::to_string(Lt) + ":");
    push_int(1);
    emit_line("L" + std::to_string(Lend) + ":");
    break;
}
        case '<': {
            int Lt = new_label(), Lend = new_label();
            emit_line("  if_icmplt L" + std::to_string(Lt));
            push_int(0);
            emit_line("  goto L" + std::to_string(Lend));
            emit_line("L" + std::to_string(Lt) + ":");
            push_int(1);
            emit_line("L" + std::to_string(Lend) + ":");
            break;
        }
        default:
            emit_line("  iadd"); // safe fallback
    }
}



void JasminGen::visit(TypeNode* /*node*/){ }
void JasminGen::visit(NullLiteralNode* /*node*/){ emit_line("  aconst_null"); }


void JasminGen::visit(ArrayAccessNode* node){
  TRACE_LN("enter JasminGen::visit");
  // Evaluate array reference and index
  node->array_expr->accept(this);
  node->index_expr->accept(this);
  // Decide opcode by descriptor of the array expression
  std::string arrd = expr_desc(node->array_expr);
  // int[] -> "[I", int[][] -> "[[I" etc.
  bool use_iaload = (!arrd.empty() && arrd[0]=='[' && arrd.back()=='I');
  emit_line(std::string("  ") + (use_iaload ? "iaload" : "aaload"));
}

