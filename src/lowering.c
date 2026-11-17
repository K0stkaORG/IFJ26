/**
 * @file lowering.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 9. 10. 2026
 *
 * @brief Functions for lowering the AST to IR
 *
 * This file contains implementation of lowering the AST to IR.
 */

#include "lowering.h"

#include "ast.h"
#include "ctx.h"
#include "ir.h"
#include "ir_helpers.h"
#include "lowering_helpers.h"

#include <string.h>  // strcmp

/**
 * @brief Lower a function signature to IR
 *
 * Lowers the fact a function exists, including it's name, parameters, and
 * return type to the IR. Does not lower the function body.
 *
 * @param ctx The context
 * @param function The function node to lower
 */
static void lower_function_signature(Ctx* ctx, AstNode* function) {
  // Add the function to the IR
  IrFunctionId function_id =
      ir_add_function(ctx, ast_get_node_identifier(ctx, function),
                      ast_get_function_return_type(ctx, function));

  // Check if this function was the main function
  if (ast_node_get_flags(function) & AST_FLAG_MAIN)
    // If so, set the entrypoint to this function
    ctx->ir.entrypoint = function_id;

  // Get the function parameters
  AstNode* params;
  ast_node_get_child(ctx, function, AST_DEF_PARAM_LIST, &params);

  // Add the function parameters to the IR
  size_t param_count = ast_node_get_child_count(params);
  for (size_t i = 0; i < param_count; i++) {
    AstNode* param;
    ast_node_get_child(ctx, params, i, &param);

    AstNode* param_type;
    ast_node_get_child(ctx, param, AST_PARAM_TYPE, &param_type);

    ir_add_parameter(
        ctx, function_id, ast_get_node_identifier(ctx, param),
        ast_to_ir_data_type(ctx, ast_node_get_data_type(param_type)));
  }
}

/**
 * @brief Lower a function body to IR
 *
 * Lowers the function body to the IR.
 *
 * @param ctx The context
 * @param function The function node to lower
 */
static void lower_function_body(Ctx* ctx, AstNode* function) {}

void lower_ast(Ctx* ctx) {
  // Lower each function signature to IR
  size_t function_count = ast_node_get_child_count(ctx->ast);
  for (size_t i = 0; i < function_count; i++) {
    AstNode* child;
    ast_node_get_child(ctx, ctx->ast, i, &child);

    lower_function_signature(ctx, child);
  }

  // Lower each function body to IR
  for (size_t i = 0; i < function_count; i++) {
    AstNode* child;
    ast_node_get_child(ctx, ctx->ast, i, &child);

    lower_function_body(ctx, child);
  }
}
