#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include "astnode.hpp"
#include "visitor.hpp"
#include "exception.hpp"
#include "data_type.hpp"
#include <memory>
#include <string>
#include <vector>

class SemanticAnalyzer : public Visitor {
 private:
    ASTNode* root;
    std::shared_ptr<Scope> currentScope;
    std::string currentFunction;
    DataType currentFunctionReturnType;
    bool hasReturn;
    bool isUnreachable;
    
    // Helper methods for analysis - updated to use parser node types
    void analyzeProgram(ProgramNode* node);
    void analyzeFunctionDecl(FunctionDeclNode* node);
    void analyzeVarDecl(VarDeclNode* node);
    void analyzeStmt(StmtNode* node);
    void analyzeAssignment(AssignmentStmtNode* node);  // Changed from AssignmentNode*
    void analyzeReturn(ReturnStmtNode* node);          // Changed from ReturnNode*
    void analyzePrint(PrintStmtNode* node);            // Changed from PrintNode*
    void analyzeIf(IfStmtNode* node);                  // Changed from IfNode*
    void analyzeWhile(WhileStmtNode* node);            // Changed from WhileNode*
    void analyzeBlock(const std::vector<ASTNode*>& block, bool createNewScope = false);  // Changed signature
    
    DataType analyzeExpr(ExprNode* expr);
    DataType stringToDataType(const std::string& typeStr);
    bool isNumericType(DataType type);
    bool isComparable(DataType type);
    bool isAssignmentCompatible(DataType target, DataType source);  // New: type compatibility check
    
    bool checkPathsReturn(const std::vector<ASTNode*>& block);  // Changed signature
    bool isTerminator(ASTNode* node);
    
 public:
    explicit SemanticAnalyzer(ASTNode* root) 
        : root(root), currentScope(nullptr), 
          currentFunctionReturnType(DataType::IOTA),
          hasReturn(false), isUnreachable(false) {}
    
    void analyze();
    
    // Visitor interface - semantic analyzer nodes
    void visit(ProgramNode* node) override;
    void visit(FunctionDeclNode* node) override;
    void visit(VarDeclNode* node) override;
    void visit(BlockNode* node) override;
    void visit(IfNode* node) override;
    void visit(WhileNode* node) override;
    void visit(ReturnNode* node) override;
    void visit(PrintNode* node) override;
    void visit(AssignmentNode* node) override;
    void visit(ExprStmtNode* node) override;
    void visit(BinaryOpNode* node) override;
    void visit(UnaryOpNode* node) override;
    void visit(LiteralNode* node) override;
    void visit(IdentifierNode* node) override;
    void visit(FunctionCallNode* node) override;
    
    // Visitor interface - parser-specific literal nodes
    void visit(IntegerNode* node) override;
    void visit(FloatNode* node) override;
    void visit(BoolNode* node) override;
    
    // Visitor interface - parser-specific statement nodes
    void visit(PrintStmtNode* node) override;
    void visit(IfStmtNode* node) override;
    void visit(WhileStmtNode* node) override;
    void visit(AssignmentStmtNode* node) override;
    void visit(ReturnStmtNode* node) override;
};

#endif // SEMANTIC_ANALYZER_HPP