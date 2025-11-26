I built a semantic analyzer!

This repo makes it easy to use the makefile to compile and analyze code for errors.

The semantic analyzer uses a Visitor Pattern combined with hierarchical symbol tables and multi-pass analysis. 
The Visitor pattern cleanly separates AST traversal from semantic operations, with each node type having a corresponding `visit()` method in the `SemanticAnalyzer` class. 
Symbol tables are organized in a scope chain using parent pointers, enabling nested scope lookup while preventing redeclarations within the same scope. 
The analysis occurs in two passes: first, all function signatures are registered in the global scope to enable forward references; second, function bodies and variable declarations are analyzed with full type checking and control flow analysis. 
Type compatibility is handled by an `isAssignmentCompatible()` function that implements spec-defined widening conversions (INT→FLOAT) and tolerated conversions (INT↔BOOL), enforcing strict incompatibility for FLOAT→INT/BOOL.


The semantic analysis process begins with initialization, creating a global scope and setting analysis state variables (currentFunction, hasReturn, isUnreachable). 
Phase 1 (first pass) iterates through all declarations, registering function signatures in the symbol table and checking for duplicate function names or conflicts with existing identifiers. 
Phase 2 (second pass) analyzes each declaration: for functions, it creates a new scope, adds parameters, recursively analyzes the function body, and verifies all execution paths return a value; for variables, it checks for name conflicts, analyzes initializer expressions, verifies type compatibility, and adds the variable to the current scope. 
Expression analysis works bottom-up, computing types for literals, performing scope-chain lookup for identifiers, applying type promotion rules for binary operators, and checking function call signatures. 
