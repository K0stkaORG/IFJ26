/**
 * @file test_ctx.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 9. 10. 2026
 *
 * @brief Unit test for the shared memory and error context
 */

#define SNOW_ENABLED 1
#include "snow/snow.h"

#include "ctx.h"

#include <stdbool.h>

/**
 * @brief Dummy function, that does not throw any error
 */
static void function_without_error(void) {}

/**
 * @brief Dummy function, that throws E_SEMANTIC_OTHER_ERROR
 *
 * @param[in,out] ctx Pointer to the context
 */
static void function_with_generic_error(Ctx* ctx) {
  THROW(E_SEMANTIC_OTHER_ERROR, "test error");
}

/**
 * @brief Dummy function, that throws E_SEMANTIC_UNDEFINED_ERROR
 *
 * @param[in,out] ctx Pointer to the context
 */
static void function_with_specific_error(Ctx* ctx) {
  THROW(E_SEMANTIC_UNDEFINED_ERROR, "test error");
}

/**
 * @brief Dummy function, that calls function_with_generic_error inside a TRY
 * block
 *
 * @param[in,out] ctx Pointer to the context
 */
static void function_with_failing_try(Ctx* ctx) {
  TRY(function_with_generic_error(ctx));

  fail("should not reach here");
}

/**
 * @brief Dummy function, that calls function_with_failing_try inside a TRY
 * block
 *
 * @param[in,out] ctx Pointer to the context
 */
static void function_with_nested_failing_try(Ctx* ctx) {
  TRY(function_with_failing_try(ctx));
}

/**
 * @brief Dummy function, that calls function_without_error inside a TRY block
 *
 * @param[in,out] ctx Pointer to the context
 * @param[out] reached Pointer to a boolean flag, that is set to true if the
 * function reaches the end
 */
static void function_with_passing_try(Ctx* ctx, bool* reached) {
  TRY(function_without_error());

  *reached = true;
}

describe(ctx) {
  Ctx context;
  Ctx* ctx = &context;

  it("initializes correctly") {
    ctx_init(ctx);

    asserteq(ctx->exit_code, E_OK);
  }

  before_each() {
    ctx_init(ctx);
  }

  subdesc(THROW) {
    it("correctly throws error") {
      function_with_generic_error(ctx);

      asserteq(ctx->exit_code, E_SEMANTIC_OTHER_ERROR);
    }

    it("correctly overrides less specific error") {
      function_with_generic_error(ctx);
      function_with_specific_error(ctx);

      asserteq(ctx->exit_code, E_SEMANTIC_UNDEFINED_ERROR);
    }

    it("does not override specific error with a generic one") {
      function_with_specific_error(ctx);
      function_with_generic_error(ctx);

      asserteq(ctx->exit_code, E_SEMANTIC_UNDEFINED_ERROR);
    }
  }

  subdesc(TRY) {
    it("propagates on THROW") {
      function_with_failing_try(ctx);

      asserteq(ctx->exit_code, E_SEMANTIC_OTHER_ERROR);
    }

    it("propagates on a nested THROW") {
      function_with_nested_failing_try(ctx);

      asserteq(ctx->exit_code, E_SEMANTIC_OTHER_ERROR);
    }

    it("does not propagate if no error is thrown") {
      bool reached = false;

      function_with_passing_try(ctx, &reached);

      assert(reached);
      asserteq(ctx->exit_code, E_OK);
    }
  }
}

snow_main();
