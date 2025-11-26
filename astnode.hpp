#ifndef ASTNODE_HPP
#define ASTNODE_HPP

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include "visitor.hpp"
#include "data_type.hpp"

#ifndef SCOPE_HPP
#define SCOPE_HPP

#include <map>
#include <memory>
#include <string>
#include "data_type.hpp"

enum class SymbolKind {
    VARIABLE,
    CONSTANT,
    FUNCTION
};

struct SymbolInfo {
    std::string name;
    DataType type;
    SymbolKind kind;
    bool isConstant;
    
    // For functions
    std::vector<DataType> paramTypes;
    DataType returnType;
    
    SymbolInfo() : type(DataType::IOTA), kind(SymbolKind::VARIABLE), 
                   isConstant(false), returnType(DataType::IOTA) {}
    
    SymbolInfo(const std::string& n, DataType t, SymbolKind k, bool constant = false)
        : name(n), type(t), kind(k), isConstant(constant), returnType(DataType::IOTA) {}
};

class Scope {
 private:
    std::map<std::string, std::unique_ptr<SymbolInfo>> table;
    
 public:
    std::shared_ptr<Scope> parent;
    
    explicit Scope(std::shared_ptr<Scope> parent = nullptr) : parent(parent) {}
    
    // Add a symbol to this scope
    bool addSymbol(const std::string& name, std::unique_ptr<SymbolInfo> info) {
        if (table.find(name) != table.end()) {
            return false; // Already exists in this scope
        }
        table[name] = std::move(info);
        return true;
    }
    
    // Look up a symbol in this scope only
    SymbolInfo* lookupLocal(const std::string& name) {
        auto it = table.find(name);
        if (it != table.end()) {
            return it->second.get();
        }
        return nullptr;
    }
    
    // Look up a symbol in this scope and parent scopes
    SymbolInfo* lookup(const std::string& name) {
        auto it = table.find(name);
        if (it != table.end()) {
            return it->second.get();
        }
        if (parent) {
            return parent->lookup(name);
        }
        return nullptr;
    }
    
    // Check if symbol exists in this scope only
    bool existsLocal(const std::string& name) const {
        return table.find(name) != table.end();
    }
};

#endif // SCOPE_HPP

// Forward declarations
class Visitor;

// Base class
class ASTNode {
 public:
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
    virtual void accept(Visitor& v) = 0;
    
    // For type checking
    DataType dataType = DataType::IOTA;
};

// Expression base
class ExprNode : public ASTNode {
 public:
    virtual ~ExprNode() = default;
};

// Statement base
class StmtNode : public ASTNode {
 public:
    virtual ~StmtNode() = default;
};

// Code item (can be declaration or statement)
class CodeItemNode : public ASTNode {};

// Declaration base
class DeclNode : public CodeItemNode {};

// === PARSER HELPER TYPES ===

// TypeNode - represents a type annotation in the source code
class TypeNode {
 public:
    std::string typeName;
    
    explicit TypeNode(const std::string& name) : typeName(name) {}
    
    DataType toDataType() const {
        if (typeName == "int") return DataType::INT;
        if (typeName == "float") return DataType::FLOAT;
        if (typeName == "bool") return DataType::BOOL;
        return DataType::IOTA;
    }
};

// ParamNode - represents a parameter declaration in the source code
class ParamNode {
 public:
    std::string name;
    TypeNode* type;
    
    ParamNode(const std::string& n, TypeNode* t) : name(n), type(t) {}
    
    ~ParamNode() {
        delete type;
    }
    
    DataType getDataType() const {
        return type->toDataType();
    }
};

// === EXPRESSIONS ===

// Helper nodes for literals
class IntegerNode : public ExprNode {
 public:
    int value;
    
    explicit IntegerNode(int val) : value(val) {
        dataType = DataType::INT;
    }
    
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Integer: " << value << std::endl;
    }
    
    void accept(Visitor& v) override;
};

class FloatNode : public ExprNode {
 public:
    double value;
    
    explicit FloatNode(double val) : value(val) {
        dataType = DataType::FLOAT;
    }
    
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Float: " << value << std::endl;
    }
    
    void accept(Visitor& v) override;
};

class BoolNode : public ExprNode {
 public:
    bool value;
    
    explicit BoolNode(bool val) : value(val) {
        dataType = DataType::BOOL;
    }
    
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Bool: " << (value ? "true" : "false") << std::endl;
    }
    
    void accept(Visitor& v) override;
};

class LiteralNode : public ExprNode {
 public:
    enum class LiteralType { INT, FLOAT, BOOL };
    LiteralType litType;
    int intValue;
    double floatValue;
    bool boolValue;
    
    LiteralNode(int val) : litType(LiteralType::INT), intValue(val) {
        dataType = DataType::INT;
    }
    LiteralNode(double val) : litType(LiteralType::FLOAT), floatValue(val) {
        dataType = DataType::FLOAT;
    }
    LiteralNode(bool val) : litType(LiteralType::BOOL), boolValue(val) {
        dataType = DataType::BOOL;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class IdentifierNode : public ExprNode {
 public:
    std::string name;
    
    explicit IdentifierNode(const std::string& n) : name(n) {}
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class BinaryOpNode : public ExprNode {
 public:
    ExprNode* left;
    std::string op;
    ExprNode* right;
    
    BinaryOpNode(ExprNode* l, const std::string& o, ExprNode* r) 
        : left(l), op(o), right(r) {}
    
    ~BinaryOpNode() {
        delete left;
        delete right;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class UnaryOpNode : public ExprNode {
 public:
    std::string op;
    ExprNode* operand;
    
    UnaryOpNode(const std::string& o, ExprNode* operand) : op(o), operand(operand) {}
    
    ~UnaryOpNode() {
        delete operand;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class FunctionCallNode : public ExprNode {
 public:
    std::string functionName;
    std::vector<ExprNode*> arguments;
    
    explicit FunctionCallNode(const std::string& name) : functionName(name) {}
    
    ~FunctionCallNode() {
        for (auto arg : arguments) delete arg;
    }
    
    void addArgument(ExprNode* arg) {
        arguments.push_back(arg);
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

// === STATEMENTS ===

class PrintStmtNode : public StmtNode {
 public:
    ExprNode* expression;
    
    explicit PrintStmtNode(ExprNode* expr) : expression(expr) {}
    
    ~PrintStmtNode() {
        delete expression;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class IfStmtNode : public StmtNode {
 public:
    ExprNode* condition;
    std::vector<ASTNode*> thenItems;
    std::vector<ASTNode*> elseItems;
    
    explicit IfStmtNode(ExprNode* cond) : condition(cond) {}
    
    ~IfStmtNode() {
        delete condition;
        for (auto item : thenItems) delete item;
        for (auto item : elseItems) delete item;
    }
    
    void addThenItem(ASTNode* item) {
        thenItems.push_back(item);
    }
    
    void addElseItem(ASTNode* item) {
        elseItems.push_back(item);
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class WhileStmtNode : public StmtNode {
 public:
    ExprNode* condition;
    std::vector<ASTNode*> bodyItems;
    
    explicit WhileStmtNode(ExprNode* cond) : condition(cond) {}
    
    ~WhileStmtNode() {
        delete condition;
        for (auto item : bodyItems) delete item;
    }
    
    void addBodyItem(ASTNode* item) {
        bodyItems.push_back(item);
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class AssignmentStmtNode : public StmtNode {
 public:
    std::string variableName;
    ExprNode* value;
    
    AssignmentStmtNode(const std::string& name, ExprNode* val)
        : variableName(name), value(val) {}
    
    ~AssignmentStmtNode() {
        delete value;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class ReturnStmtNode : public StmtNode {
 public:
    ExprNode* value;
    
    explicit ReturnStmtNode(ExprNode* val = nullptr) : value(val) {}
    
    ~ReturnStmtNode() {
        delete value;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class BlockNode : public StmtNode {
 public:
    std::vector<CodeItemNode*> items;
    std::shared_ptr<Scope> scope;
    
    ~BlockNode() {
        for (auto item : items) delete item;
    }
    
    void addItem(CodeItemNode* item) {
        items.push_back(item);
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class VarDeclNode : public DeclNode {
 public:
    bool isConstant;
    std::string name;
    TypeNode* typeNode;
    ExprNode* initializer;
    
    VarDeclNode(bool constant, const std::string& n, TypeNode* t, ExprNode* init)
        : isConstant(constant), name(n), typeNode(t), initializer(init) {}
    
    ~VarDeclNode() {
        delete typeNode;
        delete initializer;
    }
    
    DataType getDataType() const {
        return typeNode->toDataType();
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class AssignmentNode : public StmtNode {
 public:
    std::string variableName;
    ExprNode* value;
    
    AssignmentNode(const std::string& name, ExprNode* val)
        : variableName(name), value(val) {}
    
    ~AssignmentNode() {
        delete value;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class IfNode : public StmtNode {
 public:
    ExprNode* condition;
    StmtNode* thenBranch;
    StmtNode* elseBranch;
    
    IfNode(ExprNode* cond, StmtNode* thenB, StmtNode* elseB = nullptr)
        : condition(cond), thenBranch(thenB), elseBranch(elseB) {}
    
    ~IfNode() {
        delete condition;
        delete thenBranch;
        delete elseBranch;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class WhileNode : public StmtNode {
 public:
    ExprNode* condition;
    StmtNode* body;
    
    WhileNode(ExprNode* cond, StmtNode* b) : condition(cond), body(b) {}
    
    ~WhileNode() {
        delete condition;
        delete body;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class ReturnNode : public StmtNode {
 public:
    ExprNode* value;
    
    explicit ReturnNode(ExprNode* val = nullptr) : value(val) {}
    
    ~ReturnNode() {
        delete value;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class PrintNode : public StmtNode {
 public:
    ExprNode* expression;
    
    explicit PrintNode(ExprNode* expr) : expression(expr) {}
    
    ~PrintNode() {
        delete expression;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

class ExprStmtNode : public StmtNode {
 public:
    ExprNode* expression;
    
    explicit ExprStmtNode(ExprNode* expr) : expression(expr) {}
    
    ~ExprStmtNode() {
        delete expression;
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

// === DECLARATIONS ===

struct Parameter {
    std::string name;
    DataType type;
    
    Parameter(const std::string& n, DataType t) : name(n), type(t) {}
};

class FunctionDeclNode : public DeclNode {
 public:
    std::string name;
    std::vector<Parameter> parameters;
    DataType returnType;
    std::vector<ASTNode*> bodyItems;
    
    FunctionDeclNode(const std::string& n, TypeNode* retType)
        : name(n), returnType(retType->toDataType()) {}
    
    ~FunctionDeclNode() {
        for (auto item : bodyItems) delete item;
    }
    
    void addParameter(ParamNode* param) {
        parameters.push_back(Parameter(param->name, param->getDataType()));
    }
    
    void addBodyItem(ASTNode* item) {
        bodyItems.push_back(item);
    }
    
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

// === PROGRAM ===

class ProgramNode : public ASTNode {
 public:
    std::vector<DeclNode*> declarations;
    std::shared_ptr<Scope> scope;
    
    ~ProgramNode();
    void addDecl(DeclNode* decl);
    void print(int indent = 0) const override;
    void accept(Visitor& v) override;
};

#endif // ASTNODE_HPP