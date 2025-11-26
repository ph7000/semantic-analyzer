#ifndef VISITOR_HPP
#define VISITOR_HPP

// Forward declarations
class ProgramNode;
class FunctionDeclNode;
class VarDeclNode;
class BlockNode;
class IfNode;
class WhileNode;
class ReturnNode;
class PrintNode;
class AssignmentNode;
class ExprStmtNode;
class BinaryOpNode;
class UnaryOpNode;
class LiteralNode;
class IdentifierNode;
class FunctionCallNode;

// Parser-specific node types
class IntegerNode;
class FloatNode;
class BoolNode;
class PrintStmtNode;
class IfStmtNode;
class WhileStmtNode;
class AssignmentStmtNode;
class ReturnStmtNode;

class Visitor {
 public:
    virtual ~Visitor() = default;

    // Original semantic analyzer nodes
    virtual void visit(ProgramNode* node) = 0;
    virtual void visit(FunctionDeclNode* node) = 0;
    virtual void visit(VarDeclNode* node) = 0;
    virtual void visit(BlockNode* node) = 0;
    virtual void visit(IfNode* node) = 0;
    virtual void visit(WhileNode* node) = 0;
    virtual void visit(ReturnNode* node) = 0;
    virtual void visit(PrintNode* node) = 0;
    virtual void visit(AssignmentNode* node) = 0;
    virtual void visit(ExprStmtNode* node) = 0;
    virtual void visit(BinaryOpNode* node) = 0;
    virtual void visit(UnaryOpNode* node) = 0;
    virtual void visit(LiteralNode* node) = 0;
    virtual void visit(IdentifierNode* node) = 0;
    virtual void visit(FunctionCallNode* node) = 0;
    
    // Parser-specific literal nodes
    virtual void visit(IntegerNode* node) = 0;
    virtual void visit(FloatNode* node) = 0;
    virtual void visit(BoolNode* node) = 0;
    
    // Parser-specific statement nodes
    virtual void visit(PrintStmtNode* node) = 0;
    virtual void visit(IfStmtNode* node) = 0;
    virtual void visit(WhileStmtNode* node) = 0;
    virtual void visit(AssignmentStmtNode* node) = 0;
    virtual void visit(ReturnStmtNode* node) = 0;
};

#endif /* VISITOR_HPP */