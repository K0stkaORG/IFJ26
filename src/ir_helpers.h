/**
 * @file ir_helpers.h
 * Project: IFJ26
 *
 * @brief Intermediate representation construction helpers
 */

#ifndef IR_HELPERS_H
#define IR_HELPERS_H

#include "ctx.h"  // Ctx
#include "ir.h"

/**
 * @brief Adds a function to the IR
 *
 * Copies the function name and initializes the function to default values.
 * The function and program entrypoints are not set automatically.
 *
 * @param[in,out] ctx The shared context containing the IR
 * @param[in] name Name of the function to copy
 * @param[in] return_type Return type of the function
 * @return Id of the added function
 *
 * @warning Adding a function may invalidate pointers into the function array.
 * Invalid arguments or allocation failure throw E_INTERNAL_ERROR.
 */
IrFunctionId ir_add_function(Ctx* ctx, const char* name, IrType return_type);

/**
 * @brief Adds a parameter to a function in the IR
 *
 * Copies the parameter name and adds it to the function's stack slots.
 * Parameters must be added before any local variables.
 *
 * @param[in,out] ctx The shared context containing the IR
 * @param[in] function Id of the function to add the parameter to
 * @param[in] name Name of the parameter to copy
 * @param[in] type Type of the parameter
 * @return Id of the added parameter's stack slot
 *
 * @warning Adding a parameter may invalidate pointers into the slot array.
 * Invalid arguments or allocation failure throw E_INTERNAL_ERROR.
 */
IrStackSlotId ir_add_parameter(Ctx* ctx,
                               IrFunctionId function,
                               const char* name,
                               IrType type);

/**
 * @brief Adds a local variable to a function in the IR
 *
 * Copies the variable name and adds a stack slot after the function parameters.
 *
 * @param[in,out] ctx The shared context containing the IR
 * @param[in] function Id of the function to add the slot to
 * @param[in] name Name of the variable to copy
 * @param[in] type Type of the variable
 * @return Id of the added stack slot
 *
 * @warning Adding a slot may invalidate pointers into the slot array.
 * Invalid arguments or allocation failure throw E_INTERNAL_ERROR.
 */
IrStackSlotId ir_add_slot(Ctx* ctx,
                          IrFunctionId function,
                          const char* name,
                          IrType type);

/**
 * @brief Adds an empty block to a function in the IR
 *
 * Initializes the block with no instructions and a jump to IR_INVALID_ID.
 * The terminator must be set before the block is used. Does not change the
 * function entrypoint.
 *
 * @param[in,out] ctx The shared context containing the IR
 * @param[in] function Id of the function to add the block to
 * @return Id of the added block
 *
 * @warning Adding a block may invalidate pointers into the block array.
 * Invalid arguments or allocation failure throw E_INTERNAL_ERROR.
 */
IrBlockId ir_add_block(Ctx* ctx, IrFunctionId function);

/**
 * @brief Adds an instruction to a block in the IR
 *
 * Copies the instruction, including string operands and call arguments.
 * The caller keeps ownership of the original operands and argument array.
 *
 * Assigns the next value id in the function, ignoring the supplied result id.
 * Store instructions and instructions returning IR_UNIT do not have a result.
 * Does not check operand types or references to other IR objects.
 *
 * @param[in,out] ctx The shared context containing the IR
 * @param[in] function Id of the function containing the block
 * @param[in] block Id of the block to add the instruction to
 * @param[in] instruction The instruction to copy
 * @return Id of the result, or IR_INVALID_ID if there is no result
 *
 * @warning Adding an instruction may invalidate pointers into the instruction
 * array. Do not combine automatic result ids with manually assigned ids.
 * Invalid arguments or allocation failure throw E_INTERNAL_ERROR. A partially
 * added instruction is safe to free using ir_free or ctx_free.
 */
IrValueId ir_add_instruction(Ctx* ctx,
                             IrFunctionId function,
                             IrBlockId block,
                             IrInstruction instruction);

/**
 * @brief Sets the terminator of a block in the IR
 *
 * Copies the terminator and its string operand, then frees the old terminator's
 * operand. The caller keeps ownership of the original operand. Does not check
 * operand types or target block ids.
 *
 * @param[in,out] ctx The shared context containing the IR
 * @param[in] function Id of the function containing the block
 * @param[in] block Id of the block to set the terminator of
 * @param[in] terminator The terminator to copy
 *
 * @warning Invalid arguments or allocation failure throw E_INTERNAL_ERROR.
 * The previous terminator stays unchanged if copying fails.
 */
void ir_set_terminator(Ctx* ctx,
                       IrFunctionId function,
                       IrBlockId block,
                       IrBlockTerminator terminator);

#endif
