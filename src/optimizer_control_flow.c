/**
 * @file optimizer_control_flow.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for control flow analysis and optimization
 *
 * Contains definitions for helper functions for control flow analysis and
 * optimization.
 */

#include "optimizer_control_flow.h"

#include "ir.h"
#include "ir_free_helpers.h"
#include "optimizer.h"

#include <stdbool.h>  // bool

/**
 * @brief Recursively explores reachable blocks from a given block in a function
 *
 * @param[in,out] optimizer_ctx The optimizer context
 * @param[in] function The function to explore
 * @param[in] block_id The ID of the block to start from
 */
static void explore_reachable_blocks(OptimizerCtx* optimizer_ctx,
                                     IrFunction* function,
                                     IrBlockId block_id) {
  // Check if the block wasn't already explored
  if (optimizer_ctx->reachable[block_id])
    return;

  // Mark the block as reachable
  optimizer_ctx->reachable[block_id] = true;

  // Explore all blocks reachable from this one
  IrBlockTerminator* terminator = &function->blocks.data[block_id].terminator;
  switch (terminator->type) {
    case IR_TERMINATOR_JUMP:
      explore_reachable_blocks(optimizer_ctx, function,
                               terminator->value.jump_target);
      break;

    case IR_TERMINATOR_CONDITIONAL:
      explore_reachable_blocks(
          optimizer_ctx, function,
          terminator->value.conditional_target.true_target);
      explore_reachable_blocks(
          optimizer_ctx, function,
          terminator->value.conditional_target.false_target);
      break;

    default:
      break;
  }
}

bool optimizer_remove_unreachable_blocks(Ctx* ctx, IrFunctionId function_id) {
  IrFunction* function = &ctx->ir.functions.data[function_id];

  // Reset reachable blocks
  OptimizerCtx* optimizer_ctx = &ctx->optimizer;
  for (size_t i = 0; i < function->blocks.count; i++)
    optimizer_ctx->reachable[i] = false;

  // Explore reachable blocks from the entrypoint
  explore_reachable_blocks(optimizer_ctx, function, function->entrypoint);

  size_t reachable_count = 0;
  for (size_t i = 0; i < function->blocks.count; i++)
    if (optimizer_ctx->reachable[i])
      reachable_count++;

  // Check that there are unreachable blocks to remove
  if (reachable_count == function->blocks.count)
    return false;

  // Removing blocks will cause the remaining ones to change their ids, create a
  // map of old to new ids
  IrBlockId* map = optimizer_ctx->block_id_map;
  IrBlockId next_new_id = 0;
  for (IrBlockId old_id = 0; old_id < function->blocks.count; old_id++)
    map[old_id] =
        optimizer_ctx->reachable[old_id] ? next_new_id++ : IR_INVALID_ID;

  // Remove unreachable blocks
  IrBlockId next_free_id = 0;
  for (IrBlockId old_id = 0; old_id < function->blocks.count; old_id++) {
    IrBlock* block = &function->blocks.data[old_id];

    // If the block is unreachable, free it and continue to the next one
    if (!optimizer_ctx->reachable[old_id]) {
      ir_free_block(block);
      continue;
    }

    // Otherwise (it's reachable), update it's terminator targets
    IrBlockTerminator* terminator = &block->terminator;
    switch (terminator->type) {
      case IR_TERMINATOR_JUMP:
        terminator->value.jump_target = map[terminator->value.jump_target];
        break;

      case IR_TERMINATOR_CONDITIONAL:
        terminator->value.conditional_target.true_target =
            map[terminator->value.conditional_target.true_target];
        terminator->value.conditional_target.false_target =
            map[terminator->value.conditional_target.false_target];
        break;

      default:
        break;
    }

    // And (shallow) move the block to the next free id
    function->blocks.data[next_free_id++] = *block;
  }

  // Update the block count
  function->blocks.count = next_free_id;

  // Update the function entrypoint
  function->entrypoint = map[function->entrypoint];

  return true;
}
