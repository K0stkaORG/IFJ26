/**
 * @file optimizer_dead_code_elimination.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for dead code elimination
 *
 * Contains definitions for helper functions for eliminating dead code.
 */

#include "optimizer_dead_code_elimination.h"

#include "ir.h"
#include "ir_free_helpers.h"
#include "optimizer_helpers.h"

#include <stdbool.h>  // bool

bool optimizer_eliminate_dead_code(Ctx* ctx, IrFunctionId function_id) {
  IrFunction* function = &ctx->ir.functions.data[function_id];

  // Check that the function has values to eliminate
  if (function->value_count == 0)
    return false;

  // Count uses of each value
  optimizer_count_value_uses(function, ctx->optimizer.value_uses);

  bool changed = false, removed;

  do {
    removed = false;

    // Go through all instructions in all blocks
    for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
      IrBlock* block = &function->blocks.data[bid];
      for (size_t instId = 0; instId < block->instructions.count; instId++) {
        IrInstruction* instruction = &block->instructions.data[instId];

        // Check that the instruction is pure
        if (!optimizer_is_pure(instruction->type))
          continue;

        // Check that the value is not being used anywhere
        if (ctx->optimizer.value_uses[instruction->result] > 0)
          continue;

        // Remove the dead instruction
        ir_free_instruction(instruction);
        array_remove(ctx, &block->instructions, instId);
        removed = true;
        changed = true;
      }
    }
  } while (removed);

  return changed;
}
