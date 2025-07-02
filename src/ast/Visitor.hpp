#ifndef VISITOR_HPP
#define VISITOR_HPP

// Forward declarations
class ProgramNode;
class FunDefNode;
class DataDefNode; // NOVO
class BlockCmdNode;
class FunCallNode;
class FunCallCmdNode;
class NewExprNode;     // NOVO
class FieldAccessNode; // NOVO
class PrintCmd;
class ReadCmdNode;
class ReturnCmdNode;
class VarDeclNode;
class AssignCmdNode;
class IfCmdNode;
class IterateCmdNode;
class IntLiteral;
class FloatLiteralNode;
class CharLiteralNode;
class BoolLiteralNode;
class VarAccessNode;
class UnaryOpNode;
class BinaryOpNode;
class TypeNode;
class NullLiteralNode; // NOVO
class ArrayAccessNode; // NOVO

class Visitor
{
public:
    virtual ~Visitor() = default;
    virtual void visit(ProgramNode *node) = 0;
    virtual void visit(FunDefNode *node) = 0;
    virtual void visit(DataDefNode *node) = 0; // NOVO
    virtual void visit(BlockCmdNode *node) = 0;
    virtual void visit(FunCallNode *node) = 0;
    virtual void visit(FunCallCmdNode *node) = 0;
    virtual void visit(NewExprNode *node) = 0;     // NOVO
    virtual void visit(FieldAccessNode *node) = 0; // NOVO
    virtual void visit(PrintCmd *node) = 0;
    virtual void visit(ReadCmdNode *node) = 0;
    virtual void visit(ReturnCmdNode *node) = 0;
    virtual void visit(VarDeclNode *node) = 0;
    virtual void visit(AssignCmdNode *node) = 0;
    virtual void visit(IfCmdNode *node) = 0;
    virtual void visit(IterateCmdNode *node) = 0;
    virtual void visit(IntLiteral *node) = 0;
    virtual void visit(FloatLiteralNode *node) = 0;
    virtual void visit(CharLiteralNode *node) = 0;
    virtual void visit(BoolLiteralNode *node) = 0;
    virtual void visit(VarAccessNode *node) = 0;
    virtual void visit(UnaryOpNode *node) = 0;
    virtual void visit(BinaryOpNode *node) = 0;
    virtual void visit(TypeNode *node) = 0;
    virtual void visit(NullLiteralNode *node) = 0;
    virtual void visit(ArrayAccessNode *node) = 0;
};
#endif