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
#include <stddef.h>   // size_t

/**
 * @brief Recursively explores reachable blocks from a given block in a function
 *
 * @param[in,out] optimizer_ctx The optimizer context
 * @param[in] function The function to explore
 * @param[in] block_id The ID of the block to start from
 * @param[in] ignore_current Whether to ignore the current block
 */
static void explore_reachable_blocks(OptimizerCtx* optimizer_ctx,
                                     IrFunction* function,
                                     IrBlockId block_id,
                                     bool ignore_current) {
  // Check if the block wasn't already explored
  if (optimizer_ctx->reachable[block_id])
    return;

  // Mark the block as reachable
  optimizer_ctx->reachable[block_id] = !ignore_current;

  // Explore all blocks reachable from this one
  IrBlockTerminator* terminator = &function->blocks.data[block_id].terminator;
  switch (terminator->type) {
    case IR_TERMINATOR_JUMP:
      explore_reachable_blocks(optimizer_ctx, function,
                               terminator->value.jump_target, false);
      break;

    case IR_TERMINATOR_CONDITIONAL:
      explore_reachable_blocks(optimizer_ctx, function,
                               terminator->value.conditional_target.true_target,
                               false);
      explore_reachable_blocks(
          optimizer_ctx, function,
          terminator->value.conditional_target.false_target, false);
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
  explore_reachable_blocks(optimizer_ctx, function, function->entrypoint,
                           false);

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

/**
 * @brief Checks if the store instruction is redundant
 *
 * In the current block checks for any uses of the value + if there is a
 * successive store instruction before we read the value. In the blocks
 * reachable from this block (might include this block itself but from it's
 * start, not just from the given instruction) checks only for uses of the value
 * as it's hard to prove in which order things get executed.
 *
 * @param[in] optimizer_ctx The optimizer context
 * @param[in] function The function to check
 * @param[in] block_id The id of the block containing the store instruction
 * @param[in] store_id The id of the store instruction to check
 * @return true if the store instruction is redundant, false otherwise
 */
static bool is_store_redundant(OptimizerCtx* optimizer_ctx,
                               IrFunction* function,
                               IrBlockId block_id,
                               size_t store_id) {
  IrInstruction* store =
      &function->blocks.data[block_id].instructions.data[store_id];

  // First check the rest of the current block
  for (size_t inst_id = store_id + 1;
       inst_id < function->blocks.data[block_id].instructions.count;
       inst_id++) {
    IrInstruction* other =
        &function->blocks.data[block_id].instructions.data[inst_id];

    switch (other->type) {
      case IR_INST_STORE:
        // Check if the store target is the same as the other store target
        if (store->operands.store.target == other->operands.store.target)
          return true;  // If so, that makes it redundant
        break;

      case IR_INST_LOAD:
        // Check if the load target is the same as the store target
        if (store->operands.store.target == other->operands.load.source)
          return false;  // If so, that makes it explicitly non-redundant
        break;

      default:
        break;
    }
  }

  // If we reach here, the store is redundant unless a reachable block uses it

  // Go through each block
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    // Check if the block is reachable
    if (!optimizer_ctx->reachable[bid])
      continue;  // Skip unreachable blocks

    // Go through each instruction in the block
    for (size_t inst_id = 0;
         inst_id < function->blocks.data[bid].instructions.count; inst_id++) {
      IrInstruction* instruction =
          &function->blocks.data[bid].instructions.data[inst_id];

      // Only check IR_INST_LOAD instructions
      if (instruction->type != IR_INST_LOAD)
        continue;

      // Check if the load instruction uses the store target
      if (instruction->operands.load.source == store->operands.store.target)
        return false;  // If so, that makes the store non-redundant
    }
  }

  // We have not found any use of the store target anywhere in any reachable
  // future, so it is redundant
  return true;
}

bool optimizer_remove_unused_stores(Ctx* ctx, IrFunctionId function_id) {
  IrFunction* function = &ctx->ir.functions.data[function_id];

  bool changed = false;

  // Go through each block
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    // Reset reachable blocks
    OptimizerCtx* optimizer_ctx = &ctx->optimizer;
    for (size_t i = 0; i < function->blocks.count; i++)
      optimizer_ctx->reachable[i] = false;

    // Explore reachable blocks from this block (ignore the initial block for
    // better results)
    explore_reachable_blocks(optimizer_ctx, function, bid, true);

    // Go through each instruction in the block
    for (size_t inst_id = 0;
         inst_id < function->blocks.data[bid].instructions.count; inst_id++) {
      IrInstruction* instruction =
          &function->blocks.data[bid].instructions.data[inst_id];

      // Skip instructions other than IR_INST_STORE
      if (instruction->type != IR_INST_STORE)
        continue;

      // Check if the store is redundant
      if (!is_store_redundant(optimizer_ctx, function, bid, inst_id))
        continue;

      // Remove the store instruction
      ir_free_instruction(instruction);
      array_remove(ctx, &function->blocks.data[bid].instructions, inst_id);

      // Move back one instruction to account for the removed store
      inst_id--;

      changed = true;
    }
  }

  return changed;
}
