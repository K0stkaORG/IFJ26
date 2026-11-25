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
 * @brief Recursively explores reachable functions from the given function ID
 *
 * @param ir The IR program
 * @param reachable The reachable buffer
 * @param function_id The ID of the function to explore
 */
static void explore_reachable_functions(IrProgram* ir,
                                        bool* reachable,
                                        IrFunctionId function_id) {
  IrFunction* function = &ir->functions.data[function_id];

  // Check if the function has already been explored
  if (reachable[function_id])
    return;

  // Mark the function as reachable
  reachable[function_id] = true;

  // Go through all blocks and instructions of the function
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    IrBlock* block = &function->blocks.data[bid];
    for (size_t inst_id = 0; inst_id < block->instructions.count; inst_id++) {
      IrInstruction* inst = &block->instructions.data[inst_id];

      // If the instruction is a call to a user-defined function, explore it
      if (inst->type == IR_INST_CALL && inst->operands.call.type == IR_FX_USER)
        explore_reachable_functions(ir, reachable, inst->operands.call.id);
    }
  }
}

size_t optimizer_remove_unreachable_functions(Ctx* ctx) {
  IrProgram* ir = &ctx->ir;

  // Check if the program has any functions
  if (ir->functions.count == 0)
    return 0;

  // Reset the reachable buffer
  bool* reachable = ctx->optimizer.functions;
  for (IrFunctionId fid = 0; fid < ir->functions.count; fid++)
    reachable[fid] = false;

  // Explore functions reachable from the entrypoint
  explore_reachable_functions(ir, reachable, ir->entrypoint);

  size_t unreachable_count = 0;
  for (IrFunctionId fid = 0; fid < ir->functions.count; fid++)
    if (!reachable[fid])
      unreachable_count++;

  // Check that there are functions to remove
  if (unreachable_count == 0)
    return 0;

  // Removing functions will cause the remaining ones to change their ids,
  // create a map of old to new ids
  IrFunctionId* map = ctx->optimizer.function_id_map;
  IrFunctionId next_id = 0;
  for (IrFunctionId fid = 0; fid < ir->functions.count; fid++)
    map[fid] = reachable[fid] ? next_id++ : IR_INVALID_ID;

  // Remove unreachable functions
  next_id = 0;
  for (IrFunctionId fid = 0; fid < ir->functions.count; fid++) {
    IrFunction* function = &ir->functions.data[fid];

    // If a function is unreachable, free it and skip it
    if (!reachable[fid]) {
      ir_free_function(function);
      continue;
    }

    // Otherwise, update all it's call instructions
    for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
      IrBlock* block = &function->blocks.data[bid];
      for (size_t inst_id = 0; inst_id < block->instructions.count; inst_id++) {
        IrInstruction* inst = &block->instructions.data[inst_id];

        // Check that the instruction is a call to a user-defined function, if
        // so, update its id
        if (inst->type == IR_INST_CALL &&
            inst->operands.call.type == IR_FX_USER)
          inst->operands.call.id = map[inst->operands.call.id];
      }
    }

    // Move the function only if an earlier removal freed a slot.
    if (next_id != fid)
      ir->functions.data[next_id] = *function;
    next_id++;
  }

  // Update the count of functions in the program
  ir->functions.count = next_id;

  // Update the entrypoint of the program
  ir->entrypoint = map[ir->entrypoint];

  // Return the number of unreachable (removed) functions
  return unreachable_count;
}

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
  if (optimizer_ctx->blocks[block_id])
    return;

  // Mark the block as reachable
  optimizer_ctx->blocks[block_id] = !ignore_current;

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
    optimizer_ctx->blocks[i] = false;

  // Explore reachable blocks from the entrypoint
  explore_reachable_blocks(optimizer_ctx, function, function->entrypoint,
                           false);

  size_t reachable_count = 0;
  for (size_t i = 0; i < function->blocks.count; i++)
    if (optimizer_ctx->blocks[i])
      reachable_count++;

  // Check that there are unreachable blocks to remove
  if (reachable_count == function->blocks.count)
    return false;

  // Removing blocks will cause the remaining ones to change their ids, create a
  // map of old to new ids
  IrBlockId* map = optimizer_ctx->block_data;
  IrBlockId next_id = 0;
  for (IrBlockId old_id = 0; old_id < function->blocks.count; old_id++)
    map[old_id] = optimizer_ctx->blocks[old_id] ? next_id++ : IR_INVALID_ID;

  // Remove unreachable blocks
  next_id = 0;
  for (IrBlockId old_id = 0; old_id < function->blocks.count; old_id++) {
    IrBlock* block = &function->blocks.data[old_id];

    // If the block is unreachable, free it and continue to the next one
    if (!optimizer_ctx->blocks[old_id]) {
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

    // Move the block only if an earlier removal freed a slot.
    if (next_id != old_id)
      function->blocks.data[next_id] = *block;
    next_id++;
  }

  // Update the block count
  function->blocks.count = next_id;

  // Update the function entrypoint
  function->entrypoint = map[function->entrypoint];

  return true;
}
