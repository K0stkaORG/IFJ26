/**
 * @file ast.h
 * Project: IFJ26
 *
 * @author Lukáš Pollák (xpollal00)
 * @date 13. 10. 2026
 *
 * @brief Header file for the Abstract Syntax Tree (AST).
 */

#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>
#include "ctx.h"

typedef struct AstNode AstNode;
typedef struct Ast Ast;

typedef enum {
  AST_NODE_INVALID = -1,
  AST_NODE_NONE,

  AST_NODE_IMPORT,
  AST_NODE_DEF,
  AST_NODE_RETURN,
  AST_NODE_CASE,
  AST_NODE_MATCH,
  AST_NODE_IF,
  AST_NODE_ELSE,
  AST_NODE_THEN,
  AST_NODE_DO,
  AST_NODE_VAL,
  AST_NODE_VAR,
  AST_NODE_FOR,
  AST_NODE_WHILE,

  AST_NODE_TYPE_DOUBLE,
  AST_NODE_TYPE_INT,
  AST_NODE_TYPE_BOOLEAN,
  AST_NODE_TYPE_STRING,
  AST_NODE_TYPE_UNIT,

  AST_NODE_CLASS,
  AST_NODE_EXTENSION,
  AST_NODE_TYPE_LIST,
  AST_NODE_TYPE,
} AstNodeType;

typedef union {
  char char_value;
  int int_value;
  double float_value;
  bool boolean_value;
  const char* string_value;
} AstNodeValue;

// === AST_NODE===

/**
 * @brief Initializes AST node
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param node pointer to a uninitialized AST node variable
 */
void ast_node_create(Ctx* ctx, AstNode** node);

/**
 * @brief Frees AST node
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param node pointer to a AST node
 */
void ast_node_destroy(Ctx* ctx, AstNode* node);

/**
 * @brief Sets type for given node
 *
 * @pre Node pointer is not NULL!
 * @param node pointer to a AST node
 * @param type type to set for given node
 */
void ast_node_set_type(AstNode* node, AstNodeType type);

/**
 * @brief Sets value for given node
 *
 * @pre Node pointer is not NULL!
 * @param node pointer to a AST node
 * @param value value to set for given node
 */
void ast_node_set_value(AstNode* node, AstNodeValue value);

/**
 * @brief Returns node type
 *
 * @pre Node pointer is not NULL!
 * @param node pointer to a AST node
 * @return node type
 */
AstNodeType ast_node_get_type(const AstNode* node);

/**
 * @brief Returns node value
 *
 * @pre Node pointer is not NULL!
 * @param node pointer to a AST node
 * @return node value
 */
AstNodeValue ast_node_get_value(const AstNode* node);

/**
 * @brief Returns number of node children
 *
 * @pre Node pointer is not NULL!
 * @param node pointer to a AST node
 * @return number of node children
 */
size_t ast_node_get_child_count(const AstNode* node);

/**
 * @brief Returns child of given node
 *
 * @pre Node pointer is not NULL!
 * @param node pointer to a AST node
 * @param index index of child
 * @param child pointer to a variable to store child node
 */
void ast_node_get_child(Ctx* ctx, AstNode* node, size_t index, AstNode** child);

/**
 * @brief Returns parent of given node
 *
 * @pre Node pointer is not NULL!
 * @param node pointer to a AST node
 * @param parent pointer to a variable to store parent node
 */
void ast_node_get_parent(AstNode* node, AstNode** parent);

/**
 * @brief Appends child to given parent node
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param node pointer to a parent AST node
 * @param child pointer to a AST node to append
 */
void ast_node_append_child(Ctx* ctx, AstNode* node, AstNode* child);

/**
 * @brief Inserts child at given index in parent node
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param parent pointer to a parent AST node
 * @param index index to insert child at
 * @param child pointer to a AST node to insert
 */
void ast_node_insert_child(Ctx* ctx,
                           AstNode* node,
                           size_t index,
                           AstNode* child);

/**
 * @brief Detaches child at given index from parent node
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param parent pointer to a parent AST node
 * @param index index of child to detach
 * @param child pointer to a variable to store detached child node
 */
void ast_node_detach_child(Ctx* ctx, AstNode* node, size_t index, AstNode** child);

// === AST ===

/**
 * @brief Initializes AST
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param ast pointer to a uninitialized AST variable
 */
void ast_create(Ctx* ctx, Ast** ast);

/**
 * @brief Frees AST
 *
 * @param ast pointer to a AST
 */
void ast_destroy(Ast* ast);

/**
 * @brief Clears AST
 *
 * @param ast pointer to a AST
 */
void ast_clear(Ast* ast);

/**
 * @brief Returns AST root node
 *
 * @param ast pointer to a AST
 * @param root pointer to a variable to store root node
 */
void ast_get_root(Ast* ast, AstNode** root);

/**
 * @brief Sets root node for given AST
 *
 * @param ast pointer to a AST
 * @param root pointer to a AST node to set as root
 */
void ast_set_root(Ast* ast, AstNode* root);

/**
 * @brief Detaches root node from given AST
 *
 * @param ast pointer to a AST
 * @param root pointer to a variable to store detached root node
 */
void ast_detach_root(Ast* ast, AstNode** root);

/**
 * @brief Removes node from given AST
 *
 * @param ast pointer to a AST
 * @param node pointer to a AST node to remove
 */
void ast_remove_node(Ast* ast, AstNode* node);

/**
 * @brief Replaces node in given AST
 *
 * @param ast pointer to a AST
 * @param old_node pointer to a AST node to replace
 * @param new_node pointer to a replacement AST node
 */
void ast_replace_node(Ast* ast, AstNode* old_node, AstNode* new_node);

#endif
