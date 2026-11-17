/**
 * @file lowering_helpers.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 11. 11. 2026
 *
 * @brief Helper functions for lowering the AST to IR
 *
 * This file contains implementation of helper functions used for lowering the
 * AST to IR.
 */

#include "lowering_helpers.h"

#include "ast.h"
#include "ir.h"
#include "ir_helpers.h"

IrType ast_to_ir_data_type(Ctx* ctx, AstDataType data_type) {
  switch (data_type) {
    case AST_DATA_TYPE_INT:
      return IR_INT;
    case AST_DATA_TYPE_DOUBLE:
      return IR_DOUBLE;
    case AST_DATA_TYPE_BOOLEAN:
      return IR_BOOL;
    case AST_DATA_TYPE_STRING:
      return IR_STRING;
    case AST_DATA_TYPE_UNIT:
      return IR_UNIT;

    default:
      THROW(E_INTERNAL_ERROR, "Unknown AST data type");
      return IR_UNIT;  // Unreachable
  }
}

IrType ast_get_function_return_type(Ctx* ctx, AstNode* function) {
  // Read the final return type from the function symbol, including inference
  AstNode* ident;
  ast_node_get_child(ctx, function, AST_DEF_IDENT, &ident);
  const SymtableData* symbol = ast_node_get_symbol(ident);
  // Lowering must run after semantic analysis has resolved the function
  if (symbol == NULL || symbol->symbol_type != SYMBOL_FUNCTION)
    THROW(E_INTERNAL_ERROR, "Lowering requires a checked function");
  return symbol->ir_type;
}

const char* ast_get_node_identifier(Ctx* ctx, AstNode* node) {
  // Check that node is not NULL
  if (node == NULL)
    THROW(E_INTERNAL_ERROR, "Attempted to get identifier of NULL node");

  switch (ast_node_get_type(node)) {
    // We have reached a valid identifier node, return its name
    case AST_NODE_IDENT:
      return ast_node_get_name(node);

    // Functions and parameters both keep their name in an identifier child
    case AST_NODE_DEF:
    case AST_NODE_PARAM: {
      AstNode* ident_child;
      ast_node_get_child(ctx, node, ast_node_get_type(node) == AST_NODE_PARAM
                                       ? AST_PARAM_IDENT : AST_DEF_IDENT,
                         &ident_child);

      return ast_get_node_identifier(ctx, ident_child);
    }

    default:
      THROW(E_INTERNAL_ERROR, "Failed to get identifier of AST node");
      return NULL;  // Unreachable
  }
}

IrOperand value_operand(IrValueId value) {
  return (IrOperand){IR_OP_VALUE, .value.id = value};
}

IrOperand int_operand(IrInt value) {
  return (IrOperand){IR_OP_INT32, .value.int32_value = value};
}

IrOperand double_operand(IrDouble value) {
  return (IrOperand){IR_OP_DOUBLE, .value.double_value = value};
}

IrOperand string_operand(IrString value) {
  return (IrOperand){IR_OP_STRING, .value.string_value = value};
}

IrOperand bool_operand(IrBool value) {
  return (IrOperand){IR_OP_BOOL, .value.bool_value = value};
}

IrOperand unit_operand(void) {
  return (IrOperand){IR_OP_UNIT, .value.id = IR_INVALID_ID};
}

IrOperand add_unary(Ctx* ctx,
                    IrFunctionId function_id,
                    IrBlockId block_id,
                    IrInstructionType instruction_type,
                    IrType result_type,
                    IrOperand operand) {
  // Add the instruction and wrap its automatically assigned result ID
  IrValueId result =
      ir_add_instruction(ctx, function_id, block_id,
                         (IrInstruction){.type = instruction_type,
                                         .result_type = result_type,
                                         .operands.unary.operand = operand});

  return value_operand(result);
}

IrOperand add_binary(Ctx* ctx,
                     IrFunctionId function_id,
                     IrBlockId block_id,
                     IrInstructionType instruction_type,
                     IrType result_type,
                     IrOperand left,
                     IrOperand right) {
  // Add the instruction and wrap its automatically assigned result ID
  IrValueId result =
      ir_add_instruction(ctx, function_id, block_id,
                         (IrInstruction){.type = instruction_type,
                                         .result_type = result_type,
                                         .operands.binary.left = left,
                                         .operands.binary.right = right});

  return value_operand(result);
}

void emit_return(Ctx* ctx, IrFunctionId function_id, IrBlockId block_id,
                 IrOperand value) {
  // Finish the block with the supplied return operand
  ir_set_terminator(ctx, function_id, block_id,
                    (IrBlockTerminator){.type = IR_TERMINATOR_RETURN,
                                        .value.return_value = value});
}
