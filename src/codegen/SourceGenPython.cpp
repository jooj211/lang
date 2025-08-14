
#include "SourceGenPython.hpp"
#include <unordered_set>
#include <sstream>
#include <iostream>
#include <cstdlib>
static bool _TRACE_ON(){ static bool on = std::getenv("LANG_TRACE")!=nullptr; return on; }
#define TRACE_LN(x) do{ if(_TRACE_ON()) std::cerr << "[src] " << x << std::endl; }while(0)
// --- Python identifier sanitizer ---
static bool is_py_kw(const std::string& s){
    static const std::unordered_set<std::string> kws = {
        "False","None","True","and","as","assert","async","await","break","class","continue","def",
        "del","elif","else","except","finally","for","from","global","if","import","in","is",
        "lambda","nonlocal","not","or","pass","raise","return","try","while","with","yield",
        "print","input","list","dict","set","tuple","str","int","float","bool"
    };
    return kws.count(s)!=0;
}
static std::string py_name(std::string s){
    if (s.empty()) return s;
    for (auto& ch : s){
        if (!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch=='_')) ch='_';
    }
    if (s[0]>='0'&&s[0]<='9') s = "_" + s;
    if (is_py_kw(s)) s += "_";
    return s;
}


#include <algorithm>

// Represent a char as a valid Python one-character literal (no NUL byte in source)
static std::string repr_char(char c){
    std::ostringstream oss;
    if (c=='\n') return "'\\n'";
    if (c=='\t') return "'\\t'";
    if (c=='\r') return "'\\r'";
    if (c=='\'') return "'\\''";
    if (c=='\\') return "'\\\\'";
    if (c=='\0') return "'\\x00'";
    if (c=='\"') return "'\\\"'";
    oss << "'" << c << "'";
    return oss.str();
}

void SourceGenPython::generate(ProgramNode* node, std::ostream& os){
    out_.str(""); out_.clear();
    records_.clear();
    locals_types_.clear();
    tmp_counter_ = 0; indent_ = 0; cur_fun_.clear();

    // Prepass: collect record field lists
    fun_returns_.clear();
    for (auto def : node->definitions){
        if (auto d = dynamic_cast<DataDefNode*>(def)){
            std::vector<std::pair<std::string, TypeNode*>> fields;
            for (auto f : d->fields){
                fields.push_back({f->name, f->type});
            }
            records_[d->name] = fields;
        }
    }

    // prologue
    emitln("import sys");
    emitln("");
    emitln("# input token buffer for typed reads");
    emitln("_tok = []");
    emitln("def _next():");
    indent_++; indent(); emitln("global _tok");
    indent(); emitln("while not _tok:");
    indent_++; indent(); emitln("line = sys.stdin.readline()");
    indent(); emitln("if not line: return ''");
    indent(); emitln("_tok = line.strip().split()");
    indent_-=1;
    indent(); emitln("return _tok.pop(0)");
    indent_-=1;
    emitln("");
    emitln("def _read_int():");
    indent_++; indent(); emitln("s = _next()");
    indent(); emitln("return int(s) if s else 0");
    indent_-=1;
emitln("");
emitln("def _read_float():");
    indent_++; indent(); emitln("s = _next()");
    indent(); emitln("return float(s) if s else 0.0");
    indent_-=1;
emitln("");
emitln("def _read_bool():");
    indent_++; indent(); emitln("s = _next()");
    indent(); emitln("return (s.lower()=='true') if s else False");
    indent_-=1;
emitln("");
emitln("def _read_char():");
    indent_++; indent(); emitln("s = _next()");
    indent(); emitln("return s[0] if s else '\\x00'");
    indent_-=1;
emitln("");

    emitln("def _print(v):");
    indent_++; indent(); emitln("import math");
    indent(); emitln("if isinstance(v, float):");
    indent_++; indent(); emitln("sys.stdout.write(f'{v:.5f}')");
    indent_-=1; indent(); emitln("elif isinstance(v, bool):");
    indent_++; indent(); emitln("sys.stdout.write('true' if v else 'false')");
    indent_-=1; indent(); emitln("else:");
    indent_++; indent(); emitln("sys.stdout.write(str(v))");
    indent_-=1;
emitln("");
    emitln("def _new_int_array(n): return [0 for _ in range(n)]");
emitln("def _new_obj_array(n, _factory=None):");
indent_++; indent(); emitln("return [(_factory() if _factory else None) for _ in range(n)]");
indent_--;emitln("def _index_funret(v, i):");
    indent_++; indent(); emitln("if isinstance(v, tuple):");
    indent_++; indent(); emitln("return v[i]");
    indent_-=1; indent(); emitln("return v");
emitln("");

    // body
    node->accept(this);

    // entry point
    emitln("");
    emitln("if __name__ == '__main__':");
    indent_++; indent(); emitln("try:");
    indent_++; indent(); emitln("main()");
    indent_-=1; indent(); emitln("except NameError:");
    indent_++; indent(); emitln("pass");
    indent_-=2;

    os << out_.str();
}

std::string SourceGenPython::py_tmp(){ return "_t" + std::to_string(++tmp_counter_); }

void SourceGenPython::visit(ProgramNode* node){
    for (auto def : node->definitions){
        if (def) def->accept(this);
    }
}

void SourceGenPython::visit(FunDefNode* node){
    cur_fun_ = node->name;
    locals_types_.clear();
    // def name(p1, p2, ...):
    std::ostringstream sig;
    sig << "def " << py_name(node->name) << "(";
    for (size_t i=0;i<node->params.size();++i){
        if (i) sig << ", ";
        sig << py_name(node->params[i].name);
        // track param type
        locals_types_[node->params[i].name] = node->params[i].type;
    }
    sig << "):";
    emitln(sig.str());
    indent_++;
    if (node->body) node->body->accept(this);
    // In absence of explicit return, Python would return None; mimic language by no-op
    indent_--;
    emitln("");
    cur_fun_.clear();
}

void SourceGenPython::visit(DataDefNode* /*node*/){
    // No direct code needed; new T will use dict literal with fields defaulted
}

void SourceGenPython::visit(BlockCmdNode* node){
    for (auto c : node->commands){
        if (!c) continue;
        c->accept(this);
    }
}


void SourceGenPython::visit(FunCallNode* node){
    // inline expression
    if (node->return_index){
        out_ << "_index_funret(";
        out_ << py_name(node->name) << "(";
        for (size_t i=0;i<node->args.size();++i){
            if (i) out_ << ", ";
            node->args[i]->accept(this);
        }
        out_ << "), ";
        node->return_index->accept(this);
        out_ << ")";
        return;
    }
    out_ << py_name(node->name) << "(";
    for (size_t i=0;i<node->args.size();++i){
        if (i) out_ << ", ";
        node->args[i]->accept(this);
    }
    out_ << ")";
}



void SourceGenPython::visit(FunCallCmdNode* node){
  TRACE_LN("enter SourceGenPython::visit");

    // Support capture: f(args) <x, y, a[i], p.campo>
    if (!node->lvalues.empty()){
        indent();
        for (size_t i=0;i<node->lvalues.size();++i){
            if (i) out_ << ", ";
            if (auto va = dynamic_cast<VarAccessNode*>(node->lvalues[i])){
                out_ << va->name;
            } else if (auto aa = dynamic_cast<ArrayAccessNode*>(node->lvalues[i])){
                aa->array_expr->accept(this);
                out_ << "["; aa->index_expr->accept(this); out_ << "]";
            } else if (auto fa = dynamic_cast<FieldAccessNode*>(node->lvalues[i])){
                fa->record_expr->accept(this);
                out_ << "['" << fa->field_name << "']";
            } else {
                out_ << "_";
            }
        }
        out_ << " = " << node->name << "(";
        for (size_t i=0;i<node->args.size();++i){
            if (i) out_ << ", ";
            node->args[i]->accept(this);
        }
        out_ << ")";
        emitln("");
        // update locals_types_ from function returns (capture)
        {
            auto it = fun_returns_.find(node->name);
            if (it != fun_returns_.end()){
                for (size_t i=0;i<node->lvalues.size() && i<it->second.size(); ++i){
                    if (auto va = dynamic_cast<VarAccessNode*>(node->lvalues[i])){
                        locals_types_[va->name] = it->second[i];
                    }
                }
            }
        }
        return;
    }
    // default: call and discard result
    indent();
    out_ << py_name(node->name) << "(";
    for (size_t i=0;i<node->args.size();++i){
        if (i) out_ << ", ";
        node->args[i]->accept(this);
    }
    out_ << ")"; emitln("");
}






void SourceGenPython::visit(NewExprNode* node){
  // Arrays
  if (node && !node->dims.empty()){
      // choose last non-null dimension (handles new T[][N])
      int last = -1;
      for (int j=(int)node->dims.size()-1; j>=0; --j){ if (node->dims[j]) { last=j; break; } }
      bool has_dim = (last >= 0);
      bool base_is_prim_int = node->base_type && node->base_type->element_type &&
                              node->base_type->element_type->is_primitive &&
                              node->base_type->element_type->p_type == Primitive::INT;
      bool base_is_record = node->base_type && node->base_type->element_type &&
                            !node->base_type->element_type->is_primitive &&
                            !node->base_type->element_type->is_array;
      bool base_is_prim_char = node->base_type && node->base_type->element_type &&
                               node->base_type->element_type->is_primitive &&
                               node->base_type->element_type->p_type == Primitive::CHAR;

      if (base_is_prim_int){
          out_ << "_new_int_array("; if (has_dim) node->dims[last]->accept(this); else out_ << "0"; out_ << ")";
          return;
      }
      if (base_is_record){
          // arrays of records -> create None slots; elements may be filled later
          out_ << "_new_obj_array("; if (has_dim) node->dims[last]->accept(this); else out_ << "0"; out_ << ")";
          return;
      }
      if (base_is_prim_char){
          // If there is a null earlier dim (e.g., new Char[][N]) emit outer shell of None
          bool outer_shell = false; for (int j=0;j<last; ++j){ if (!node->dims[j]) { outer_shell=true; break; } }
          if (outer_shell){ out_ << "[None for _ in range("; if (has_dim) node->dims[last]->accept(this); else out_ << "0"; out_ << ")]"; }
          else { out_ << "[chr(0) for _ in range("; if (has_dim) node->dims[last]->accept(this); else out_ << "0"; out_ << ")]"; }
          return;
      }
      // fallback: array shell (object/unknown element or array-of-arrays outer)
      out_ << "[None for _ in range("; if (has_dim) node->dims[last]->accept(this); else out_ << "0"; out_ << ")]";
      return;
  }
  // Single record literal: produce dict with default fields
  if (node && node->base_type && !node->base_type->is_primitive && !node->base_type->is_array){
      auto it = records_.find(node->base_type->user_type_name);
      out_ << "{";
      if (it != records_.end()){
          for (size_t i=0;i<it->second.size();++i){
              if (i) out_ << ", ";
              out_ << "'" << it->second[i].first << "': " << default_value_for(it->second[i].second);
          }
      }
      out_ << "}";
      return;
  }
  out_ << "None";
}





std::string SourceGenPython::default_value_for(TypeNode* t){
    if (!t) return "None";
    if (t->is_array) return "[]";
    if (t->is_primitive){
        switch (t->p_type){
            case Primitive::INT:   return "0";
            case Primitive::FLOAT: return "0.0";
            case Primitive::BOOL:  return "False";
            case Primitive::CHAR:  return "chr(0)";
        }
    }
    // Any user-defined record type -> avoid recursion/cycles
    return "None";
}


// ===== Typed READ helpers =====
std::string SourceGenPython::read_fn_for(TypeNode* t){
    if (!t) return "_read_int()";
    if (t->is_array){
        // read into array element uses element type
        return read_fn_for(t->element_type);
    }
    if (t->is_primitive){
        switch(t->p_type){
            case Primitive::INT:   return "_read_int()";
            case Primitive::FLOAT: return "_read_float()";
            case Primitive::BOOL:  return "_read_bool()";
            case Primitive::CHAR:  return "_read_char()";
            case Primitive::VOID:  return "_read_int()";
        }
    }
    // record: we can't read a dict directly; default to int (shouldn't happen for lvalue)
    return "_read_int()";
}

TypeNode* SourceGenPython::field_type_of(const std::string& record_name, const std::string& field) const{
    auto it = records_.find(record_name);
    if (it == records_.end()) return nullptr;
    for (auto const& p : it->second){
        if (p.first == field) return p.second;
    }
    return nullptr;
}

std::string SourceGenPython::record_name_of_expr(Expression* e) const{
  TRACE_LN("enter SourceGenPython::record_name_of_expr");

    if (!e) return "";
    if (auto va = dynamic_cast<VarAccessNode*>(e)){
        auto it = locals_types_.find(va->name);
        if (it != locals_types_.end()){
            TypeNode* t = it->second;
            if (t && !t->is_primitive && !t->is_array) return t->user_type_name;
        }
        return "";
    }
    if (auto fa = dynamic_cast<FieldAccessNode*>(e)){
        std::string base = record_name_of_expr(fa->record_expr);
        if (base.empty()) return "";
        TypeNode* ft = field_type_of(base, fa->field_name);
        if (ft && !ft->is_primitive && !ft->is_array) return ft->user_type_name;
        return "";
    }
    return "";

    if (auto aa = dynamic_cast<ArrayAccessNode*>(e)){
        if (auto f = dynamic_cast<FieldAccessNode*>(aa->array_expr)){
            std::string base = record_name_of_expr(f->record_expr);
            if (!base.empty()){
                TypeNode* ft = field_type_of(base, f->field_name);
                // unwrap arrays
                while (ft && ft->is_array) ft = ft->element_type;
                if (ft && !ft->is_primitive && !ft->is_array) return ft->user_type_name;
            }
            return "";
        }
        if (auto a2 = dynamic_cast<ArrayAccessNode*>(aa->array_expr)){
            return record_name_of_expr(a2->array_expr);
        }
        return "";
    }
}


TypeNode* SourceGenPython::type_of_expr(Expression* e) const{
  TRACE_LN("enter SourceGenPython::type_of_expr");

    if (!e) return nullptr;
    if (auto va = dynamic_cast<VarAccessNode*>(e)){
        auto it = locals_types_.find(va->name);
        if (it != locals_types_.end()) return it->second;
        return nullptr;
    }
    if (auto fa = dynamic_cast<FieldAccessNode*>(e)){
        std::string base = record_name_of_expr(fa->record_expr);
        if (base.empty()) return nullptr;
        return field_type_of(base, fa->field_name);
    }
    if (auto aa = dynamic_cast<ArrayAccessNode*>(e)){
        // return element type
        if (auto v = dynamic_cast<VarAccessNode*>(aa->array_expr)){
            auto it = locals_types_.find(v->name);
            if (it != locals_types_.end()){
                TypeNode* t = it->second;
                return t && t->is_array ? t->element_type : nullptr;
            }
        } else if (auto f = dynamic_cast<FieldAccessNode*>(aa->array_expr)){
            TypeNode* t = type_of_expr(f);
            return t && t->is_array ? t->element_type : nullptr;
        }
        return nullptr;
    }
    
    if (auto lit = dynamic_cast<IntLiteral*>(e)){
        auto *t = new TypeNode(Primitive::INT);
        t->is_primitive = true;
        return t;
    }
    if (auto flit = dynamic_cast<FloatLiteralNode*>(e)){
        auto *t = new TypeNode(Primitive::FLOAT);
        t->is_primitive = true;
        return t;
    }
    if (auto call = dynamic_cast<FunCallNode*>(e)){
        // prefer explicit return_index if present
        if (call->return_index){
            auto it = fun_returns_.find(call->name);
            if (it != fun_returns_.end()){
                // attempt to evaluate static index if literal
                if (auto idxlit = dynamic_cast<IntLiteral*>(call->return_index)){
                    if ((size_t)idxlit->value < it->second.size()) return it->second[idxlit->value];
                }
            }
        } else {
            // single return
            auto it = fun_returns_.find(call->name);
            if (it != fun_returns_.end() && it->second.size()==1) return it->second[0];
        }
        return nullptr;
    }
    if (auto bin = dynamic_cast<BinaryOpNode*>(e)){
        TypeNode* lt = type_of_expr(bin->left);
        TypeNode* rt = type_of_expr(bin->right);
        bool lf = (lt && lt->is_primitive && lt->p_type == Primitive::FLOAT);
        bool rf = (rt && rt->is_primitive && rt->p_type == Primitive::FLOAT);
        if (lf || rf){
            auto *t = new TypeNode(Primitive::FLOAT);
            t->is_primitive = true;
            return t;
        }
        // default to INT
        auto *t = new TypeNode(Primitive::INT);
        t->is_primitive = true;
        return t;
    }
    return nullptr;
}

// ===== Visitors =====

void SourceGenPython::visit(FieldAccessNode* node){
  TRACE_LN("enter SourceGenPython::visit");

    node->record_expr->accept(this);
    out_ << "['" << node->field_name << "']";
}

void SourceGenPython::visit(ArrayAccessNode* node){
  TRACE_LN("enter SourceGenPython::visit");

    if (dynamic_cast<FunCallNode*>(node->array_expr)){
        out_ << "_index_funret(";
        node->array_expr->accept(this);
        out_ << ", ";
        node->index_expr->accept(this);
        out_ << ")";
        return;
    }
    node->array_expr->accept(this);
    out_ << "[";
    node->index_expr->accept(this);
    out_ << "]";
}

void SourceGenPython::visit(PrintCmd* node){
    indent(); out_ << "_print(";
    node->expr->accept(this);
    out_ << ")"; emitln("");
}

void SourceGenPython::visit(ReadCmdNode* node){
    indent();
    if (auto va = dynamic_cast<VarAccessNode*>(node->lvalue)){
        TypeNode* t = type_of_expr(va);
        out_ << va->name << " = " << read_fn_for(t);
        emitln(""); 
        return;
    }
    if (auto aa = dynamic_cast<ArrayAccessNode*>(node->lvalue)){
        TypeNode* et = type_of_expr(aa); // element type
        aa->array_expr->accept(this); out_ << "["; aa->index_expr->accept(this); out_ << "] = " << read_fn_for(et); emitln(""); 
        return;
    }
    if (auto fa = dynamic_cast<FieldAccessNode*>(node->lvalue)){
        TypeNode* ft = type_of_expr(fa);
        fa->record_expr->accept(this); out_ << "['" << fa->field_name << "'] = " << read_fn_for(ft); emitln(""); 
        return;
    }
    emitln("_ = _read_int()");
}

void SourceGenPython::visit(ReturnCmdNode* node){
    indent();
    if (node->expressions.empty()){
        emitln("return");
    } else if (node->expressions.size()==1){
        out_ << "return ";
        node->expressions[0]->accept(this);
        emitln("");
    } else {
        out_ << "return (";
        for (size_t i=0;i<node->expressions.size();++i){
            if (i) out_ << ", ";
            node->expressions[i]->accept(this);
        }
        out_ << ")"; emitln("");
    }
}

void SourceGenPython::visit(VarDeclNode* node){
    // Track declared type for typed read
    locals_types_[node->name] = node->type;
    indent(); out_ << node->name << " = " << default_value_for(node->type); emitln("");
}

void SourceGenPython::visit(AssignCmdNode* node){

    // Special-case: if RHS is 'new ...', construct by LHS type and emit proper target (var/field/arr/field[i])
    if (auto ne = dynamic_cast<NewExprNode*>(node->expr)){
        TypeNode* lt = type_of_expr(node->lvalue);
        if (lt){
            // last non-null dimension if any
            Expression* dimExpr = nullptr;
            if (!ne->dims.empty()){
                for (int j=(int)ne->dims.size()-1; j>=0; --j){ if (ne->dims[j]){ dimExpr = ne->dims[j]; break; } }
            }
            auto emit_target_prefix = [this](Expression* lv){
                if (auto fv = dynamic_cast<FieldAccessNode*>(lv)){
                    fv->record_expr->accept(this);
                    out_ << "['" << fv->field_name << "']";
                    return true;
                } else if (auto vv = dynamic_cast<VarAccessNode*>(lv)){
                    out_ << py_name(vv->name);
                    return true;
                } else if (auto aa = dynamic_cast<ArrayAccessNode*>(lv)){
                    bool ok = false;
                    if (auto fv2 = dynamic_cast<FieldAccessNode*>(aa->array_expr)){
                        fv2->record_expr->accept(this);
                        out_ << "['" << fv2->field_name << "']";
                        ok = true;
                    } else if (auto vv2 = dynamic_cast<VarAccessNode*>(aa->array_expr)){
                        out_ << py_name(vv2->name);
                        ok = true;
                    } else {
                        aa->array_expr->accept(this);
                        ok = true;
                    }
                    out_ << "["; aa->index_expr->accept(this); out_ << "]";
                    return ok;
                }
                return false;
            };
            indent();
            bool emitted = emit_target_prefix(node->lvalue);
            if (!emitted){ node->lvalue->accept(this); out_ << " = "; }
            else { out_ << " = "; }
            if (lt->is_array){
                TypeNode* e = lt->element_type;
                if (e && e->is_primitive && e->p_type == Primitive::INT){
                    out_ << "_new_int_array("; if (dimExpr) dimExpr->accept(this); else out_ << "0"; out_ << ")";
                    emitln("");
                    return;
                } else if (e && e->is_array){
                    out_ << "[[] for _ in range("; if (dimExpr) dimExpr->accept(this); else out_ << "0"; out_ << ")]";
                    emitln("");
                    return;
                } else if (e && !e->is_primitive && !e->is_array){
                    out_ << "_new_obj_array("; if (dimExpr) dimExpr->accept(this); else out_ << "0"; out_ << ")";
                    emitln("");
                    return;
                } else {
                    out_ << "_new_obj_array("; if (dimExpr) dimExpr->accept(this); else out_ << "0"; out_ << ")";
                    emitln("");
                    return;
                }
            } else if (!lt->is_primitive && !lt->is_array){
                auto it = records_.find(lt->user_type_name);
                out_ << "{";
                if (it != records_.end()){
                    for (size_t i=0;i<it->second.size();++i){
                        if (i) out_ << ", ";
                        out_ << "'" << it->second[i].first << "': " << default_value_for(it->second[i].second);
                    }
                }
                out_ << "}";
                emitln("");
                return;
            }
        }
    }
    /*NEW_RHS_BY_LHS_TYPE*/

  TRACE_LN("enter SourceGenPython::visit");

    indent();
    node->lvalue->accept(this);
    out_ << " = ";
    node->expr->accept(this);
    emitln("");
}

void SourceGenPython::visit(IfCmdNode* node){
    indent(); out_ << "if ("; node->condition->accept(this); out_ << "):"; emitln("");
    indent_++; if (node->then_branch) node->then_branch->accept(this); indent_--;
    if (node->else_branch){
        indent(); emitln("else:");
        indent_++; node->else_branch->accept(this); indent_--;
    }
}


void SourceGenPython::visit(IterateCmdNode* node){
    if (!node->loop_variable.empty()){
        // Support both: iterate(i: N) and iterate(i: iterable)
        std::string t_it = py_tmp();
        std::string t_iter = py_tmp();
        indent(); out_ << t_it << " = ("; node->condition->accept(this); out_ << ")"; emitln("");
        indent(); emitln("try:");
        indent_++; indent(); out_ << t_iter << " = iter(" << t_it << ")"; emitln("");
        indent_--; indent(); emitln("except TypeError:");
        indent_++; indent(); out_ << t_iter << " = range(0, " << t_it << ")"; emitln("");
        indent_--; 
        indent(); out_ << "for __v in " << t_iter << ":"; emitln("");
        indent_++; indent(); out_ << node->loop_variable << " = __v"; emitln("");
        if (node->body) node->body->accept(this);
        indent_--; 
        return;
    }
    indent(); out_ << "for __i in range(0, ("; node->condition->accept(this); out_ << ")):"; emitln("");
    indent_++; if (node->body) node->body->accept(this); indent_--;
}


void SourceGenPython::visit(IntLiteral* node){ out_ << node->value; }
void SourceGenPython::visit(FloatLiteralNode* node){ out_ << node->value; }
void SourceGenPython::visit(CharLiteralNode* node){ out_ << repr_char(node->value); }
void SourceGenPython::visit(BoolLiteralNode* node){ out_ << (node->value ? "True" : "False"); }
void SourceGenPython::visit(VarAccessNode* node){ out_ << py_name(node->name); }

void SourceGenPython::visit(UnaryOpNode* node){
    if (node->op == '-') { out_ << "("; out_ << "-("; node->expr->accept(this); out_ << "))"; }
    else if (node->op == '!') { out_ << "(not ("; node->expr->accept(this); out_ << "))"; }
    else { node->expr->accept(this); }
}


void SourceGenPython::visit(BinaryOpNode* node){
    out_ << "(";
    node->left->accept(this);
    if (node->op == '+') out_ << " + ";
    else if (node->op == '-') out_ << " - ";
    else if (node->op == '*') out_ << " * ";
    else if (node->op == '/'){
        TypeNode* lt = type_of_expr(node->left);
        TypeNode* rt = type_of_expr(node->right);
        bool li = (lt && lt->is_primitive && lt->p_type == Primitive::INT);
        bool ri = (rt && rt->is_primitive && rt->p_type == Primitive::INT);
        out_ << ((li && ri) ? " // " : " / ");
    }
    else if (node->op == '%') out_ << " % ";
    else if (node->op == '=') out_ << " == ";
    else if (node->op == 'n') out_ << " != ";
    else if (node->op == '<') out_ << " < ";
    else if (node->op == '&') out_ << " and ";
    else out_ << " + ";
    node->right->accept(this);
    out_ << ")";
}


void SourceGenPython::visit(TypeNode* /*node*/){ out_ << "None"; }
void SourceGenPython::visit(NullLiteralNode* /*node*/){ out_ << "None"; }
