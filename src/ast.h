#ifndef AST_H
#define AST_H

typedef struct AstNode AstNode;
typedef struct Ast Ast;

AstNode* ast_node_create(void);
Ast* ast_create(void);

int ast_node_destroy(AstNode* AstNode);
int ast_destroy(Ast* ast);

int ast_add_node(Ast* ast, AstNode* AstNode);
int ast_remove_node(Ast* ast, AstNode* AstNode);
int ast_set_node(Ast* ast, int index, AstNode* AstNode);
AstNode* ast_get_node(Ast* ast, int value);
int ast_replace_node(Ast* ast, AstNode* old_AstNode, AstNode* new_AstNode);

#endif
