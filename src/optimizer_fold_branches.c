/**
 * @file optimizer_fold_branches.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 15. 11. 2026
 *
 * @brief Helpers for folding branches in the IR
 *
 * Contains definitions for helper functions for simplifying branches.
 */

#include "optimizer_fold_branches.h"

#include "ir.h"
#include "ir_helpers.h"
#include "optimizer_helpers.h"

#include <stdbool.h>  // bool

bool optimizer_fold_branch(IrBlockTerminator* terminator) {
  // Check that the terminator is a conditional branch
  if (terminator->type != IR_TERMINATOR_CONDITIONAL)
    return false;

  IrOperand* condition = &terminator->value.conditional_target.condition;
  IrBlockId true_target = terminator->value.conditional_target.true_target;
  IrBlockId false_target = terminator->value.conditional_target.false_target;

  // If true and false targets are the same, fold into a simple jump
  if (true_target == false_target) {
    terminator->type = IR_TERMINATOR_JUMP;
    terminator->value.jump_target = true_target;

    return true;
  }

  // If the condition is a constant value, fold into a simple jump
  if (condition->type == IR_OP_BOOL) {
    terminator->type = IR_TERMINATOR_JUMP;
    terminator->value.jump_target =
        condition->value.bool_value ? true_target : false_target;

    return true;
  }

  // Return false if the branch is not foldable
  return false;
}

bool optimizer_simplify_negated_branch(Ctx* ctx,
                                       IrFunction* function,
                                       IrBlockTerminator* terminator) {
  // Check that the terminator is a conditional branch
  if (terminator->type != IR_TERMINATOR_CONDITIONAL)
    return false;

  IrOperand* condition = &terminator->value.conditional_target.condition;
  IrBlockId true_target = terminator->value.conditional_target.true_target;
  IrBlockId false_target = terminator->value.conditional_target.false_target;

  // Check that the condition depends on a value
  if (condition->type != IR_OP_VALUE)
    return false;

  // Find the definition of the condition value
  IrInstruction* definition =
      optimizer_find_definition(function, condition->value.id);

  // Check that the definition is a negation
  if (definition->type != IR_INST_NOT)
    return false;

  // Simplify the branch by inverting the condition and swapping the targets
  IrOperand negated = ir_copy_operand(ctx, definition->operands.unary.operand);

  terminator->value.conditional_target.condition = negated;
  terminator->value.conditional_target.true_target = false_target;
  terminator->value.conditional_target.false_target = true_target;

  return true;
}
