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
 * Reads the final return type from the semantic function symbol.
 * Semantic analysis must have completed before calling this helper.
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

/**
 * @brief Create an IR operand for a value
 *
 * @param value The value
 * @return The IR operand
 */
IrOperand value_operand(IrValueId value);

/**
 * @brief Create an IR operand for an integer value
 *
 * @param value The integer value
 * @return The IR operand
 */
IrOperand int_operand(IrInt value);

/**
 * @brief Create an IR operand for a double value
 *
 * @param value The double value
 * @return The IR operand
 */
IrOperand double_operand(IrDouble value);

/**
 * @brief Create an IR operand for a string value
 *
 * @param value Borrowed string; IR construction helpers copy it when emitted
 * @return The IR operand
 */
IrOperand string_operand(IrString value);

/**
 * @brief Create an IR operand for a boolean value
 *
 * @param value The boolean value
 * @return The IR operand
 */
IrOperand bool_operand(IrBool value);

/**
 * @brief Create an IR operand for unit
 *
 * @note This could be a constant value but for consistency with other types, it
 * is a function. Optimizer will inline it anyway.
 *
 * @return The IR operand
 */
IrOperand unit_operand(void);

/**
 * @brief Add a unary instruction to the IR
 *
 * @param ctx The context of the program
 * @param function_id Id of the function to add the instruction to
 * @param block_id Id of the block to add the instruction to
 * @param instruction_type The instruction type
 * @param result_type The type of the result
 * @param operand The operand
 * @return The result of the unary operation (as a IR_OP_VALUE)
 */
IrOperand add_unary(Ctx* ctx,
                    IrFunctionId function_id,
                    IrBlockId block_id,
                    IrInstructionType instruction_type,
                    IrType result_type,
                    IrOperand operand);

/**
 * @brief Add a binary instruction to the IR
 *
 * @param ctx The context of the program
 * @param function_id Id of the function to add the instruction to
 * @param block_id Id of the block to add the instruction to
 * @param instruction_type The instruction type
 * @param result_type The type of the result
 * @param left The left operand
 * @param right The right operand
 * @return The result of the binary operation (as a IR_OP_VALUE)
 */
IrOperand add_binary(Ctx* ctx,
                     IrFunctionId function_id,
                     IrBlockId block_id,
                     IrInstructionType instruction_type,
                     IrType result_type,
                     IrOperand left,
                     IrOperand right);

/**
 * @brief Set block's terminator to return
 *
 * @param ctx The context of the program
 * @param function_id Id of the function to set the terminator for
 * @param block_id Id of the block to set the terminator for
 * @param value The value to return
 */
void emit_return(Ctx* ctx,
                 IrFunctionId function_id,
                 IrBlockId block_id,
                 IrOperand value);

#endif
