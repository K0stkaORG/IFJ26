/**
 * @file lowering_calls.c
 * Project: IFJ26
 *
 * @brief Function-call lowering and argument-buffer cleanup
 */

#include "lowering_internal.h"

#include <stdint.h>  // SIZE_MAX
#include <stdlib.h>  // free

IrOperand lower_call(Lower* lower, AstNode* node) {
  Ctx* ctx = lower->ctx;

  // Reserve the argument buffer before evaluating any arguments
  size_t count = ast_node_get_child_count(node) - AST_CALL_FIRST_ARG;
  if (count > SIZE_MAX / sizeof(IrOperand))
    THROW(E_INTERNAL_ERROR, "Too many call arguments");
  IrOperand* args = count ? TRY_MALLOC(count * sizeof(*args)) : NULL;

  IrOperand result = unit_operand();
  TRY {
    // Evaluate arguments left to right, stopping if an argument returns
    for (size_t i = 0; i < count && lower->block != IR_INVALID_ID; i++)
      args[i] =
          lower_node(lower, lower_child(ctx, node, AST_CALL_FIRST_ARG + i));
    if (lower->block != IR_INVALID_ID) {
      const SymtableData* callee =
          lower_symbol(ctx, lower_child(ctx, node, AST_CALL_CALLEE));

      // Built-in symbols carry a function kind instead of a user function ID
      bool builtin = (ast_node_get_flags(node) & AST_FLAG_BUILTIN) != 0;
      result = lower_emit_call(
          lower, builtin ? (IrFunctionType)callee->ir_index : IR_FX_USER,
          builtin ? IR_INVALID_ID : callee->ir_index, callee->ir_type, args,
          count);
    }
  }
  CATCH {
    // args was allocated before TRY and remains valid after longjmp
    free(args);
    RETHROW();
  }

  // IR construction copied the arguments, so the temporary buffer can go
  free(args);
  return result;
}
