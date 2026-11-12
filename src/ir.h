/**
 * @file ir.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 4. 10. 2026
 *
 * @brief Intermediate Representation and helper functions
 *
 * This file contains the declarations of the intermediate representation
 * and helper functions used when working with the IR.
 */

#ifndef IR_H
#define IR_H

#include "array.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int32_t, SIZE_MAX

/**
 * @brief Id of a value in the IR
 *
 * Values are any intermediate values the program computes during execution. Ids
 * are local to the function which contains the value.
 */
typedef size_t IrValueId;

/**
 * @brief Id of a stack slot in the IR
 *
 * Stack slots are function parameters + local variables in the IR. Ids are
 * local to the function which contains the slot. IDs 0 - (parameter_count - 1)
 * are function parameters, IDs parameter_count - (slots.count - 1) are local
 * variables
 */
typedef size_t IrStackSlotId;

/**
 * @brief Id of a block in the IR
 *
 * Blocks are sequences of instructions in the IR with a terminator at the end.
 * Ids are local to the function which contains the block.
 */
typedef size_t IrBlockId;

/**
 * @brief Id of a function in the IR
 *
 * Function ids are global across the whole program.
 */
typedef size_t IrFunctionId;

/**
 * @brief Invalid id for a value, slot, block, or function in the IR
 */
#define IR_INVALID_ID SIZE_MAX

/**
 * @brief Integer type used in the IR
 *
 * Even though IFJ26Code works with 64bit integers, we are supposed to pretend
 * its 32bit.
 */
typedef int32_t IrInt;

/**
 * @brief Double type used in the IR
 */
typedef double IrDouble;

/**
 * @brief String type used in the IR
 */
typedef char* IrString;

/**
 * @brief Bool type used in the IR
 */
typedef bool IrBool;

/**
 * @brief IR type
 *
 * Type a variable can have/function can return in the IR
 */
typedef enum {
  IR_INT,
  IR_DOUBLE,
  IR_STRING,
  IR_BOOL,
  IR_UNIT,
} IrType;

/**
 * @brief IR operand type
 *
 * Type of the operand in an IR instruction.
 */
typedef enum {
  IR_OP_VALUE,
  IR_OP_INT32,
  IR_OP_DOUBLE,
  IR_OP_STRING,
  IR_OP_BOOL,
  IR_OP_UNIT,
} IrOperandType;

/**
 * @brief IR operand
 *
 * Represents an operand in an IR instruction.
 */
typedef struct {
  // Type of the operand
  IrOperandType type;

  // Value of the operand
  union {
    // Id of the value in the IR
    IrValueId id;

    // Constant value
    IrInt int32_value;
    IrDouble double_value;
    IrString string_value;
    IrBool bool_value;
  } value;
} IrOperand;

/**
 * @brief IR function type
 *
 * Type of a function in the IR. It's either a function defined somewhere in the
 * IR or a specific built-in function.
 */
typedef enum {
  // User-defined function
  IR_FX_USER,

  // Built-in functions:

  // print(any) -> IrUnit
  IR_FX_PRINT,

  // readLine() -> IrString
  IR_FX_READ_LINE,

  // readInt() -> IrInt
  IR_FX_READ_INT,

  // readBoolean() -> IrBool
  IR_FX_READ_BOOL,

  // readDouble() -> IrDouble
  IR_FX_READ_DOUBLE,

  // toInt() -> IrInt
  IR_FX_TO_INT,

  // toStr() -> IrString
  IR_FX_TO_STR,

  // strLen(IrString) -> IrInt
  IR_FX_STR_LEN,

  // strSub(IrString, IrInt, IrInt) -> IrString
  IR_FX_STR_SUB,

  // ord(IrString, IrInt) -> IrInt
  IR_FX_ORD,

  // chr(IrInt) -> IrString
  IR_FX_CHR,
} IrFunctionType;

/**
 * @brief IR instruction type
 *
 * Type of an IR instruction.
 */
typedef enum {
  // Arithmetic operations
  IR_INST_ADD,
  IR_INST_SUB,
  IR_INST_MUL,
  IR_INST_IDIV,
  IR_INST_DIV,
  IR_INST_FLOOR,

  // Comparison operations
  IR_INST_EQ,
  IR_INST_NE,
  IR_INST_LT,
  IR_INST_LE,
  IR_INST_GT,
  IR_INST_GE,

  // Logical operations
  IR_INST_NOT,

  // Type conversion
  IR_INST_INT2DOUBLE,

  // String operations
  IR_INST_CONCAT,

  // Variable operations
  IR_INST_LOAD,
  IR_INST_STORE,

  // Function calls
  IR_INST_CALL,
} IrInstructionType;

/**
 * @brief Enum representing the class of an instruction
 *
 * This differentiates instructions based on their operands type.
 */
typedef enum {
  IR_INST_CLASS_UNARY,
  IR_INST_CLASS_BINARY,
  IR_INST_CLASS_STORE,
  IR_INST_CLASS_LOAD,
  IR_INST_CLASS_CALL,
} IrInstructionClass;

/**
 * @brief IR instruction
 *
 * Represents a single instruction in the IR.
 */
typedef struct {
  // Type of the instruction
  IrInstructionType type;

  // Operands of the instruction
  union {
    // Operands of a unary instruction (not, int2double)
    struct {
      IrOperand operand;
    } unary;

    // Operands of a binary instruction (add, sub, concat, etc.)
    struct {
      IrOperand left;
      IrOperand right;
    } binary;

    // Operands of a load instruction
    struct {
      IrStackSlotId source;
    } load;

    // Operands of a store instruction
    struct {
      IrOperand source;
      IrStackSlotId target;
    } store;

    // Operands of a call instruction
    struct {
      // Type of the function
      IrFunctionType type;

      // Id of the function in the IR (valid only for IR_FX_USER)
      IrFunctionId id;

      // Arguments of the call
      IrOperand* args;
      size_t arg_count;
    } call;
  } operands;

  // Result of the instruction
  IrType result_type;  // undefined if the instruction does not have a result
  IrValueId result;  // IR_INVALID_ID if the instruction does not have a result
} IrInstruction;

/**
 * @brief IR block terminator type
 *
 * Type of the terminator of an IR block.
 */
typedef enum {
  // Unconditional jump to another block
  IR_TERMINATOR_JUMP,

  // Conditional jump to another block
  IR_TERMINATOR_CONDITIONAL,

  // Return from the function
  IR_TERMINATOR_RETURN,

  // Exit the program
  IR_TERMINATOR_EXIT,
} IrBlockTerminatorType;

/**
 * @brief IR block terminator
 *
 * Represents the terminator of an IR block.
 */
typedef struct {
  // Type of the terminator
  IrBlockTerminatorType type;

  // Value of the terminator
  union {
    // Target of the IR_TERMINATOR_JUMP
    IrBlockId jump_target;

    // Condition + true/false targets of the IR_TERMINATOR_CONDITIONAL
    struct {
      IrOperand condition;
      IrBlockId true_target;
      IrBlockId false_target;
    } conditional_target;

    // Value of the return value
    IrOperand return_value;

    // Value of the exit code
    IrOperand exit_code;
  } value;
} IrBlockTerminator;

/**
 * @brief Array of IR instructions
 */
ARRAY_TYPE(IrInstructionArray, IrInstruction);

/**
 * @brief IR block
 *
 * Represents a block of instructions in the IR. Always has exactly one
 * terminator.
 */
typedef struct {
  // Instructions in the block
  IrInstructionArray instructions;

  // Terminator of the block
  IrBlockTerminator terminator;
} IrBlock;

/**
 * @brief Piece of memory on the stack in the IR
 *
 * Represents a piece of memory on the stack in the IR like a local variable or
 * function parameter.
 */
typedef struct {
  // Name of the stack slot
  char* name;

  // Type of the stack slot
  IrType type;
} IrStackSlot;

/**
 * @brief Array of IR stack slots
 */
ARRAY_TYPE(IrStackSlotArray, IrStackSlot);

/**
 * @brief Array of IR blocks
 */
ARRAY_TYPE(IrBlockArray, IrBlock);

/**
 * @brief IR for a single function
 *
 * Represents a single function in the IR.
 */
typedef struct {
  // Name of the function
  char* name;

  // Parameters and local variables of the function
  IrStackSlotArray slots;  // First parameter_count slots are parameters, rest
                           // are local variables
  size_t parameter_count;

  // Return type of the function
  IrType return_type;

  // Entrypoint of the function
  IrBlockId entrypoint;

  // Blocks of the function
  IrBlockArray blocks;

  // Number of values in the function (also the next available value id)
  size_t value_count;
} IrFunction;

/**
 * @brief Array of IR functions
 */
ARRAY_TYPE(IrFunctionArray, IrFunction);

/**
 * @brief IR for entire program
 *
 * Represents the entire program in the IR. It is composed of multiple functions
 * with one being the entrypoint.
 */
typedef struct {
  // Entrypoint of the program
  IrFunctionId entrypoint;

  // Array of all functions in the program
  IrFunctionArray functions;
} IrProgram;

/**
 * @brief Initializes the IR struct
 *
 * @param[out] ir Pointer to the IR struct to initialize
 */
void ir_init(IrProgram* ir);

/**
 * @brief Frees the IR struct and resets it to default values
 *
 * @param[in,out] ir Pointer to the IR struct to free
 */
void ir_free(IrProgram* ir);

#endif
