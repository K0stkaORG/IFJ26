/**
 * @file optimizer_constant_folding.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Helpers for folding constants in the IR
 *
 * This file contains definitions for helper functions for folding constants in
 * the IR.
 *
 * @note Folding is done with 64 bit integers as that is what the interpreter
 * uses internally even though IFJ26 and IFJ26Code's integers are supposed to be
 * 32 bit
 */

#include "optimizer_constant_folding.h"

#include "ctx.h"
#include "ir.h"
#include "ir_free_helpers.h"
#include "optimizer_constant_propagation.h"
#include "optimizer_fold_branches.h"
#include "optimizer_fold_internal.h"

bool optimizer_fold_function(Ctx* ctx, IrFunctionId function_id) {
  IrFunction* function = &ctx->ir.functions.data[function_id];

  bool modified = false;

  // Iterate over all blocks in the function
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    IrBlock* block = &function->blocks.data[bid];

    // Iterate over all instructions in the block
    for (size_t inst_id = 0; inst_id < block->instructions.count; inst_id++) {
      IrInstruction* instruction = &block->instructions.data[inst_id];

      IrOperand result;

      // Try to fold the instruction
      if (!optimizer_fold_instruction(ctx, function, instruction, &result))
        continue;  // Unable to fold, go to next instruction

      // Replace the value with the constant
      size_t replacements = optimizer_replace_value(
          ctx, function_id, instruction->result, result, true);

      // If the result is a string and it's not used anywhere, free it
      if (result.type == IR_OP_STRING && replacements == 0)
        free(result.value.string_value);

      // The instruction was folded, remove it as it's no longer needed
      ir_free_instruction(instruction);
      array_remove(ctx, &block->instructions, inst_id);

      // Decrement the instruction index to account for the removed instruction
      inst_id--;

      // Mark the function as modified
      modified = true;
    }

    // Attempt to fold the block terminator
    modified |= optimizer_fold_branch(&block->terminator);
    modified |=
        optimizer_simplify_negated_branch(ctx, function, &block->terminator);
  }

  // Return whether the function was modified
  return modified;
}
