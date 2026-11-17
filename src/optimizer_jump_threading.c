/**
 * @file optimizer_jump_threading.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for jump threading optimization
 *
 * Contains definitions for helper functions for control flow analysis and
 * optimization.
 */

#include "optimizer_jump_threading.h"

#include <stdbool.h>  // bool
#include "ir.h"

/**
 * @brief Simplifies a jump terminator if the target block is empty and the
 *        terminator is a simple jump.
 *
 * @param function The function containing the block.
 * @param jump_target The jump target to update if the jump is simplified.
 * @return true if the jump terminator was simplified, false otherwise.
 */
static bool simplify_jump(IrFunction* function, IrBlockId* jump_target) {
  IrBlock* target = &function->blocks.data[*jump_target];

  // Check if the target block is empty
  if (target->instructions.count != 0)
    return false;

  // Check if the target block's terminator is a simple jump
  if (target->terminator.type != IR_TERMINATOR_JUMP)
    return false;

  // Guard against self-loops
  if (target->terminator.value.jump_target == *jump_target)
    return false;

  // Replace the jump target with the block's terminator target
  *jump_target = target->terminator.value.jump_target;

  return true;
}

bool optimizer_simplify_jumps(Ctx* ctx, IrFunctionId fid) {
  IrFunction* function = &ctx->ir.functions.data[fid];

  bool changed = false;

  // Go through each block in the function
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    IrBlockTerminator* terminator = &function->blocks.data[bid].terminator;

    // Attempt to simplify consecutive jumps
    switch (terminator->type) {
      case IR_TERMINATOR_JUMP:
        changed |= simplify_jump(function, &terminator->value.jump_target);
        break;

      case IR_TERMINATOR_CONDITIONAL:
        changed |= simplify_jump(
            function, &terminator->value.conditional_target.true_target);
        changed |= simplify_jump(
            function, &terminator->value.conditional_target.false_target);
        break;

      // Ignore remaining terminator types
      default:
        break;
    }
  }

  // Attempt to simplify the entrypoint of the function
  while (simplify_jump(function, &function->entrypoint))
    ;

  return changed;
}
