#include "semantic_analyzer.hpp"
#include <iostream>

// Main entry point
void SemanticAnalyzer::analyze() {
    if (!root) {
        throw std::runtime_error("AST root is null");
    }
    
    ProgramNode* program = dynamic_cast<ProgramNode*>(root);
    if (!program) {
        throw std::runtime_error("Root is not a ProgramNode");
    }
    
    analyzeProgram(program);
}

void SemanticAnalyzer::visit(IntegerNode* node) {
    // Integer literals are already typed correctly
    node->dataType = DataType::INT;
}

void SemanticAnalyzer::visit(FloatNode* node) {
    // Float literals are already typed correctly
    node->dataType = DataType::FLOAT;
}

void SemanticAnalyzer::visit(BoolNode* node) {
    // Boolean literals are already typed correctly
    node->dataType = DataType::BOOL;
}

// Convert string type to DataType enum
DataType SemanticAnalyzer::stringToDataType(const std::string& typeStr) {
    if (typeStr == "int") return DataType::INT;
    if (typeStr == "float") return DataType::FLOAT;
    if (typeStr == "bool") return DataType::BOOL;
    return DataType::IOTA;
}

bool SemanticAnalyzer::isNumericType(DataType type) {
    return type == DataType::INT || type == DataType::FLOAT;
}

bool SemanticAnalyzer::isComparable(DataType type) {
    return type == DataType::INT || type == DataType::FLOAT || type == DataType::BOOL;
}

// Check if source type is assignment-compatible with target type
// Based on spec section 2.2: Type Compatibility Rules
bool SemanticAnalyzer::isAssignmentCompatible(DataType target, DataType source) {
    // Same type is always compatible
    if (target == source) {
        return true;
    }
    
    // BOOL can accept INT (Tolerated)
    if (target == DataType::BOOL && source == DataType::INT) {
        return true;
    }
    
    // FLOAT can accept INT (Widening)
    if (target == DataType::FLOAT && source == DataType::INT) {
        return true;
    }
    
    // INT can accept BOOL (Tolerated)
    if (target == DataType::INT && source == DataType::BOOL) {
        return true;
    }
    
    // All other combinations are incompatible
    // Specifically: INT cannot accept FLOAT, BOOL cannot accept FLOAT
    return false;
}

// Analyze program
void SemanticAnalyzer::analyzeProgram(ProgramNode* node) {
    // Create global scope
    currentScope = std::make_shared<Scope>(nullptr);
    
    // First pass: Register all function declarations
    for (auto decl : node->declarations) {
        if (FunctionDeclNode* funcDecl = dynamic_cast<FunctionDeclNode*>(decl)) {
            // Check if identifier already declared (could be function or variable)
            if (currentScope->existsLocal(funcDecl->name)) {
                // Check what kind of symbol it is
                SymbolInfo* existing = currentScope->lookupLocal(funcDecl->name);
                if (existing && existing->kind == SymbolKind::FUNCTION) {
                    throw SemanticException(
                        SemanticErrorType::REDECLARED_FUNCTION,
                        SemanticErrorContext::Function(funcDecl->name)
                    );
                } else {
                    // It's a variable - this is also an error
                    throw SemanticException(
                        SemanticErrorType::REDECLARED_IDENTIFIER,
                        SemanticErrorContext::Identifier(funcDecl->name)
                    );
                }
            }
            
            // Add function to symbol table
            auto funcInfo = std::make_unique<SymbolInfo>();
            funcInfo->name = funcDecl->name;
            funcInfo->kind = SymbolKind::FUNCTION;
            funcInfo->returnType = funcDecl->returnType;
            
            for (const auto& param : funcDecl->parameters) {
                funcInfo->paramTypes.push_back(param.type);
            }
            
            currentScope->addSymbol(funcDecl->name, std::move(funcInfo));
        }
    }
    
    // Second pass: Analyze all declarations
    for (auto decl : node->declarations) {
        if (FunctionDeclNode* funcDecl = dynamic_cast<FunctionDeclNode*>(decl)) {
            analyzeFunctionDecl(funcDecl);
        } else if (VarDeclNode* varDecl = dynamic_cast<VarDeclNode*>(decl)) {
            analyzeVarDecl(varDecl);
        }
    }
}

// Analyze function declaration
void SemanticAnalyzer::analyzeFunctionDecl(FunctionDeclNode* node) {
    // Create new scope for function
    auto parentScope = currentScope;
    currentScope = std::make_shared<Scope>(parentScope);
    
    // Save current function context
    std::string previousFunction = currentFunction;
    DataType previousReturnType = currentFunctionReturnType;
    bool previousHasReturn = hasReturn;
    bool previousUnreachable = isUnreachable;
    
    currentFunction = node->name;
    currentFunctionReturnType = node->returnType;
    hasReturn = false;
    isUnreachable = false;
    
    // Add parameters to function scope
    for (const auto& param : node->parameters) {
        if (currentScope->existsLocal(param.name)) {
            throw SemanticException(
                SemanticErrorType::REDECLARED_IDENTIFIER,
                SemanticErrorContext::Identifier(param.name)
            );
        }
        
        auto paramInfo = std::make_unique<SymbolInfo>(
            param.name,
            param.type,
            SymbolKind::VARIABLE,
            false
        );
        currentScope->addSymbol(param.name, std::move(paramInfo));
    }
    
    // Analyze function body
    analyzeBlock(node->bodyItems, false);
    
    // Check if function has return on all paths (if not void/IOTA)
    if (currentFunctionReturnType != DataType::IOTA) {
        if (!checkPathsReturn(node->bodyItems)) {
            throw SemanticException(
                SemanticErrorType::MISSING_RETURN,
                SemanticErrorContext::Function(node->name)
            );
        }
    }
    
    // Restore context
    currentFunction = previousFunction;
    currentFunctionReturnType = previousReturnType;
    hasReturn = previousHasReturn;
    isUnreachable = previousUnreachable;
    currentScope = parentScope;
}

// Analyze variable declaration
void SemanticAnalyzer::analyzeVarDecl(VarDeclNode* node) {
    if (isUnreachable) {
        throw SemanticException(
            SemanticErrorType::UNREACHABLE_CODE,
            SemanticErrorContext()
        );
    }
    
    // Check if identifier already declared in current scope
    if (currentScope->existsLocal(node->name)) {
        // Check what kind of symbol it is
        SymbolInfo* existing = currentScope->lookupLocal(node->name);
        if (existing && existing->kind == SymbolKind::FUNCTION) {
            // Trying to redeclare a function as a variable
            throw SemanticException(
                SemanticErrorType::REDECLARED_FUNCTION,
                SemanticErrorContext::Function(node->name)
            );
        } else {
            // Redeclaring a variable or constant
            throw SemanticException(
                SemanticErrorType::REDECLARED_IDENTIFIER,
                SemanticErrorContext::Identifier(node->name)
            );
        }
    }
    
    DataType declaredType = node->getDataType();
    
    // Check initializer type if present
    if (node->initializer) {
        DataType initType = analyzeExpr(node->initializer);
        
        // Use assignment compatibility rules from spec section 2.2
        if (!isAssignmentCompatible(declaredType, initType)) {
            throw SemanticException(
                SemanticErrorType::VAR_DECL_TYPE_MISMATCH,
                SemanticErrorContext::IdentifierTypeMismatch(
                    node->name, declaredType, initType
                )
            );
        }
    }
    
    // Add variable to symbol table
    auto varInfo = std::make_unique<SymbolInfo>(
        node->name,
        declaredType,
        SymbolKind::VARIABLE,
        node->isConstant
    );
    currentScope->addSymbol(node->name, std::move(varInfo));
}

// Analyze statement
void SemanticAnalyzer::analyzeStmt(StmtNode* node) {
    if (AssignmentStmtNode* assign = dynamic_cast<AssignmentStmtNode*>(node)) {
        analyzeAssignment(assign);
    } else if (ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(node)) {
        analyzeReturn(ret);
    } else if (PrintStmtNode* print = dynamic_cast<PrintStmtNode*>(node)) {
        analyzePrint(print);
    } else if (IfStmtNode* ifStmt = dynamic_cast<IfStmtNode*>(node)) {
        analyzeIf(ifStmt);
    } else if (WhileStmtNode* whileStmt = dynamic_cast<WhileStmtNode*>(node)) {
        analyzeWhile(whileStmt);
    }
}

// Analyze assignment
void SemanticAnalyzer::analyzeAssignment(AssignmentStmtNode* node) {
    if (isUnreachable) {
        throw SemanticException(
            SemanticErrorType::UNREACHABLE_CODE,
            SemanticErrorContext()
        );
    }
    
    // Check if variable exists
    SymbolInfo* symbol = currentScope->lookup(node->variableName);
    
    if (!symbol) {
        throw SemanticException(
            SemanticErrorType::UNDECLARED_IDENTIFIER,
            SemanticErrorContext::Identifier(node->variableName)
        );
    }
    
    // Check if it's a function
    if (symbol->kind == SymbolKind::FUNCTION) {
        throw SemanticException(
            SemanticErrorType::FUNCTION_USED_AS_VARIABLE,
            SemanticErrorContext::Function(node->variableName)
        );
    }
    
    // Check if trying to assign to constant
    if (symbol->isConstant) {
        throw SemanticException(
            SemanticErrorType::VAR_ASSIGN_TO_CONSTANT,
            SemanticErrorContext::Identifier(node->variableName)
        );
    }
    
    DataType valueType = analyzeExpr(node->value);
    
    // Use assignment compatibility rules from spec section 2.2
    if (!isAssignmentCompatible(symbol->type, valueType)) {
        throw SemanticException(
            SemanticErrorType::VAR_ASSIGN_TYPE_MISMATCH,
            SemanticErrorContext::IdentifierTypeMismatch(
                node->variableName, symbol->type, valueType
            )
        );
    }
}

// Analyze return statement
void SemanticAnalyzer::analyzeReturn(ReturnStmtNode* node) {
    if (isUnreachable) {
        throw SemanticException(
            SemanticErrorType::UNREACHABLE_CODE,
            SemanticErrorContext()
        );
    }
    
    if (currentFunction.empty()) {
        throw SemanticException(
            SemanticErrorType::RETURN_OUTSIDE_FUNCTION,
            SemanticErrorContext()
        );
    }
    
    if (node->value) {
        DataType returnType = analyzeExpr(node->value);
        
        // Use assignment compatibility rules from spec section 2.2
        if (!isAssignmentCompatible(currentFunctionReturnType, returnType)) {
            throw SemanticException(
                SemanticErrorType::RETURN_TYPE_MISMATCH,
                SemanticErrorContext::ReturnTypeMismatch(
                    currentFunction, currentFunctionReturnType, returnType
                )
            );
        }
    } else {
        if (currentFunctionReturnType != DataType::IOTA) {
            throw SemanticException(
                SemanticErrorType::RETURN_TYPE_MISMATCH,
                SemanticErrorContext::ReturnTypeMismatch(
                    currentFunction, currentFunctionReturnType, DataType::IOTA
                )
            );
        }
    }
    
    hasReturn = true;
    isUnreachable = true;
}

// Analyze print statement
void SemanticAnalyzer::analyzePrint(PrintStmtNode* node) {
    if (isUnreachable) {
        throw SemanticException(
            SemanticErrorType::UNREACHABLE_CODE,
            SemanticErrorContext()
        );
    }
    
    analyzeExpr(node->expression);
}

// Analyze if statement
void SemanticAnalyzer::analyzeIf(IfStmtNode* node) {
    if (isUnreachable) {
        throw SemanticException(
            SemanticErrorType::UNREACHABLE_CODE,
            SemanticErrorContext()
        );
    }
    
    DataType condType = analyzeExpr(node->condition);
    
    if (condType != DataType::BOOL) {
        throw SemanticException(
            SemanticErrorType::CONDITION_NOT_BOOL,
            SemanticErrorContext::ActualType(condType)
        );
    }
    
    bool prevUnreachable = isUnreachable;
    
    analyzeBlock(node->thenItems, true);
    bool thenUnreachable = isUnreachable;
    
    isUnreachable = prevUnreachable;
    
    if (!node->elseItems.empty()) {
        analyzeBlock(node->elseItems, true);
        bool elseUnreachable = isUnreachable;
        
        if (thenUnreachable && elseUnreachable) {
            isUnreachable = true;
        } else {
            isUnreachable = prevUnreachable;
        }
    } else {
        isUnreachable = prevUnreachable;
    }
}

// Analyze while statement
void SemanticAnalyzer::analyzeWhile(WhileStmtNode* node) {
    if (isUnreachable) {
        throw SemanticException(
            SemanticErrorType::UNREACHABLE_CODE,
            SemanticErrorContext()
        );
    }
    
    DataType condType = analyzeExpr(node->condition);
    
    if (condType != DataType::BOOL) {
        throw SemanticException(
            SemanticErrorType::CONDITION_NOT_BOOL,
            SemanticErrorContext::ActualType(condType)
        );
    }
    
    bool prevUnreachable = isUnreachable;
    analyzeBlock(node->bodyItems, true);
    isUnreachable = prevUnreachable;
}

// Analyze block
void SemanticAnalyzer::analyzeBlock(const std::vector<ASTNode*>& block, bool createNewScope) {
    auto parentScope = currentScope;
    
    if (createNewScope) {
        currentScope = std::make_shared<Scope>(parentScope);
    }
    
    bool blockUnreachable = false;
    
    for (size_t i = 0; i < block.size(); ++i) {
        auto item = block[i];
        
        if (blockUnreachable) {
            throw SemanticException(
                SemanticErrorType::UNREACHABLE_CODE,
                SemanticErrorContext()
            );
        }
        
        if (FunctionDeclNode* funcDecl = dynamic_cast<FunctionDeclNode*>(item)) {
            analyzeFunctionDecl(funcDecl);
        } else if (VarDeclNode* varDecl = dynamic_cast<VarDeclNode*>(item)) {
            analyzeVarDecl(varDecl);
        } else if (StmtNode* stmt = dynamic_cast<StmtNode*>(item)) {
            analyzeStmt(stmt);
            if (isTerminator(stmt)) {
                blockUnreachable = true;
                isUnreachable = true;
            }
        }
    }
    
    if (createNewScope) {
        currentScope = parentScope;
    }
}

// Analyze expression
DataType SemanticAnalyzer::analyzeExpr(ExprNode* expr) {
    if (dynamic_cast<IntegerNode*>(expr)) {
        return DataType::INT;
    }
    else if (dynamic_cast<FloatNode*>(expr)) {
        return DataType::FLOAT;
    }
    else if (dynamic_cast<BoolNode*>(expr)) {
        return DataType::BOOL;
    }
    else if (IdentifierNode* idNode = dynamic_cast<IdentifierNode*>(expr)) {
        SymbolInfo* symbol = currentScope->lookup(idNode->name);
        
        if (!symbol) {
            throw SemanticException(
                SemanticErrorType::UNDECLARED_IDENTIFIER,
                SemanticErrorContext::Identifier(idNode->name)
            );
        }
        
        if (symbol->kind == SymbolKind::FUNCTION) {
            throw SemanticException(
                SemanticErrorType::FUNCTION_USED_AS_VARIABLE,
                SemanticErrorContext::Function(idNode->name)
            );
        }
        
        return symbol->type;
    }
    else if (BinaryOpNode* binOp = dynamic_cast<BinaryOpNode*>(expr)) {
        DataType leftType = analyzeExpr(binOp->left);
        DataType rightType = analyzeExpr(binOp->right);
        
        if (binOp->op == "+" || binOp->op == "-" || binOp->op == "*" || binOp->op == "/") {
            // Arithmetic operations - both operands must be numeric
            // Per spec section 2.1.B: Any operand is BOOL is an error
            if (!isNumericType(leftType) || !isNumericType(rightType)) {
                throw SemanticException(
                    SemanticErrorType::INVALID_BINARY_OPERATION,
                    SemanticErrorContext::InvalidOperationBetweenTypes(
                        binOp->op, leftType, rightType
                    )
                );
            }
            // Result is FLOAT if either operand is FLOAT, otherwise INT
            if (leftType == DataType::FLOAT || rightType == DataType::FLOAT) {
                return DataType::FLOAT;
            }
            return DataType::INT;
        }
        else if (binOp->op == "<" || binOp->op == ">" || binOp->op == "<=" || binOp->op == ">=") {
            // Comparison operators - both operands must be numeric (INT or FLOAT)
            // Per spec section 2.1.B: Comparison between BOOL and numeric is an error
            if (!isNumericType(leftType) || !isNumericType(rightType)) {
                throw SemanticException(
                    SemanticErrorType::INVALID_BINARY_OPERATION,
                    SemanticErrorContext::InvalidOperationBetweenTypes(
                        binOp->op, leftType, rightType
                    )
                );
            }
            return DataType::BOOL;
        }
        else if (binOp->op == "==" || binOp->op == "!=") {
            // Equality operators - both operands must be of the same type
            // Per spec section 2.1.B: Both must be same type (e.g., BOOL == BOOL)
            if (leftType != rightType) {
                throw SemanticException(
                    SemanticErrorType::INVALID_BINARY_OPERATION,
                    SemanticErrorContext::InvalidOperationBetweenTypes(
                        binOp->op, leftType, rightType
                    )
                );
            }
            return DataType::BOOL;
        }
    }
    else if (UnaryOpNode* unOp = dynamic_cast<UnaryOpNode*>(expr)) {
        DataType operandType = analyzeExpr(unOp->operand);
        
        if (unOp->op == "-") {
            // Per spec section 2.1.A: Unary minus requires numeric operand
            if (!isNumericType(operandType)) {
                throw SemanticException(
                    SemanticErrorType::INVALID_UNARY_OPERATION,
                    SemanticErrorContext::ActualType(operandType)
                );
            }
            return operandType;
        }
    }
    else if (FunctionCallNode* callNode = dynamic_cast<FunctionCallNode*>(expr)) {
        SymbolInfo* symbol = currentScope->lookup(callNode->functionName);
        
        if (!symbol) {
            throw SemanticException(
                SemanticErrorType::UNDECLARED_FUNCTION,
                SemanticErrorContext::Function(callNode->functionName)
            );
        }
        
        if (symbol->kind != SymbolKind::FUNCTION) {
            throw SemanticException(
                SemanticErrorType::NOT_A_FUNCTION,
                SemanticErrorContext::Identifier(callNode->functionName)
            );
        }
        
        // Check argument count
        if (callNode->arguments.size() != symbol->paramTypes.size()) {
            throw SemanticException(
                SemanticErrorType::WRONG_NUMBER_OF_ARGUMENTS,
                SemanticErrorContext::ArgCount(
                    callNode->functionName,
                    symbol->paramTypes.size(),
                    callNode->arguments.size()
                )
            );
        }
        
        // Check argument types - use assignment compatibility per spec section 2.3
        std::vector<DataType> actualTypes;
        for (size_t i = 0; i < callNode->arguments.size(); ++i) {
            DataType argType = analyzeExpr(callNode->arguments[i]);
            actualTypes.push_back(argType);
            
            // Each argument type must be assignment-compatible with parameter type
            if (!isAssignmentCompatible(symbol->paramTypes[i], argType)) {
                throw SemanticException(
                    SemanticErrorType::INVALID_SIGNATURE,
                    SemanticErrorContext::Signature(
                        callNode->functionName,
                        symbol->paramTypes,
                        actualTypes
                    )
                );
            }
        }
        
        return symbol->returnType;
    }
    
    return DataType::IOTA;
}

// Check if all paths in block return
bool SemanticAnalyzer::checkPathsReturn(const std::vector<ASTNode*>& block) {
    for (auto item : block) {
        if (dynamic_cast<ReturnStmtNode*>(item)) {
            return true;
        }
        
        if (IfStmtNode* ifStmt = dynamic_cast<IfStmtNode*>(item)) {
            if (!ifStmt->elseItems.empty()) {
                bool thenReturns = checkPathsReturn(ifStmt->thenItems);
                bool elseReturns = checkPathsReturn(ifStmt->elseItems);
                if (thenReturns && elseReturns) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Check if statement is a terminator
bool SemanticAnalyzer::isTerminator(ASTNode* node) {
    return dynamic_cast<ReturnStmtNode*>(node) != nullptr;
}


void SemanticAnalyzer::visit(PrintStmtNode* node) {
    // Analyze the expression being printed
    if (node->expression) {
        analyzeExpr(node->expression);
    }
}

void SemanticAnalyzer::visit(IfStmtNode* node) {
    analyzeIf(node);
}

void SemanticAnalyzer::visit(WhileStmtNode* node) {
    analyzeWhile(node);
}

void SemanticAnalyzer::visit(AssignmentStmtNode* node) {
    analyzeAssignment(node);
}

void SemanticAnalyzer::visit(ReturnStmtNode* node) {
    analyzeReturn(node);
}

// Stub implementations for nodes that shouldn't be encountered or need forwarding
void SemanticAnalyzer::visit(ProgramNode* node) {
    analyzeProgram(node);
}

void SemanticAnalyzer::visit(FunctionDeclNode* node) {
    analyzeFunctionDecl(node);
}

void SemanticAnalyzer::visit(VarDeclNode* node) {
    analyzeVarDecl(node);
}

void SemanticAnalyzer::visit(BlockNode* node) {
    // Convert vector<CodeItemNode*> to vector<ASTNode*>
    std::vector<ASTNode*> items;
    items.reserve(node->items.size());
    for (auto item : node->items) {
        items.push_back(item);
    }
    analyzeBlock(items, false);
}

void SemanticAnalyzer::visit(IfNode* node) {
    (void)node;  // Mark as intentionally unused
    // This version shouldn't be used - parser creates IfStmtNode
    throw std::runtime_error("IfNode should not be encountered in semantic analysis");
}

void SemanticAnalyzer::visit(WhileNode* node) {
    (void)node;  // Mark as intentionally unused
    // This version shouldn't be used - parser creates WhileStmtNode
    throw std::runtime_error("WhileNode should not be encountered in semantic analysis");
}

void SemanticAnalyzer::visit(ReturnNode* node) {
    (void)node;  // Mark as intentionally unused
    // This version shouldn't be used - parser creates ReturnStmtNode
    throw std::runtime_error("ReturnNode should not be encountered in semantic analysis");
}

void SemanticAnalyzer::visit(PrintNode* node) {
    (void)node;  // Mark as intentionally unused
    // This version shouldn't be used - parser creates PrintStmtNode
    throw std::runtime_error("PrintNode should not be encountered in semantic analysis");
}

void SemanticAnalyzer::visit(AssignmentNode* node) {
    (void)node;  // Mark as intentionally unused
    // This version shouldn't be used - parser creates AssignmentStmtNode
    throw std::runtime_error("AssignmentNode should not be encountered in semantic analysis");
}

void SemanticAnalyzer::visit(ExprStmtNode* node) {
    analyzeExpr(node->expression);
}

void SemanticAnalyzer::visit(BinaryOpNode* node) {
    analyzeExpr(node);
}

void SemanticAnalyzer::visit(UnaryOpNode* node) {
    analyzeExpr(node);
}

void SemanticAnalyzer::visit(LiteralNode* node) {
    (void)node;  // Mark as intentionally unused
    // Already typed
}

void SemanticAnalyzer::visit(IdentifierNode* node) {
    (void)node;  // Mark as intentionally unused
    // Type is determined in analyzeExpr
}

void SemanticAnalyzer::visit(FunctionCallNode* node) {
    analyzeExpr(node);
}