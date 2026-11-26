/**
 * @file ir_helpers.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 3. 11. 2026
 *
 * @brief Intermediate representation construction helpers
 */

#include "ir_helpers.h"

#include "array.h"
#include "ir.h"
#include "ir_free_helpers.h"

#include <stdbool.h>  // bool
#include <string.h>   // strlen, memcpy, size_t

/**
 * @brief Returns the class of an instruction
 *
 * @param[in] inst The instruction to get the class of
 * @return The class of the instruction
 */
IrInstructionClass ir_get_instruction_class(IrInstruction* instruction) {
  switch (instruction->type) {
    // Unary operations
    case IR_INST_NOT:
    case IR_INST_INT2DOUBLE:
    case IR_INST_FLOOR:
      return IR_INST_CLASS_UNARY;

    // Binary operations
    case IR_INST_ADD:
    case IR_INST_SUB:
    case IR_INST_MUL:
    case IR_INST_IDIV:
    case IR_INST_DIV:
    case IR_INST_EQ:
    case IR_INST_NE:
    case IR_INST_LT:
    case IR_INST_LE:
    case IR_INST_GT:
    case IR_INST_GE:
    case IR_INST_CONCAT:
      return IR_INST_CLASS_BINARY;

    // Store
    case IR_INST_STORE:
      return IR_INST_CLASS_STORE;

    // Load
    case IR_INST_LOAD:
      return IR_INST_CLASS_LOAD;

      // Function calls
    case IR_INST_CALL:
      return IR_INST_CLASS_CALL;
  }
}

/**
 * @brief Gets a function from the IR by its id
 *
 * Throws E_INTERNAL_ERROR if the function id is invalid.
 *
 * @param[in,out] ctx The shared context containing the IR
 * @param[in] function Id of the function to get
 * @return Pointer to the function
 */
static IrFunction* ir_get_function(Ctx* ctx, IrFunctionId function) {
  // Check for out of bounds function id
  if (function >= ctx->ir.functions.count)
    THROW(E_INTERNAL_ERROR, "Invalid IR function ID: %d", function);

  // Return the function pointer
  return &ctx->ir.functions.data[function];
}

char* copy_string(Ctx* ctx, const char* source) {
  // Check for NULL source
  if (source == NULL)
    THROW(E_INTERNAL_ERROR, "Cannot copy a NULL string");

  // Allocate memory for the string and copy the contents
  size_t size = strlen(source) + 1;
  char* copy = TRY_MALLOC(size);
  memcpy(copy, source, size);

  // Return the copied string
  return copy;
}

IrOperand ir_copy_operand(Ctx* ctx, IrOperand operand) {
  if (operand.type < IR_OP_VALUE || operand.type > IR_OP_UNIT)
    THROW(E_INTERNAL_ERROR, "Invalid IR operand type: %d", operand.type);

  if (operand.type == IR_OP_STRING)
    operand.value.string_value = copy_string(ctx, operand.value.string_value);

  return operand;
}

IrFunctionId ir_add_function(Ctx* ctx, const char* name, IrType return_type) {
  // Validate the function name and return type
  if (name == NULL || return_type < IR_INT || return_type > IR_UNIT)
    THROW(E_INTERNAL_ERROR, "Invalid IR function name or return type");

  IrProgram* ir = &ctx->ir;

  // Reserve space for the new function (so appending cannot fail)
  array_reserve(ctx, &ir->functions, ir->functions.count + 1);

  // Create the new function
  IrFunctionId id = (IrFunctionId)ir->functions.count;
  IrFunction function = {.name = copy_string(ctx, name),
                         .return_type = return_type,
                         .entrypoint = IR_INVALID_ID};

  // Append the new function to the program
  array_append(ctx, &ir->functions, function);

  // Return the function id
  return id;
}

IrStackSlotId ir_add_slot(Ctx* ctx,
                          IrFunctionId function,
                          const char* name,
                          IrType type) {
  // Get the function pointer
  IrFunction* fx = ir_get_function(ctx, function);

  // Validate the slot name and type
  if (name == NULL || type < IR_INT || type > IR_UNIT)
    THROW(E_INTERNAL_ERROR, "Invalid IR slot name or type");

  // Reserve space for the new slot (so appending cannot fail)
  array_reserve(ctx, &fx->slots, fx->slots.count + 1);

  // Create the slot
  IrStackSlotId id = (IrStackSlotId)fx->slots.count;
  IrStackSlot slot = {
      .name = copy_string(ctx, name), .type = type, .lastStore = NULL};

  // Append the slot to the function's slot array
  array_append(ctx, &fx->slots, slot);

  // Return the slot id
  return id;
}

IrStackSlotId ir_add_parameter(Ctx* ctx,
                               IrFunctionId function,
                               const char* name,
                               IrType type) {
  // Get the function pointer
  IrFunction* fx = ir_get_function(ctx, function);

  // Check that there are no local slots
  if (fx->slots.count != fx->parameter_count)
    THROW(E_INTERNAL_ERROR, "IR parameters must be added before local slots");

  // Add the parameter
  IrStackSlotId id = ir_add_slot(ctx, function, name, type);
  fx->parameter_count++;

  // Return the parameter id
  return id;
}

IrBlockId ir_add_block(Ctx* ctx, IrFunctionId function) {
  // Get the function pointer
  IrFunction* fx = ir_get_function(ctx, function);

  // Create a new block
  IrBlockId id = (IrBlockId)fx->blocks.count;
  IrBlock block = {.terminator = {.type = IR_TERMINATOR_JUMP,
                                  .value.jump_target = IR_INVALID_ID}};

  // Append the block to the function's block array
  array_append(ctx, &fx->blocks, block);

  // Return the block id
  return id;
}

IrValueId ir_add_instruction(
    Ctx* ctx,
    IrFunctionId function,  // NOLINT(bugprone-easily-swappable-parameters)
    IrBlockId block,
    IrInstruction instruction) {
  // Get the function pointer
  IrFunction* fx = ir_get_function(ctx, function);

  // Check for invalid block id
  if (block >= fx->blocks.count)
    THROW(E_INTERNAL_ERROR, "Invalid IR block ID: %d", block);

  // Check for invalid instruction type
  if (instruction.type < IR_INST_ADD || instruction.type > IR_INST_CALL)
    THROW(E_INTERNAL_ERROR, "Invalid IR instruction type: %d",
          instruction.type);

  // Check for invalid result type
  if (instruction.type != IR_INST_STORE &&
      (instruction.result_type < IR_INT || instruction.result_type > IR_UNIT))
    THROW(E_INTERNAL_ERROR, "Invalid IR instruction result type");

  bool has_result =
      instruction.type != IR_INST_STORE && instruction.result_type != IR_UNIT;

  // Check for invalid operands (IR_INST_CALL)
  // AI generated by: Antigravity / Gemini 4 Hydrogen
  // Prompt: "Apply the fix to change upper bound in ir_add_instruction to
  // reflect the enum changes." Changes: Update IrFunctionType upper bound from
  // IR_FX_CHR to IR_FX_LIST_LEN.
  if (instruction.type == IR_INST_CALL)
    if ((instruction.operands.call.arg_count < 0 ||
         (instruction.operands.call.arg_count > 0 &&
          instruction.operands.call.args == NULL) ||
         instruction.operands.call.type < IR_FX_USER ||
         instruction.operands.call.type > IR_FX_LIST_LEN))
      THROW(E_INTERNAL_ERROR, "Invalid IR call arguments or function type");

  IrBlock* target = &fx->blocks.data[block];

  // Initialize the empty instruction
  IrInstruction new = {
      .type = instruction.type,
      .result_type = has_result ? instruction.result_type : IR_UNIT,
      .result = IR_INVALID_ID};

  // Append the empty instruction to the block (so ir_free can clean it up if
  // the copy fails)
  array_append(ctx, &target->instructions, new);

  IrInstruction* ptr =
      &target->instructions.data[target->instructions.count - 1];

  // Copy the instruction operands based on the instruction type
  switch (instruction.type) {
    case IR_INST_FLOOR:
    case IR_INST_NOT:
    case IR_INST_INT2DOUBLE:
      ptr->operands.unary.operand =
          ir_copy_operand(ctx, instruction.operands.unary.operand);
      break;

    case IR_INST_LOAD:
      ptr->operands.load = instruction.operands.load;
      break;

    case IR_INST_STORE:
      ptr->operands.store.target = instruction.operands.store.target;
      ptr->operands.store.source =
          ir_copy_operand(ctx, instruction.operands.store.source);
      break;

    case IR_INST_CALL: {
      ptr->operands.call.type = instruction.operands.call.type;
      ptr->operands.call.id = instruction.operands.call.id;

      // If there are arguments
      size_t count = instruction.operands.call.arg_count;
      if (count > 0) {
        // Allocate memory for the arguments array
        ptr->operands.call.args = TRY_MALLOC(count * sizeof(IrOperand));

        // and allocate memory for each argument
        for (size_t i = 0; i < count; i++) {
          ptr->operands.call.args[i] =
              ir_copy_operand(ctx, instruction.operands.call.args[i]);

          // making sure the argument count is incremented only after successful
          // copy, meaning in case of subsequent failure, ir_free does not
          // access uninitialized memory
          ptr->operands.call.arg_count++;
        }
      }
      break;
    }

    default:
      ptr->operands.binary.left =
          ir_copy_operand(ctx, instruction.operands.binary.left);
      ptr->operands.binary.right =
          ir_copy_operand(ctx, instruction.operands.binary.right);
      break;
  }

  // Get the next available value id if the instruction has a result
  // AI generated by: Antigravity / Gemini 4 Hydrogen
  // Prompt: "Okay, let's do it. Store value_types directly in IrFunction (use
  // macros from ctx correctly). Update helpers. In codegen, pass IrFunction*
  // into cg_print_builtin and replace the inline TYPE checks with switch-case."
  // Changes: Record each allocated IrValueId's result_type in fx->value_types.
  if (has_result) {
    ptr->result = fx->value_count++;
    array_append(ctx, &fx->value_types, ptr->result_type);
  }

  // Return the result value id
  return ptr->result;
}

void ir_set_terminator(Ctx* ctx,
                       // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
                       IrFunctionId function,
                       IrBlockId block,
                       IrBlockTerminator terminator) {
  // Get the function pointer
  IrFunction* fx = ir_get_function(ctx, function);

  // Check for invalid block id
  if (block >= fx->blocks.count)
    THROW(E_INTERNAL_ERROR, "Invalid IR block ID: %d", block);

  // Create a copy of the terminator operand (so that if allocation fails, the
  // original is not corrupted)
  switch (terminator.type) {
    case IR_TERMINATOR_JUMP:
      break;
    case IR_TERMINATOR_CONDITIONAL:
      terminator.value.conditional_target.condition =
          ir_copy_operand(ctx, terminator.value.conditional_target.condition);
      break;
    case IR_TERMINATOR_RETURN:
      terminator.value.return_value =
          ir_copy_operand(ctx, terminator.value.return_value);
      break;
    case IR_TERMINATOR_EXIT:
      terminator.value.exit_code =
          ir_copy_operand(ctx, terminator.value.exit_code);
      break;
    default:
      THROW(E_INTERNAL_ERROR, "Invalid IR terminator type: %d",
            terminator.type);
      return;
  }

  // Free the previous terminator operand (if any)
  IrBlockTerminator* previous = &fx->blocks.data[block].terminator;
  switch (previous->type) {
    case IR_TERMINATOR_CONDITIONAL:
      ir_free_operand(&previous->value.conditional_target.condition);
      break;
    case IR_TERMINATOR_RETURN:
      ir_free_operand(&previous->value.return_value);
      break;
    case IR_TERMINATOR_EXIT:
      ir_free_operand(&previous->value.exit_code);
      break;
    default:
      break;
  }

  // Set the new terminator
  *previous = terminator;
}
