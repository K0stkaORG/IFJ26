/**
 * @file test_ir.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 1. 11. 2026
 *
 * @brief Unit test for the intermediate representation
 */

#define SNOW_ENABLED 1
#include "snow/snow.h"

#include "ir.h"

#include "helpers/mock_ir.h"  // generate_mock_ir

describe(ir) {
  static IrProgram ir;  // static to survive longjmp

  subdesc(ir_init) {
    it("initializes IR program to default values") {
      ir_init(&ir);

      assert(ir.entrypoint == IR_INVALID_ID);
      assert(ir.functions == NULL);
      assert(ir.function_count == 0);
    }
  }

  before_each() {
    ir_init(&ir);
  }

  subdesc(ir_free) {
    it("frees empty IR correctly") {
      ir_free(&ir);

      assert(ir.entrypoint == IR_INVALID_ID);
      assert(ir.functions == NULL);
      assert(ir.function_count == 0);
    }

    it("frees all allocated memory") {
      generate_mock_ir(&ir);

      ir_free(&ir);

      assert(ir.entrypoint == IR_INVALID_ID);
      assert(ir.functions == NULL);
      assert(ir.function_count == 0);
    }
  }

  after_each() {
    ir_free(&ir);
  }
}

snow_main();
