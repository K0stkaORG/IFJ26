/**
 * @file ir.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 1. 11. 2026
 *
 * @brief Intermediate representation initialization and cleanup
 *
 * This file contains functions for initializing and cleaning up the
 * intermediate representation of the program.
 */

#include "ir.h"
#include "array.h"
#include "ir_free_helpers.h"

void ir_init(IrProgram* ir) {
  // Set IR to default values
  ir->entrypoint = IR_INVALID_ID;
  array_init(&ir->functions);
}

void ir_free(IrProgram* ir) {
  // Check the pointer is valid
  if (ir->functions.data == NULL)
    return;

  // Free contents of each function
  for (IrFunctionId fid = 0; fid < ir->functions.count; fid++)
    ir_free_function(&ir->functions.data[fid]);

  // Free functions array
  array_free(&ir->functions);

  // Reset entrypoint
  ir->entrypoint = IR_INVALID_ID;
}
