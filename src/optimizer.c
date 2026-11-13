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

#include "ir.h"
#include "optimizer_constant_folding.h"

#include <stdbool.h>  // bool

void optimize_ir(Ctx* ctx) {
  // For all functions, fold constants until no more can be folded
  for (IrFunctionId fid = 0; fid < ctx->ir.functions.count; fid++)
    do {
    } while (optimizer_fold_function(ctx, fid));
}
