/**
 * @file ir_internal.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 3. 11. 2026
 *
 * @brief Internal functions shared by the IR modules
 */

#ifndef IR_INTERNAL_H
#define IR_INTERNAL_H

#include "ir.h"

/**
 * @brief Frees the memory used by an operand
 *
 * Does not free the operand pointer itself. Used internally by IR cleanup
 * and construction helpers.
 *
 * @param[in,out] operand The operand to free
 */
void ir_free_operand(IrOperand* operand);

#endif
