/**
 * @file optimizer_fold_branches.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 15. 11. 2026
 *
 * @brief Helpers for folding branches in the IR
 *
 * Contains declarations for helper functions for simplifying branches.
 */

#ifndef OPTIMIZER_FOLD_BRANCHES_H
#define OPTIMIZER_FOLD_BRANCHES_H

#include "ir.h"

#include <stdbool.h>  // bool

/**
 * @brief Tries to fold a branch into a simple jump if the condition is a
 * constant value or if the true/false targets are the same
 *
 * @param terminator The terminator to fold
 * @return true if the branch was folded, false otherwise
 */
bool optimizer_fold_branch(IrBlockTerminator* terminator);

/**
 * @brief Tries to simplify a negated branch into a normal branch with jump
 * targets swapped
 *
 * @code
 *  if (!cond) {
 *      // do something
 *  } else {
 *      // do something else
 *  }
 *
 *  // Will be simplified to:
 *  if (cond) {
 *      // do something else
 *  } else {
 *      // do something
 *  }
 * @endcode
 *
 * @param ctx The context of the program
 * @param function The function containing the branch
 * @param terminator The terminator to fold
 * @return true if the branch was simplified, false otherwise
 */
bool optimizer_simplify_negated_branch(Ctx* ctx,
                                       IrFunction* function,
                                       IrBlockTerminator* terminator);

#endif
