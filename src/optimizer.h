/**
 * @file optimizer.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Header file for the IFJ26 optimizer
 *
 * This file contains the declarations of the functions and data structures
 * used by the IFJ26 optimizer.
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ir.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

typedef struct Ctx Ctx;

/**
 * @brief Optimizes the intermediate representation of the program
 *
 * @param[in,out] ctx The context of the program
 */
void optimize_ir(Ctx* ctx);

/**
 * @brief Context for the optimizer
 *
 * This structure holds dynamic memory used by the optimizer to allow for more
 * efficient memory usage.
 */
typedef struct {
  // Number of times each value is used (valid only in specific contexts)
  size_t* value_uses;

  // Tracks whether each block is reachable (valid only in specific contexts)
  bool* reachable;

  // Maps block IDs to their index in the reachable buffer
  IrBlockId* block_id_map;
} OptimizerCtx;

/**
 * @brief Initializes the optimizer context
 *
 * @param[in,out] optimizer The context of the optimizer
 */
void optimizer_init(OptimizerCtx* optimizer);

/**
 * @brief Prepares the optimizer context for use
 *
 * @param[in,out] ctx The context of the program
 */
void optimizer_prepare(Ctx* ctx);

/**
 * @brief Frees the optimizer context
 *
 * @param[in,out] optimizer The context of the optimizer
 */
void optimizer_free(OptimizerCtx* optimizer);

#endif
