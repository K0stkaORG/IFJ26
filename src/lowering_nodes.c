/**
 * @file lowering_nodes.c
 * Project: IFJ26
 *
 * @brief AST node dispatch and expression lowering
 */

#include "lowering_internal.h"

#include <string.h>  // strcmp

/**
 * @brief Selects the IR opcode of a checked binary expression
 *
 * @param ctx The shared context
 * @param node The checked binary expression
 * @return The matching opcode, including typed division and concatenation
 */
static IrInstructionType binary_kind(Ctx* ctx, AstNode* node) {
  // Semantic types distinguish numeric addition from string concatenation
  bool string = ast_node_get_data_type(node) == AST_DATA_TYPE_STRING;
  switch (ast_node_get_type(node)) {
    case AST_NODE_ADD:
      return string ? IR_INST_CONCAT : IR_INST_ADD;
    case AST_NODE_SUB:
      return IR_INST_SUB;
    case AST_NODE_MUL:
      return IR_INST_MUL;
    case AST_NODE_DIV:
      return ast_node_get_data_type(node) == AST_DATA_TYPE_INT ? IR_INST_IDIV
                                                               : IR_INST_DIV;
    case AST_NODE_EQ:
      return IR_INST_EQ;
    case AST_NODE_NEQ:
      return IR_INST_NE;
    case AST_NODE_LT:
      return IR_INST_LT;
    case AST_NODE_LE:
      return IR_INST_LE;
    case AST_NODE_GT:
      return IR_INST_GT;
    case AST_NODE_GE:
      return IR_INST_GE;
    default:
      THROW(E_INTERNAL_ERROR, "Unknown binary operation");
  }
  return IR_INST_ADD;
}

IrOperand lower_node(Lower* lower, AstNode* node) {
  Ctx* ctx = lower->ctx;
  if (lower->block == IR_INVALID_ID)
    return unit_operand();

  // Dispatch by AST kind; child evaluation may change the current block
  AstNodeType kind = ast_node_get_type(node);
  switch (kind) {
    // Literals are immediate operands and need no instruction
    case AST_NODE_NONE:
    case AST_NODE_UNIT_LIT:
      return unit_operand();
    case AST_NODE_INT_LIT:
      return int_operand(ast_node_get_value(node).int_value);
    case AST_NODE_DOUBLE_LIT:
      return double_operand(ast_node_get_value(node).double_value);
    case AST_NODE_BOOL_LIT:
      return bool_operand(ast_node_get_value(node).boolean_value);
    case AST_NODE_STRING_LIT:
      return string_operand((char*)ast_node_get_value(node).string_value);

    // Keep the last statement result and stop emitting after a return
    case AST_NODE_BLOCK: {
      IrOperand result = unit_operand();
      for (size_t i = 0;
           i < ast_node_get_child_count(node) && lower->block != IR_INVALID_ID;
           i++)
        result = lower_node(lower, lower_child(ctx, node, i));
      return result;
    }

    // Identifier uses load the slot assigned to their resolved declaration
    case AST_NODE_IDENT: {
      const SymtableData* data = lower_symbol(ctx, node);
      return lower_load(lower, data->ir_index, data->ir_type);
    }

    // Declarations and assignments evaluate their value before storing it
    case AST_NODE_VAL:
    case AST_NODE_VAR:
    case AST_NODE_ASSIGN: {
      bool declaration = kind != AST_NODE_ASSIGN;
      AstNode* ident = lower_child(
          ctx, node, declaration ? AST_VAL_IDENT : AST_ASSIGN_IDENT);
      IrOperand value = lower_node(
          lower, lower_child(ctx, node,
                             declaration ? AST_VAL_INIT : AST_ASSIGN_VALUE));
      if (lower->block != IR_INVALID_ID)
        lower_store(lower, lower_symbol(ctx, ident)->ir_index, value);
      return unit_operand();
    }

    // Preserve expression effects even when the function discards its value
    case AST_NODE_RETURN: {
      IrOperand result =
          lower_node(lower, lower_child(ctx, node, AST_RETURN_VALUE));
      if (lower->block != IR_INVALID_ID) {
        if (ctx->ir.functions.data[lower->function].return_type == IR_UNIT)
          result = unit_operand();
        emit_return(ctx, lower->function, lower->block, result);
        lower->block = IR_INVALID_ID;
      }
      return unit_operand();
    }
    case AST_NODE_IF:
      return lower_if(lower, node);
    case AST_NODE_AND:
    case AST_NODE_OR:
      return lower_logic(lower, node);
    case AST_NODE_WHILE:
      lower_while(lower, node);
      return unit_operand();
    case AST_NODE_FOR:
      lower_for(lower, node);
      return unit_operand();
    case AST_NODE_CALL:
      return lower_call(lower, node);

    // Most members are built-in calls; floor has its own IR instruction
    case AST_NODE_MEMBER: {
      IrOperand receiver =
          lower_node(lower, lower_child(ctx, node, AST_MEMBER_RECEIVER));
      if (lower->block == IR_INVALID_ID)
        return unit_operand();
      if (strcmp(ast_node_get_name(node), "floor") == 0)
        return add_unary(ctx, lower->function, lower->block, IR_INST_FLOOR,
                         IR_DOUBLE, receiver);
      const SymtableData* member = lower_symbol(ctx, node);
      return lower_emit_call(lower, (IrFunctionType)member->ir_index,
                             IR_INVALID_ID, member->ir_type, &receiver, 1);
    }

    // Convert non-string parts, then concatenate in source order
    case AST_NODE_INTERP_STRING: {
      IrOperand result = string_operand("");
      for (size_t i = 0; i < ast_node_get_child_count(node); i++) {
        AstNode* part = lower_child(ctx, node, i);
        IrOperand value = lower_node(lower, part);
        if (lower->block == IR_INVALID_ID)
          return unit_operand();
        if (ast_node_get_data_type(part) != AST_DATA_TYPE_STRING)
          value = lower_emit_call(lower, IR_FX_TO_STR, IR_INVALID_ID, IR_STRING,
                                  &value, 1);
        result = lower_binary(lower, IR_INST_CONCAT, IR_STRING, result, value);
      }
      return result;
    }

    // Consume the semantic conversion nodes instead of inferring new types
    case AST_NODE_NOT:
    case AST_NODE_NEG:
    case AST_NODE_INT_TO_DOUBLE: {
      IrOperand value =
          lower_node(lower, lower_child(ctx, node, AST_UNARY_OPERAND));
      if (lower->block == IR_INVALID_ID)
        return unit_operand();
      IrType type = ast_to_ir_data_type(ctx, ast_node_get_data_type(node));

      // Negative zero as the Double zero operand preserves the sign of zero
      if (kind == AST_NODE_NEG)
        return lower_binary(
            lower, IR_INST_SUB, type,
            type == IR_INT ? int_operand(0) : double_operand(-0.0), value);
      return add_unary(ctx, lower->function, lower->block,
                       kind == AST_NODE_NOT ? IR_INST_NOT : IR_INST_INT2DOUBLE,
                       type, value);
    }
    case AST_NODE_ADD:
    case AST_NODE_SUB:
    case AST_NODE_MUL:
    case AST_NODE_DIV:
    case AST_NODE_EQ:
    case AST_NODE_NEQ:
    case AST_NODE_LT:
    case AST_NODE_LE:
    case AST_NODE_GT:
    case AST_NODE_GE: {
      // Separate C statements guarantee left-to-right operand evaluation
      IrOperand left =
          lower_node(lower, lower_child(ctx, node, AST_BINARY_LEFT));
      IrOperand right =
          lower_node(lower, lower_child(ctx, node, AST_BINARY_RIGHT));
      return lower_binary(
          lower, binary_kind(ctx, node),
          ast_to_ir_data_type(ctx, ast_node_get_data_type(node)), left, right);
    }
    default:
      THROW(E_INTERNAL_ERROR, "Unsupported checked AST node in lowering");
  }
  return unit_operand();
}
