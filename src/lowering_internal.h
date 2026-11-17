/**
 * @file lowering_internal.h
 * Project: IFJ26
 *
 * @brief Shared state and helpers internal to the lowering pass
 *
 * Only lowering implementation files should include this header.
 */
#ifndef LOWERING_INTERNAL_H
#define LOWERING_INTERNAL_H

#include "ast.h"
#include "ir_helpers.h"
#include "lowering_helpers.h"
#include "symtable.h"

/**
 * @brief State of the current function while lowering its AST
 *
 * Keeps IDs instead of pointers because adding IR objects may move arrays.
 * IR_INVALID_ID as the current block means the path has already returned.
 */
typedef struct {
  // Shared allocation, error, AST and IR context
  Ctx* ctx;

  // Function currently being lowered
  IrFunctionId function;

  // Current block, or IR_INVALID_ID when the path is closed
  IrBlockId block;
} Lower;

/**
 * @brief Gets an AST child by its index
 *
 * @param ctx The shared context
 * @param node The parent node
 * @param index Index of the child
 * @return The child node
 */
AstNode* lower_child(Ctx* ctx, AstNode* node, size_t index);

/**
 * @brief Gets the resolved symbol of an AST identifier
 *
 * @param ctx The shared context
 * @param ident The checked identifier or member node
 * @return The semantic symbol, owned by its scope
 */
const SymtableData* lower_symbol(Ctx* ctx, AstNode* ident);

/**
 * @brief Adds a block to the current IR function
 *
 * @param lower The lowering state
 * @return ID of the new block
 */
IrBlockId lower_new_block(Lower* lower);

/**
 * @brief Ends the current block with an unconditional jump
 *
 * @param lower The lowering state
 * @param target ID of the target block
 */
void lower_jump(Lower* lower, IrBlockId target);

/**
 * @brief Ends the current block with a conditional jump
 *
 * @param lower The lowering state
 * @param condition The Boolean condition
 * @param yes Target when the condition is true
 * @param no Target when the condition is false
 */
void lower_branch(Lower* lower,
                  IrOperand condition,
                  IrBlockId yes,
                  IrBlockId no);

/**
 * @brief Loads a stack slot into a new IR value
 *
 * @param lower The lowering state
 * @param slot ID of the slot to read
 * @param type Type of the slot
 * @return An operand referring to the loaded value
 */
IrOperand lower_load(Lower* lower, IrStackSlotId slot, IrType type);

/**
 * @brief Stores an operand into a stack slot
 *
 * @param lower The lowering state
 * @param slot ID of the target slot
 * @param value The operand to store
 */
void lower_store(Lower* lower, IrStackSlotId slot, IrOperand value);

/**
 * @brief Adds a binary operation if the current path is still open
 *
 * @param lower The lowering state
 * @param op The IR instruction type
 * @param type Type of the result
 * @param left The left operand
 * @param right The right operand
 * @return The result operand, or Unit if the path has returned
 */
IrOperand lower_binary(Lower* lower,
                       IrInstructionType op,
                       IrType type,
                       IrOperand left,
                       IrOperand right);

/**
 * @brief Adds a user or built-in function call to the current block
 *
 * @param lower The lowering state
 * @param type User function or built-in function kind
 * @param id User function ID, unused for built-ins
 * @param result_type Return type of the called function
 * @param args Arguments copied by the IR construction helper
 * @param count Number of arguments
 * @return The result operand, or Unit for a Unit-returning call
 */
IrOperand lower_emit_call(Lower* lower,
                          IrFunctionType type,
                          IrFunctionId id,
                          IrType result_type,
                          IrOperand* args,
                          size_t count);

/**
 * @brief Lowers a call and its arguments in source order
 *
 * @param lower The lowering state
 * @param node The checked CALL node
 * @return The result operand, or Unit if the call has no result
 */
IrOperand lower_call(Lower* lower, AstNode* node);

/**
 * @brief Lowers an if into separate arms and a continuation block
 *
 * @param lower The lowering state
 * @param node The checked IF node
 * @return The merged result operand, or Unit when no value is produced
 */
IrOperand lower_if(Lower* lower, AstNode* node);

/**
 * @brief Lowers AND or OR without eagerly evaluating the right operand
 *
 * @param lower The lowering state
 * @param node The checked AND or OR node
 * @return The Boolean result read from the shared result slot
 */
IrOperand lower_logic(Lower* lower, AstNode* node);

/**
 * @brief Lowers a while loop with a repeated condition and back edge
 *
 * @param lower The lowering state
 * @param node The checked WHILE node
 */
void lower_while(Lower* lower, AstNode* node);

/**
 * @brief Lowers a range-based for loop and its guards
 *
 * @param lower The lowering state
 * @param node The checked FOR node
 */
void lower_for(Lower* lower, AstNode* node);

/**
 * @brief Lowers an expression or statement and updates the current block
 *
 * @param lower The lowering state
 * @param node The checked AST node
 * @return The expression result, or Unit for a statement or closed path
 */
IrOperand lower_node(Lower* lower, AstNode* node);

#endif
