/**
 * @file lowering_helpers.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 11. 11. 2026
 *
 * @brief Helper functions for lowering the AST to IR
 *
 * This file contains declarations of helper functions used for lowering the AST
 * to IR.
 */

#ifndef LOWERING_HELPERS_H
#define LOWERING_HELPERS_H

#include "ast.h"
#include "ir.h"

/**
 * @brief Convert an AST data type to an IR data type
 *
 * @param data_type The AST data type to convert
 * @return The corresponding IR data type
 */
IrType ast_to_ir_data_type(Ctx* ctx, AstDataType data_type);

/**
 * @brief Get the return type of a function
 *
 * If the return type is not specified, infers it from the function body.
 *
 * @param ctx The context
 * @param function The function node
 * @return The return type of the function
 */
IrType ast_get_function_return_type(Ctx* ctx, AstNode* function);

/**
 * @brief Get the identifier of a node
 *
 * @param node The node to get the identifier of
 * @return The identifier of the node
 */
const char* ast_get_node_identifier(Ctx* ctx, AstNode* node);

#endif
