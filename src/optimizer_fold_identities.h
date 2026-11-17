/**
 * @file optimizer_fold_identities.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 15. 11. 2026
 *
 * @brief Helpers for folding identities in the IR
 *
 * This file contains declarations for helper functions for folding identities
 * in the IR.
 */

#ifndef OPTIMIZER_FOLD_IDENTITIES_H
#define OPTIMIZER_FOLD_IDENTITIES_H

#include "ir.h"

#include <stdbool.h>  // bool

/**
 * @brief Attempts to fold a binary instruction even if only one operand is a
 * constant
 *
 * Folds things like x + 0, x * 1, etc.
 *
 * @param[in] instruction The instruction to fold (must be IR_INST_CLASS_BINARY)
 * @param[out] result The result of the folded instruction
 * @return true if the instruction was folded, false otherwise
 */
bool optimizer_fold_identity(IrInstruction* instruction, IrOperand* result);

#endif
