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

#include <stdbool.h>  // bool, true, false
#include <stdlib.h>   // malloc, free

#define TEST_PATTERN "123456789"
#define TEST_PATTERN_PREFIX "12345"

static void function_without_error(void) {}

static void function_with_generic_error(Ctx* ctx) {
  THROW(E_SEMANTIC_OTHER_ERROR, "test error");
}

static void function_with_specific_error(Ctx* ctx) {
  THROW(E_SEMANTIC_UNDEFINED_ERROR, "test error");
}

static void function_with_failing_try(Ctx* ctx) {
  TRY(function_with_generic_error(ctx));

  fail("should not reach here");
}

static void function_with_nested_failing_try(Ctx* ctx) {
  TRY(function_with_failing_try(ctx));
}

static void function_with_passing_try(Ctx* ctx, bool* reached) {
  TRY(function_without_error());

  *reached = true;
}

static void attempt_ctx_malloc(Ctx* ctx, bool* reached) {
  ctx->buffer = TRY_MALLOC(sizeof(char) * 10);

  *reached = true;
}

static void attempt_ctx_realloc(Ctx* ctx, bool* reached, int size) {
  TRY_REALLOC(ctx->buffer, sizeof(char) * size);

  *reached = true;
}

static void allocate_buffer_helper(Ctx* ctx) {
  ctx->buffer = malloc(sizeof(char) * 10);

  // Check for unsuccessful allocation, as that would void the test result
  assertneq(ctx->buffer, NULL,
            "Failed to allocate memory required to conduct the unit test");

  for (int i = 0; i < 9; i++)
    ctx->buffer[i] = TEST_PATTERN[i];

  ctx->buffer[9] = '\0';
}

describe(ctx) {
  Ctx context;
  Ctx* ctx = &context;

  it("initializes correctly") {
    ctx_init(ctx);

    asserteq(ctx->exit_code, E_OK);
    asserteq(ctx->flags.forceNthMallocFail, 0);
    asserteq(ctx->buffer, NULL);
  }

  before_each() {
    ctx_init(ctx);
  }

  if ("correctly frees without any allocated memory") {
    ctx_free(ctx);

    asserteq(ctx->buffer, NULL);
  }

  it("correctly frees with allocated memory") {
    allocate_buffer_helper(ctx);

    ctx_free(ctx);

    asserteq(ctx->buffer, NULL);

    // Leaks will be detected by the test runner
  }

  after_each() {
    ctx_free(ctx);
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

  subdesc(TRY_MALLOC) {
    it("allocates memory correctly") {
      bool reached = false;
      attempt_ctx_malloc(ctx, &reached);

      assert(reached);
      asserteq(ctx->exit_code, E_OK);
      assert(ctx->buffer != NULL);
    }

    it("throws on allocation failure") {
      ctx->flags.forceNthMallocFail = 1;

      bool reached = false;
      attempt_ctx_malloc(ctx, &reached);

      assert(!reached);
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
      asserteq(ctx->buffer, NULL);
    }
  }

  subdesc(TRY_REALLOC) {
    it("reallocates memory correctly (increasing size)") {
      allocate_buffer_helper(ctx);

      bool reached = false;
      attempt_ctx_realloc(ctx, &reached, 20);

      assert(reached);
      asserteq(ctx->exit_code, E_OK);
      asserteq_str(ctx->buffer, TEST_PATTERN);

      ctx->buffer[19] = '0';
      assert(ctx->buffer[19] == '0');
    }

    it("reallocates memory correctly (decreasing size)") {
      allocate_buffer_helper(ctx);

      bool reached = false;
      attempt_ctx_realloc(ctx, &reached, 5);

      assert(reached);
      asserteq(ctx->exit_code, E_OK);

      for (int i = 0; i < 5; i++)
        assert(ctx->buffer[i] == TEST_PATTERN[i]);
    }

    it("throws on reallocation failure") {
      allocate_buffer_helper(ctx);

      ctx->flags.forceNthMallocFail = 1;

      bool reached = false;
      attempt_ctx_realloc(ctx, &reached, 20);

      assert(!reached);
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
    }

    it("keeps buffer contents after failure") {
      allocate_buffer_helper(ctx);

      char* oldBuffer = ctx->buffer;

      ctx->flags.forceNthMallocFail = 1;

      bool reached = false;
      attempt_ctx_realloc(ctx, &reached, 20);

      assert(!reached);
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
      asserteq_str(ctx->buffer, TEST_PATTERN);
      asserteq_ptr(ctx->buffer, oldBuffer);
    }

    it("does not realloc NULL pointer") {
      bool reached = false;
      attempt_ctx_realloc(ctx, &reached, 20);

      assert(!reached);
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
      asserteq_ptr(ctx->buffer, NULL);
    }
  }
}

snow_main();
