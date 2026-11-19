/**
 * @file optimizer_forwarding.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for store-load forwarding
 *
 * Contains definitions for helper functions for store-load forwarding.
 */

#include "optimizer_forwarding.h"

#include "ir.h"
#include "ir_free_helpers.h"
#include "optimizer_constant_propagation.h"

#include <stdbool.h>  // bool

bool optimizer_forward_stores(Ctx* ctx, IrFunctionId function_id) {
  IrFunction* function = &ctx->ir.functions.data[function_id];

  // Check that the function even has any stack slots
  if (function->slots.count == 0)
    return false;

  bool changed = false;

  // Go through each block in the function
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    IrBlock* block = &function->blocks.data[bid];

    // Reset the last store operand for each slot
    for (IrStackSlotId slot_i = 0; slot_i < function->slots.count; slot_i++)
      function->slots.data[slot_i].lastStore = NULL;

    // Go through each instruction in the block
    for (size_t inst_id = 0; inst_id < block->instructions.count; inst_id++) {
      IrInstruction* instruction = &block->instructions.data[inst_id];

      switch (instruction->type) {
        case IR_INST_LOAD: {
          IrStackSlotId slot_id = instruction->operands.load.source;

          // Check if we know the value of this slot
          if (function->slots.data[slot_id].lastStore == NULL)
            break;

          // Replace uses of the load result with the known value
          optimizer_replace_value(ctx, function_id, instruction->result,
                                  *function->slots.data[slot_id].lastStore,
                                  false);

          // Remove the load instruction
          ir_free_instruction(instruction);
          array_remove(ctx, &block->instructions, inst_id);

          // Move back one instruction to account for the removed load
          inst_id--;

          // Mark that the function has changed
          changed = true;

          break;
        }

        case IR_INST_STORE:
          function->slots.data[instruction->operands.store.target].lastStore =
              &instruction->operands.store.source;
          break;

        default:
          break;
      }
    }
  }

  return changed;
}
