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
#include "optimizer_dead_code_elimination.h"
#include "optimizer_forwarding.h"

#include <stdbool.h>  // bool

void optimize_ir(Ctx* ctx) {
  // Prepare the optimizer context
  optimizer_prepare(ctx);

  // Iteratively optimize functions until no more changes are made
  for (IrFunctionId fid = 0; fid < ctx->ir.functions.count; fid++) {
    size_t iterations = 0;
    bool changed;

    do {
      iterations++;

      changed = optimizer_fold_function(ctx, fid);
      changed |= optimizer_eliminate_dead_code(ctx, fid);
      changed |= optimizer_forward_stores(ctx, fid);
    } while (changed);

    // Print debug info
    iterations--;
    if (iterations > 0)
      DEBUG("Optimization of function %s took %zu iterations",
            ctx->ir.functions.data[fid].name, iterations);
    else
      DEBUG("Failed to optimize function %s", ctx->ir.functions.data[fid].name);
  }
}

void optimizer_init(OptimizerCtx* optimizer) {
  // Initialize value uses buffer to NULL
  optimizer->value_uses = NULL;
}

void optimizer_prepare(Ctx* ctx) {
  // Compute the maximum number of value uses across all functions
  size_t max_values = 0;
  for (IrFunctionId fid = 0; fid < ctx->ir.functions.count; fid++)
    if (ctx->ir.functions.data[fid].value_count > max_values)
      max_values = ctx->ir.functions.data[fid].value_count;

  // Allocate value uses buffer
  ctx->optimizer.value_uses = TRY_MALLOC(max_values * sizeof(size_t));
}

void optimizer_free(OptimizerCtx* optimizer) {
  // Free value uses buffer if it exists
  if (optimizer->value_uses != NULL) {
    free(optimizer->value_uses);
    optimizer->value_uses = NULL;
  }
}
