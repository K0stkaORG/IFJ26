/**
 * @file lowering_helpers.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 11. 11. 2026
 *
 * @brief Helper functions for lowering the AST to IR
 *
 * This file contains implementation of helper functions used for lowering the
 * AST to IR.
 */

#include "lowering_helpers.h"

#include "ast.h"
#include "ir.h"

IrType ast_to_ir_data_type(Ctx* ctx, AstDataType data_type) {
  switch (data_type) {
    case AST_DATA_TYPE_INT:
      return IR_INT;
    case AST_DATA_TYPE_DOUBLE:
      return IR_DOUBLE;
    case AST_DATA_TYPE_BOOLEAN:
      return IR_BOOL;
    case AST_DATA_TYPE_STRING:
      return IR_STRING;
    case AST_DATA_TYPE_UNIT:
      return IR_UNIT;

    default:
      THROW(E_INTERNAL_ERROR, "Unknown AST data type");
      return IR_UNIT;  // Unreachable
  }
}

IrType ast_get_function_return_type(Ctx* ctx, AstNode* function) {
  // Get the return type child node
  AstNode* return_type_node;
  ast_node_get_child(ctx, function, 1, &return_type_node);

  // Get the return type
  AstDataType return_type = ast_node_get_data_type(return_type_node);

  // Convert the return type to IR
  return ast_to_ir_data_type(ctx, return_type);
}

const char* ast_get_node_identifier(Ctx* ctx, AstNode* node) {
  // Check that node is not NULL
  if (node == NULL)
    THROW(E_INTERNAL_ERROR, "Attempted to get identifier of NULL node");

  switch (ast_node_get_type(node)) {
    // We have reached a valid identifier node, return its name
    case AST_NODE_IDENT:
      return ast_node_get_name(node);

    // For functions, parameters, their name is the first child
    case AST_NODE_DEF: {
      AstNode* ident_child;
      ast_node_get_child(ctx, node, 0, &ident_child);

      return ast_get_node_identifier(ctx, ident_child);
    }

    default:
      THROW(E_INTERNAL_ERROR, "Failed to get identifier of AST node");
      return NULL;  // Unreachable
  }
}
