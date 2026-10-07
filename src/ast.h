#ifndef AST_H
#define AST_H

#include <stddef.h>

typedef struct AstNode AstNode;
typedef struct Ast Ast;

typedef enum {
  AST_NODE_INVALID = -1,
  CHAR,
  INT,
  FLOAT,
  STRING,
} AstNodeType;

typedef enum {
  AST_OK = 0,
  AST_ERROR_ARGUMENT,
  AST_ERROR_ALLOCATION,
  AST_ERROR_TYPE,
} AstStatus;

AstNode* ast_node_create(void);
void ast_node_destroy(AstNode* node);
AstStatus ast_node_set_type(AstNode* node, AstNodeType type);
AstNodeType ast_node_get_type(const AstNode* node);

AstStatus ast_node_set_char(AstNode* node, char value);
AstStatus ast_node_get_char(const AstNode* node, char* value);
AstStatus ast_node_set_int(AstNode* node, int value);
AstStatus ast_node_get_int(const AstNode* node, int* value);
AstStatus ast_node_set_float(AstNode* node, double value);
AstStatus ast_node_get_float(const AstNode* node, double* value);
AstStatus ast_node_set_string(AstNode* node, const char* value);
AstStatus ast_node_get_string(const AstNode* node, const char** value);

size_t ast_node_child_count(const AstNode* node);
AstNode* ast_node_get_child(AstNode* node, size_t index);
AstNode* ast_node_get_parent(AstNode* node);
AstStatus ast_node_append_child(AstNode* parent, AstNode* child);
AstStatus ast_node_insert_child(AstNode* parent, size_t index, AstNode* child);
AstStatus ast_node_detach_child(AstNode* parent, size_t index, AstNode** child);

Ast* ast_create(void);
void ast_destroy(Ast* ast);
void ast_clear(Ast* ast);
AstNode* ast_get_root(Ast* ast);
AstStatus ast_set_root(Ast* ast, AstNode* root);
AstStatus ast_detach_root(Ast* ast, AstNode** root);
AstStatus ast_remove_node(Ast* ast, AstNode* node);
AstStatus ast_replace_node(Ast* ast, AstNode* old_node, AstNode* new_node);

#endif
