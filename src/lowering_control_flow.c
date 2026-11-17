/**
 * @file lowering_control_flow.c
 * Project: IFJ26
 *
 * @brief Conditional, short-circuit and loop lowering
 */

#include "lowering_internal.h"

IrOperand lower_if(Lower* lower, AstNode* node) {
  Ctx* ctx = lower->ctx;

  // Lower the condition before splitting the current path
  IrOperand condition = lower_node(lower, lower_child(ctx, node, AST_IF_COND));
  if (lower->block == IR_INVALID_ID)
    return unit_operand();
  IrType type = ast_to_ir_data_type(ctx, ast_node_get_data_type(node));
  IrStackSlotId result = type == IR_UNIT
                             ? IR_INVALID_ID
                             : ir_add_slot(ctx, lower->function, "_if", type);
  IrBlockId arms[] = {lower_new_block(lower), lower_new_block(lower)};
  lower_branch(lower, condition, arms[0], arms[1]);

  // Only create a merge block if at least one arm can continue
  IrBlockId merge = IR_INVALID_ID;

  // Lower both arms, but do not append a jump after an explicit return
  for (size_t i = 0; i < 2; i++) {
    lower->block = arms[i];
    IrOperand value = lower_node(
        lower, lower_child(ctx, node, i == 0 ? AST_IF_THEN : AST_IF_ELSE));
    if (lower->block == IR_INVALID_ID)
      continue;
    if (result != IR_INVALID_ID)
      lower_store(lower, result, value);
    if (merge == IR_INVALID_ID)
      merge = lower_new_block(lower);
    lower_jump(lower, merge);
  }

  // Both returning arms leave the whole conditional path closed
  lower->block = merge;
  return merge == IR_INVALID_ID || result == IR_INVALID_ID
             ? unit_operand()
             : lower_load(lower, result, type);
}

IrOperand lower_logic(Lower* lower, AstNode* node) {
  Ctx* ctx = lower->ctx;
  bool is_and = ast_node_get_type(node) == AST_NODE_AND;
  IrOperand left = lower_node(lower, lower_child(ctx, node, AST_BINARY_LEFT));
  if (lower->block == IR_INVALID_ID)
    return unit_operand();
  IrStackSlotId result = ir_add_slot(ctx, lower->function, "_bool", IR_BOOL);

  // The skipped right operand leaves false for AND and true for OR
  lower_store(lower, result, bool_operand(!is_and));
  IrBlockId right = lower_new_block(lower), merge = lower_new_block(lower);

  // Evaluate the right operand only when the left does not decide the result
  lower_branch(lower, left, is_and ? right : merge, is_and ? merge : right);

  // Store the right result only if evaluating it leaves a continuing path
  lower->block = right;
  IrOperand value = lower_node(lower, lower_child(ctx, node, AST_BINARY_RIGHT));
  if (lower->block != IR_INVALID_ID) {
    lower_store(lower, result, value);
    lower_jump(lower, merge);
  }
  lower->block = merge;
  return lower_load(lower, result, IR_BOOL);
}

void lower_while(Lower* lower, AstNode* node) {
  Ctx* ctx = lower->ctx;
  IrBlockId test = lower_new_block(lower), body = lower_new_block(lower),
            end = lower_new_block(lower);
  lower_jump(lower, test);
  lower->block = test;

  // Re-evaluate the condition on every visit, including the first iteration
  IrOperand condition =
      lower_node(lower, lower_child(ctx, node, AST_WHILE_COND));
  if (lower->block != IR_INVALID_ID)
    lower_branch(lower, condition, body, end);
  lower->block = body;

  // A body that returns must not jump back to the condition
  lower_node(lower, lower_child(ctx, node, AST_WHILE_BODY));
  if (lower->block != IR_INVALID_ID)
    lower_jump(lower, test);
  lower->block = end;
}

void lower_for(Lower* lower, AstNode* node) {
  Ctx* ctx = lower->ctx;

  // Evaluate both bounds and the optional step exactly once
  AstNode* range = lower_child(ctx, node, AST_FOR_SOURCE);
  IrOperand first = lower_node(lower, lower_child(ctx, range, 0));
  IrOperand last = lower_node(lower, lower_child(ctx, range, 1));
  AstNode* step_node = lower_child(ctx, range, AST_RANGE_STEP);
  IrOperand step = ast_node_get_type(step_node) == AST_NODE_NONE
                       ? int_operand(1)
                       : lower_node(lower, step_node);
  if (lower->block == IR_INVALID_ID)
    return;

  // A non-literal zero step is a runtime error with exit code 40
  IrOperand zero =
      lower_binary(lower, IR_INST_EQ, IR_BOOL, step, int_operand(0));
  IrBlockId error = lower_new_block(lower), setup = lower_new_block(lower);
  lower_branch(lower, zero, error, setup);
  ir_set_terminator(ctx, lower->function, error,
                    (IrBlockTerminator){.type = IR_TERMINATOR_EXIT,
                                        .value.exit_code = int_operand(40)});
  lower->block = setup;
  /* A double counter holds every Int32 sum exactly and avoids overflow in
   * the update after the final iteration, including Int32 endpoint ranges. */
  IrOperand start = add_unary(ctx, lower->function, setup, IR_INST_INT2DOUBLE,
                              IR_DOUBLE, first);
  IrOperand limit = add_unary(ctx, lower->function, setup, IR_INST_INT2DOUBLE,
                              IR_DOUBLE, last);
  IrOperand stride = add_unary(ctx, lower->function, setup, IR_INST_INT2DOUBLE,
                               IR_DOUBLE, step);

  // Select the direction once; the step is constant throughout the loop
  IrOperand positive =
      lower_binary(lower, IR_INST_GT, IR_BOOL, step, int_operand(0));
  IrStackSlotId counter =
      ir_add_slot(ctx, lower->function, "_range", IR_DOUBLE);
  lower_store(lower, counter, start);
  IrBlockId test = lower_new_block(lower), up = lower_new_block(lower),
            down = lower_new_block(lower);
  IrBlockId body = lower_new_block(lower), increment = lower_new_block(lower),
            end = lower_new_block(lower);
  lower_jump(lower, test);
  lower->block = test;
  IrOperand current = lower_load(lower, counter, IR_DOUBLE);
  lower_branch(lower, positive, up, down);

  // Use inclusive comparisons for to and exclusive comparisons for until
  bool inclusive = ast_node_get_type(range) == AST_NODE_RANGE_TO;
  lower->block = up;
  IrOperand condition = lower_binary(lower, inclusive ? IR_INST_LE : IR_INST_LT,
                                     IR_BOOL, current, limit);
  lower_branch(lower, condition, body, end);
  lower->block = down;
  condition = lower_binary(lower, inclusive ? IR_INST_GE : IR_INST_GT, IR_BOOL,
                           current, limit);
  lower_branch(lower, condition, body, end);
  lower->block = body;

  // Expose an Int loop variable only while the counter is within the bounds
  IrOperand value =
      lower_emit_call(lower, IR_FX_TO_INT, IR_INVALID_ID, IR_INT, &current, 1);
  lower_store(
      lower, lower_symbol(ctx, lower_child(ctx, node, AST_FOR_IDENT))->ir_index,
      value);

  // A false guard skips this iteration; later guards are not evaluated
  for (size_t i = AST_FOR_FIRST_GUARD; i < ast_node_get_child_count(node);
       i++) {
    condition = lower_node(lower, lower_child(ctx, node, i));
    if (lower->block == IR_INVALID_ID)
      break;
    IrBlockId next = lower_new_block(lower);
    lower_branch(lower, condition, next, increment);
    lower->block = next;
  }
  lower_node(lower, lower_child(ctx, node, AST_FOR_BODY));
  if (lower->block != IR_INVALID_ID)
    lower_jump(lower, increment);

  // Advance the counter after a continuing body or a skipped iteration
  lower->block = increment;
  lower_store(lower, counter,
              lower_binary(lower, IR_INST_ADD, IR_DOUBLE, current, stride));
  lower_jump(lower, test);
  lower->block = end;
}
