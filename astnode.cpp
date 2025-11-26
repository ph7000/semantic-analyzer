#include <iostream>
#include "astnode.hpp"
#include "visitor.hpp"
#include "data_type.hpp"

void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) std::cout << " ";
}

// ============================================================================
// PROGRAM NODE
// ============================================================================

ProgramNode::~ProgramNode() {
    for (auto& decl : declarations) delete decl;
}

void ProgramNode::addDecl(DeclNode* decl) {
    declarations.push_back(decl);
}

void ProgramNode::print(int indent) const {
    printIndent(indent);
    std::cout << "ProgramNode:\n";
    for (auto& decl : declarations) decl->print(indent + 2);
}

void ProgramNode::accept(Visitor& v) {
    v.visit(this);
}

// ============================================================================
// EXPRESSION NODES - LITERALS
// ============================================================================

void IntegerNode::accept(Visitor& v) {
    v.visit(this);
}

void FloatNode::accept(Visitor& v) {
    v.visit(this);
}

void BoolNode::accept(Visitor& v) {
    v.visit(this);
}

void LiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "LiteralNode: ";
    switch (litType) {
        case LiteralType::INT:
            std::cout << intValue << " (int)\n";
            break;
        case LiteralType::FLOAT:
            std::cout << floatValue << " (float)\n";
            break;
        case LiteralType::BOOL:
            std::cout << (boolValue ? "true" : "false") << " (bool)\n";
            break;
    }
}

void LiteralNode::accept(Visitor& v) {
    v.visit(this);
}

void IdentifierNode::print(int indent) const {
    printIndent(indent);
    std::cout << "IdentifierNode: " << name << "\n";
}

void IdentifierNode::accept(Visitor& v) {
    v.visit(this);
}

void BinaryOpNode::print(int indent) const {
    printIndent(indent);
    std::cout << "BinaryOpNode: " << op << "\n";
    left->print(indent + 2);
    right->print(indent + 2);
}

void BinaryOpNode::accept(Visitor& v) {
    v.visit(this);
}

void UnaryOpNode::print(int indent) const {
    printIndent(indent);
    std::cout << "UnaryOpNode: " << op << "\n";
    operand->print(indent + 2);
}

void UnaryOpNode::accept(Visitor& v) {
    v.visit(this);
}

void FunctionCallNode::print(int indent) const {
    printIndent(indent);
    std::cout << "FunctionCallNode: " << functionName << "\n";
    for (auto arg : arguments) {
        arg->print(indent + 2);
    }
}

void FunctionCallNode::accept(Visitor& v) {
    v.visit(this);
}

// ============================================================================
// STATEMENT NODES
// ============================================================================

void PrintStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "PrintStmtNode:\n";
    expression->print(indent + 2);
}

void PrintStmtNode::accept(Visitor& v) {
    v.visit(this);
}

void IfStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "IfStmtNode:\n";
    printIndent(indent + 2);
    std::cout << "Condition:\n";
    condition->print(indent + 4);
    printIndent(indent + 2);
    std::cout << "Then:\n";
    for (auto item : thenItems) {
        item->print(indent + 4);
    }
    if (!elseItems.empty()) {
        printIndent(indent + 2);
        std::cout << "Else:\n";
        for (auto item : elseItems) {
            item->print(indent + 4);
        }
    }
}

void IfStmtNode::accept(Visitor& v) {
    v.visit(this);
}

void WhileStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "WhileStmtNode:\n";
    printIndent(indent + 2);
    std::cout << "Condition:\n";
    condition->print(indent + 4);
    printIndent(indent + 2);
    std::cout << "Body:\n";
    for (auto item : bodyItems) {
        item->print(indent + 4);
    }
}

void WhileStmtNode::accept(Visitor& v) {
    v.visit(this);
}

void AssignmentStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "AssignmentStmtNode: " << variableName << " =\n";
    value->print(indent + 2);
}

void AssignmentStmtNode::accept(Visitor& v) {
    v.visit(this);
}

void ReturnStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "ReturnStmtNode";
    if (value) {
        std::cout << ":\n";
        value->print(indent + 2);
    } else {
        std::cout << "\n";
    }
}

void ReturnStmtNode::accept(Visitor& v) {
    v.visit(this);
}

void BlockNode::print(int indent) const {
    printIndent(indent);
    std::cout << "BlockNode:\n";
    for (auto item : items) {
        item->print(indent + 2);
    }
}

void BlockNode::accept(Visitor& v) {
    v.visit(this);
}

void AssignmentNode::print(int indent) const {
    printIndent(indent);
    std::cout << "AssignmentNode: " << variableName << " :=\n";
    value->print(indent + 2);
}

void AssignmentNode::accept(Visitor& v) {
    v.visit(this);
}

void IfNode::print(int indent) const {
    printIndent(indent);
    std::cout << "IfNode:\n";
    printIndent(indent + 2);
    std::cout << "Condition:\n";
    condition->print(indent + 4);
    printIndent(indent + 2);
    std::cout << "Then:\n";
    thenBranch->print(indent + 4);
    if (elseBranch) {
        printIndent(indent + 2);
        std::cout << "Else:\n";
        elseBranch->print(indent + 4);
    }
}

void IfNode::accept(Visitor& v) {
    v.visit(this);
}

void WhileNode::print(int indent) const {
    printIndent(indent);
    std::cout << "WhileNode:\n";
    printIndent(indent + 2);
    std::cout << "Condition:\n";
    condition->print(indent + 4);
    printIndent(indent + 2);
    std::cout << "Body:\n";
    body->print(indent + 4);
}

void WhileNode::accept(Visitor& v) {
    v.visit(this);
}

void ReturnNode::print(int indent) const {
    printIndent(indent);
    std::cout << "ReturnNode";
    if (value) {
        std::cout << ":\n";
        value->print(indent + 2);
    } else {
        std::cout << "\n";
    }
}

void ReturnNode::accept(Visitor& v) {
    v.visit(this);
}

void PrintNode::print(int indent) const {
    printIndent(indent);
    std::cout << "PrintNode:\n";
    expression->print(indent + 2);
}

void PrintNode::accept(Visitor& v) {
    v.visit(this);
}

void ExprStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "ExprStmtNode:\n";
    expression->print(indent + 2);
}

void ExprStmtNode::accept(Visitor& v) {
    v.visit(this);
}

// ============================================================================
// DECLARATION NODES
// ============================================================================

void VarDeclNode::print(int indent) const {
    printIndent(indent);
    std::cout << (isConstant ? "ConstDeclNode: " : "VarDeclNode: ")
              << name << " : " << typeNode->typeName;
    if (initializer) {
        std::cout << " =\n";
        initializer->print(indent + 2);
    } else {
        std::cout << "\n";
    }
}

void VarDeclNode::accept(Visitor& v) {
    v.visit(this);
}

void FunctionDeclNode::print(int indent) const {
    printIndent(indent);
    std::cout << "FunctionDeclNode: " << name << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        std::cout << parameters[i].name << ":" << parameters[i].type;
        if (i < parameters.size() - 1) std::cout << ", ";
    }
    std::cout << ") -> " << returnType << "\n";
    printIndent(indent + 2);
    std::cout << "Body:\n";
    for (auto item : bodyItems) {
        item->print(indent + 4);
    }
}

void FunctionDeclNode::accept(Visitor& v) {
    v.visit(this);
}