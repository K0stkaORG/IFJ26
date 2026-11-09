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

typedef enum {
  AST_NODE_INVALID = -1,
  AST_NODE_NONE,

  AST_NODE_IMPORT,
  AST_NODE_DEF,
  AST_NODE_RETURN,
  AST_NODE_CASE,
  AST_NODE_MATCH,
  AST_NODE_IF,
  AST_NODE_VAL,
  AST_NODE_VAR,
  AST_NODE_FOR,
  AST_NODE_WHILE,

  AST_NODE_TYPE_ANNOTATION,

  AST_NODE_CLASS,
  AST_NODE_EXTENSION,
  AST_NODE_TYPE_LIST,

  // Literals
  AST_NODE_IDENT,
  AST_NODE_INT_LIT,
  AST_NODE_DOUBLE_LIT,
  AST_NODE_STRING_LIT,
  AST_NODE_BOOL_LIT,
  AST_NODE_UNIT_LIT,

  // Binary operators, children are [left, right]
  AST_NODE_ADD,
  AST_NODE_SUB,
  AST_NODE_MUL,
  AST_NODE_DIV,
  AST_NODE_EQ,
  AST_NODE_NEQ,
  AST_NODE_LT,
  AST_NODE_GT,
  AST_NODE_LE,
  AST_NODE_GE,
  AST_NODE_AND,
  AST_NODE_OR,
  AST_NODE_ASSIGN,

  // Unary operator, children are [operand]
  AST_NODE_NOT,

  // N-ary nodes
  AST_NODE_CALL,   // children are [callee, arg...]
  AST_NODE_BLOCK,  // children are the statements in order
} AstNodeType;

/**
 * @brief Semantic data type of an expression, filled in by type checking.
 *
 * Not to be confused with AST_NODE_TYPE_ANNOTATION, which is a type written in
 * the source code (its data_type holds the annotated type).
 */
typedef enum {
  AST_DATA_TYPE_UNKNOWN,
  AST_DATA_TYPE_INT,
  AST_DATA_TYPE_DOUBLE,
  AST_DATA_TYPE_BOOLEAN,
  AST_DATA_TYPE_STRING,
  AST_DATA_TYPE_UNIT,
} AstDataType;

typedef struct SymtableData SymtableData;

/**
 * @brief Payload of a node, the valid member is given by the node type.
 *
 * IDENT -> symbol, INT_LIT -> int_value, DOUBLE_LIT -> float_value,
 * STRING_LIT -> string_value, BOOL_LIT -> boolean_value. Other kinds carry no
 * value.
 */
typedef union {
  const SymtableData* symbol;
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
 * @brief Frees an AST node and its descendants, detaching it from its parent
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
void ast_node_detach_child(Ctx* ctx,
                           AstNode* node,
                           size_t index,
                           AstNode** child);

// === AST NODE ATTRIBUTES ===

/**
 * @brief Tells whether nodes of given type carry a value
 *
 * @param type node type
 * @return true for IDENT and literal kinds, false otherwise
 */
bool ast_node_type_has_value(AstNodeType type);

/**
 * @brief Sets semantic data type (result type of an expression) of given node
 *
 * @pre Node pointer is not NULL!
 */
void ast_node_set_data_type(AstNode* node, AstDataType data_type);

/**
 * @brief Returns semantic data type of given node
 *
 * @pre Node pointer is not NULL!
 * @return AST_DATA_TYPE_UNKNOWN until type checking sets it
 */
AstDataType ast_node_get_data_type(const AstNode* node);

/**
 * @brief Sets source position the node originates from
 *
 * @pre Node pointer is not NULL!
 * @param line line number from the token
 * @param column column number from the token
 */
void ast_node_set_position(AstNode* node, int line, int column);

/**
 * @brief Returns source line of given node (0 if unset)
 *
 * @pre Node pointer is not NULL!
 */
int ast_node_get_line(const AstNode* node);

/**
 * @brief Returns source column of given node (0 if unset)
 *
 * @pre Node pointer is not NULL!
 */
int ast_node_get_column(const AstNode* node);

// === AST CONSTRUCTION HELPERS (MakeLeaf / MakeTree) ===

/**
 * @brief Creates a leaf node with a value
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param type node type, must satisfy ast_node_type_has_value
 * @param value value of the leaf
 * @param node pointer to a variable to store the new node
 */
void ast_make_leaf(Ctx* ctx,
                   AstNodeType type,
                   AstNodeValue value,
                   AstNode** node);

/**
 * @brief Creates a node with one child
 *
 * Takes ownership of operand on success. On a thrown error the operand stays
 * owned by the caller.
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param type node type
 * @param operand parentless child node
 * @param node pointer to a variable to store the new node
 */
void ast_make_unary(Ctx* ctx,
                    AstNodeType type,
                    AstNode* operand,
                    AstNode** node);

/**
 * @brief Creates a node with two children, in the order [left, right]
 *
 * Takes ownership of both children on success. On a thrown error the children
 * stay owned by the caller.
 *
 * @param ctx pointer to an initialized shared allocation and error context
 * @param type node type
 * @param left parentless left child node
 * @param right parentless right child node
 * @param node pointer to a variable to store the new node
 */
void ast_make_binary(Ctx* ctx,
                   AstNodeType type,
                   AstNode* left,
                   AstNode* right,
                   AstNode** node);

#endif
