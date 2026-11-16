/**
 * @file optimizer_helpers.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for optimizing the IR
 *
 * Contains definitions for helper functions for optimizing the IR.
 */

#include "optimizer_helpers.h"

#include "ir.h"

IrInstruction* optimizer_find_definition(IrFunction* function,
                                         IrValueId value) {
  // Go through each block and instruction
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    IrBlock* block = &function->blocks.data[bid];

    for (size_t instId = 0; instId < block->instructions.count; instId++) {
      IrInstruction* instruction = &block->instructions.data[instId];

      if (instruction->result == value)
        return instruction;  // Return the instruction if it defines the value
    }
  }

  // No such instruction found, return NULL
  return NULL;
}
