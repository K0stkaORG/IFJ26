/**
 * @file optimizer.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Implementation of the main function of the IFJ26 optimizer
 *
 * This file contains the implementation of the main function of the IFJ26
 * optimizer.
 */

#include "optimizer.h"

#include "ctx.h"
#include "ir.h"
#include "optimizer_constant_folding.h"
#include "optimizer_control_flow.h"
#include "optimizer_dead_code_elimination.h"
#include "optimizer_forwarding.h"
#include "optimizer_jump_threading.h"

#include <stdbool.h>  // bool

void optimize_ir(Ctx* ctx) {
  // Prepare the optimizer context
  optimizer_prepare(ctx);

  // Remove unreachable functions (before optimizing, to avoid optimizing
  // unreachable functions)
  size_t removed = optimizer_remove_unreachable_functions(ctx);
  if (removed > 0)
    DEBUG("Removed (pre-optimization) %zu unreachable functions", removed);

  // Iteratively optimize functions until no more changes are made
  for (IrFunctionId fid = 0; fid < ctx->ir.functions.count; fid++) {
    size_t iterations = 0;
    bool changed;

    do {
      iterations++;

      changed = optimizer_fold_function(ctx, fid);
      changed |= optimizer_eliminate_dead_code(ctx, fid);
      changed |= optimizer_forward_stores(ctx, fid);
      changed |= optimizer_remove_unreachable_blocks(ctx, fid);
      changed |= optimizer_remove_unused_stores(ctx, fid);
      changed |= optimizer_simplify_jumps(ctx, fid);
    } while (changed);

    // Print debug info
    iterations--;
    if (iterations > 0)
      DEBUG("Optimization of function %s took %zu iterations",
            ctx->ir.functions.data[fid].name, iterations);
    else
      DEBUG("Failed to optimize function %s", ctx->ir.functions.data[fid].name);
  }

  // Remove unreachable functions (also after optimizing, to remove functions
  // that are no longer reachable)
  removed = optimizer_remove_unreachable_functions(ctx);
  if (removed > 0)
    DEBUG("Removed (post-optimization) %zu unreachable functions", removed);
}

void optimizer_init(OptimizerCtx* optimizer) {
  // Initialize value uses buffer to NULL
  optimizer->value_uses = NULL;

  // Initialize reachable buffer to NULL
  optimizer->reachable = NULL;

  // Initialize block ID map to NULL
  optimizer->id_map = NULL;
}

void optimizer_prepare(Ctx* ctx) {
  // Compute the maximum number of value uses across all functions
  size_t max_values = 0;
  for (IrFunctionId fid = 0; fid < ctx->ir.functions.count; fid++)
    if (ctx->ir.functions.data[fid].value_count > max_values)
      max_values = ctx->ir.functions.data[fid].value_count;

  // Allocate value uses buffer
  ctx->optimizer.value_uses = TRY_MALLOC(max_values * sizeof(size_t));

  // Compute the maximum number of blocks across all functions and the number of
  // functions (so that we can reuse the same buffer for blocks and functions)
  size_t buffer_size = ctx->ir.functions.count;
  for (IrFunctionId fid = 0; fid < ctx->ir.functions.count; fid++)
    if (ctx->ir.functions.data[fid].blocks.count > buffer_size)
      buffer_size = ctx->ir.functions.data[fid].blocks.count;

  // Allocate reachable buffer
  ctx->optimizer.reachable = TRY_MALLOC(buffer_size * sizeof(bool));

  // Allocate block ID map
  ctx->optimizer.id_map = TRY_MALLOC(buffer_size * sizeof(size_t));
}

void optimizer_free(OptimizerCtx* optimizer) {
  // Free value uses buffer if it exists
  if (optimizer->value_uses != NULL) {
    free(optimizer->value_uses);
    optimizer->value_uses = NULL;
  }

  // Free reachable buffer if it exists
  if (optimizer->reachable != NULL) {
    free(optimizer->reachable);
    optimizer->reachable = NULL;
  }

  // Free block ID map if it exists
  if (optimizer->id_map != NULL) {
    free(optimizer->id_map);
    optimizer->id_map = NULL;
  }
}
