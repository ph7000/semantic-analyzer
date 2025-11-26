%code requires {
    // Forward declarations for all AST node types used in union
    class ASTNode;
    class CodeItemNode;
    class DeclNode;
    class ProgramNode;
    class ExprNode;
    class StmtNode;
    class TypeNode;
    class ParamNode;
    class FunctionDeclNode;
    class VarDeclNode;
    
    // Now include the full definitions
    #include "astnode.hpp"
} 
%{
#include <string>
#include <stdio.h>
#include "astnode.hpp"
#include "exception.hpp"

void yyerror(ASTNode** root, const char* s);
extern int yylex();
extern int yylineno;
extern char* yytext;

int error_count = 0;
int yydebug = 0;
static int last_token_line = 1;
static int last_token_column = 1;
%}

%parse-param { ASTNode** root }
%locations

%union {
    ProgramNode* program;
    DeclNode* decl;
    StmtNode* stmt;
    ExprNode* expr;
    TypeNode* type;
    ParamNode* param;
    FunctionDeclNode* func_decl;
    VarDeclNode* var_decl;
    std::vector<DeclNode*>* decl_list;
    std::vector<ASTNode*>* block;
    std::vector<ParamNode*>* param_list;
    std::vector<ExprNode*>* expr_list;
    std::string* text;
    int intval;
    double floatval;
    bool boolval;
}

%token <floatval> FLOAT_LITERAL
%token <intval> INTEGER_LITERAL 
%token <boolval> BOOL_LITERAL
%token <text> IDENTIFIER
%token FUNC_KEYWORD VAR_KEYWORD LET_KEYWORD IF_KEYWORD ELSE_KEYWORD WHILE_KEYWORD PRINT_KEYWORD RETURN_KEYWORD INT_KEYWORD FLOAT_KEYWORD BOOL_KEYWORD
%token ASSIGN_OP EQUAL_OP NEQ_OP LT_OP GT_OP LEQ_OP GEQ_OP PLUS_OP MINUS_OP MULTIPLY_OP DIVIDE_OP
%token LPAREN_DELIMITER RPAREN_DELIMITER LBRACE_DELIMITER RBRACE_DELIMITER SEMI_DELIMITER COMMA_DELIMITER COLON_DELIMITER END_OF_FILE

%type <program> program
%type <decl> decl
%type <stmt> stmt
%type <expr> expr primary_expr unary_expr multiplicative_expr additive_expr comparison_expr equality_expr
%type <type> type
%type <param> param
%type <func_decl> func_decl
%type <var_decl> var_decl
%type <decl_list> decl_list
%type <block> block block_items
%type <param_list> param_list param_list_nonempty
%type <expr_list> arg_list arg_list_nonempty

%start program
%%

program : decl_list END_OF_FILE { ProgramNode* ast = new ProgramNode(); for (auto& decl : *$1) ast->addDecl(decl); *root = ast; delete $1; };

decl_list : decl_list decl { $$ = $1; if ($2 != nullptr) $$->push_back($2); }
          | decl { $$ = new std::vector<DeclNode*>; if ($1 != nullptr) $$->push_back($1); };

decl : func_decl { $$ = $1; } | var_decl { $$ = $1; };

func_decl : FUNC_KEYWORD IDENTIFIER LPAREN_DELIMITER param_list RPAREN_DELIMITER COLON_DELIMITER type LBRACE_DELIMITER block RBRACE_DELIMITER {
    FunctionDeclNode* func = new FunctionDeclNode(*$2, $7);
    for (auto param : *$4) func->addParameter(param);
    for (auto item : *$9) func->addBodyItem(item);
    $$ = func;
    delete $2; delete $4; delete $9;
};

param_list : param_list_nonempty { $$ = $1; } | %empty { $$ = new std::vector<ParamNode*>; };

param_list_nonempty : param_list_nonempty COMMA_DELIMITER param { $$ = $1; $$->push_back($3); }
                    | param { $$ = new std::vector<ParamNode*>; $$->push_back($1); };

param : IDENTIFIER COLON_DELIMITER type { $$ = new ParamNode(*$1, $3); delete $1; };

var_decl : VAR_KEYWORD IDENTIFIER COLON_DELIMITER type ASSIGN_OP expr SEMI_DELIMITER {
    $$ = new VarDeclNode(false, *$2, $4, $6); delete $2;
}
| LET_KEYWORD IDENTIFIER COLON_DELIMITER type ASSIGN_OP expr SEMI_DELIMITER {
    $$ = new VarDeclNode(true, *$2, $4, $6); delete $2;
};

type : INT_KEYWORD { $$ = new TypeNode("int"); }
     | FLOAT_KEYWORD { $$ = new TypeNode("float"); }
     | BOOL_KEYWORD { $$ = new TypeNode("bool"); };

block : block_items { $$ = $1; } | %empty { $$ = new std::vector<ASTNode*>; };

block_items : block_items var_decl { $$ = $1; if ($2 != nullptr) $$->push_back($2); }
            | block_items func_decl { $$ = $1; if ($2 != nullptr) $$->push_back($2); }
            | block_items stmt { $$ = $1; if ($2 != nullptr) $$->push_back($2); }
            | var_decl { $$ = new std::vector<ASTNode*>; if ($1 != nullptr) $$->push_back($1); }
            | func_decl { $$ = new std::vector<ASTNode*>; if ($1 != nullptr) $$->push_back($1); }
            | stmt { $$ = new std::vector<ASTNode*>; if ($1 != nullptr) $$->push_back($1); };

stmt : PRINT_KEYWORD LPAREN_DELIMITER expr RPAREN_DELIMITER SEMI_DELIMITER { $$ = new PrintStmtNode($3); }
     | IF_KEYWORD LPAREN_DELIMITER expr RPAREN_DELIMITER LBRACE_DELIMITER block RBRACE_DELIMITER {
         IfStmtNode* ifStmt = new IfStmtNode($3);
         for (auto item : *$6) ifStmt->addThenItem(item);
         $$ = ifStmt; delete $6;
     }
     | IF_KEYWORD LPAREN_DELIMITER expr RPAREN_DELIMITER LBRACE_DELIMITER block RBRACE_DELIMITER ELSE_KEYWORD LBRACE_DELIMITER block RBRACE_DELIMITER {
         IfStmtNode* ifStmt = new IfStmtNode($3);
         for (auto item : *$6) ifStmt->addThenItem(item);
         for (auto item : *$10) ifStmt->addElseItem(item);
         $$ = ifStmt; delete $6; delete $10;
     }
     | WHILE_KEYWORD LPAREN_DELIMITER expr RPAREN_DELIMITER LBRACE_DELIMITER block RBRACE_DELIMITER {
         WhileStmtNode* whileStmt = new WhileStmtNode($3);
         for (auto item : *$6) whileStmt->addBodyItem(item);
         $$ = whileStmt; delete $6;
     }
     | IDENTIFIER ASSIGN_OP expr SEMI_DELIMITER { $$ = new AssignmentStmtNode(*$1, $3); delete $1; }
     | RETURN_KEYWORD expr SEMI_DELIMITER { $$ = new ReturnStmtNode($2); };

expr : equality_expr;

equality_expr : comparison_expr
              | equality_expr EQUAL_OP comparison_expr { $$ = new BinaryOpNode($1, "==", $3); }
              | equality_expr NEQ_OP comparison_expr { $$ = new BinaryOpNode($1, "!=", $3); };

comparison_expr : additive_expr
                | comparison_expr LT_OP additive_expr { $$ = new BinaryOpNode($1, "<", $3); }
                | comparison_expr GT_OP additive_expr { $$ = new BinaryOpNode($1, ">", $3); }
                | comparison_expr LEQ_OP additive_expr { $$ = new BinaryOpNode($1, "<=", $3); }
                | comparison_expr GEQ_OP additive_expr { $$ = new BinaryOpNode($1, ">=", $3); };

additive_expr : multiplicative_expr
              | additive_expr PLUS_OP multiplicative_expr { $$ = new BinaryOpNode($1, "+", $3); }
              | additive_expr MINUS_OP multiplicative_expr { $$ = new BinaryOpNode($1, "-", $3); };

multiplicative_expr : unary_expr
                    | multiplicative_expr MULTIPLY_OP unary_expr { $$ = new BinaryOpNode($1, "*", $3); }
                    | multiplicative_expr DIVIDE_OP unary_expr { $$ = new BinaryOpNode($1, "/", $3); };

unary_expr : primary_expr | MINUS_OP unary_expr { $$ = new UnaryOpNode("-", $2); };

primary_expr : INTEGER_LITERAL { last_token_line = yylloc.last_line; last_token_column = yylloc.last_column; $$ = new IntegerNode($1); }
             | FLOAT_LITERAL { last_token_line = yylloc.last_line; last_token_column = yylloc.last_column; $$ = new FloatNode($1); }
             | BOOL_LITERAL { last_token_line = yylloc.last_line; last_token_column = yylloc.last_column; $$ = new BoolNode($1); }
             | IDENTIFIER { last_token_line = yylloc.last_line; last_token_column = yylloc.last_column; $$ = new IdentifierNode(*$1); delete $1; }
             | IDENTIFIER LPAREN_DELIMITER arg_list RPAREN_DELIMITER {
                 last_token_line = yylloc.last_line; last_token_column = yylloc.last_column;
                 FunctionCallNode* call = new FunctionCallNode(*$1);
                 for (auto arg : *$3) call->addArgument(arg);
                 $$ = call; delete $1; delete $3;
             }
             | LPAREN_DELIMITER expr RPAREN_DELIMITER { last_token_line = yylloc.last_line; last_token_column = yylloc.last_column; $$ = $2; };

arg_list : arg_list_nonempty { $$ = $1; } | %empty { $$ = new std::vector<ExprNode*>; };

arg_list_nonempty : arg_list_nonempty COMMA_DELIMITER expr { $$ = $1; $$->push_back($3); }
                  | expr { $$ = new std::vector<ExprNode*>; $$->push_back($1); };

%%

void yyerror(ASTNode** root, const char* s) {
    (void)root; (void)s;
    error_count++;
    bool at_eof = (yytext == NULL || yytext[0] == '\0');
    int error_line = yylloc.first_line;
    int error_column = yylloc.first_column;
    if (at_eof && last_token_line > 0) {
        error_line = last_token_line;
        error_column = last_token_column + 1;
    }
    fprintf(stderr, "Parser error at line %d, column %d\n", error_line, error_column);
}
