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

#include "ast.h"
#include "ctx.h"

#include <stdbool.h>  // bool, true, false
#include <stdlib.h>   // malloc, free

#define TEST_PATTERN "123456789"
#define TEST_PATTERN_PREFIX "12345"

static void function_with_error(Ctx* ctx) {
  THROW(E_LEXER_ERROR, "test error");
}

static void function_with_nested_error(Ctx* ctx) {
  function_with_error(ctx);
}

describe(Ctx) {
  subdesc(ctx_init) {
    it("initializes Ctx correctly") {
      Ctx test_ctx;

      ctx_init(&test_ctx, DEFAULT_FLAGS);

      asserteq(test_ctx.exit_code, E_OK);
      asserteq(test_ctx.flags.forceNthMallocFail, 0);
    }
  }

  subdesc(ctx_allocate) {}

  subdesc(ctx_free) {
    it("returns correct ExitCode") {
      Ctx test_ctx;

      ctx_init(&test_ctx, DEFAULT_FLAGS);

      test_ctx.exit_code = E_LEXER_ERROR;

      asserteq(ctx_free(&test_ctx), E_LEXER_ERROR);
    }

    it("starts without an AST") {
      Ctx test_ctx;

      ctx_init(&test_ctx, DEFAULT_FLAGS);

      assert(test_ctx.ast == NULL);
      asserteq(ctx_free(&test_ctx), E_OK);
    }

    it("frees the registered AST and is safe to call twice") {
      assertNoThrow({
        AstNode* root = NULL;
        AstNode* child = NULL;

        ast_node_create(ctx, &root);
        ctx->ast = root;
        ast_node_create(ctx, &child);
        ast_node_append_child(ctx, root, child);
      });

      asserteq(ctx_free(ctx), E_OK);
      assert(ctx->ast == NULL);
      asserteq(ctx_free(ctx), E_OK);
    }
  }

  subdesc(parse_cli_flags) {
    it("uses default flags when no options are provided") {
      Flags flags;
      char* argv[] = {"ifj26"};

      assert(parse_cli_args(1, argv, &flags));
      asserteq(flags.forceNthMallocFail, 0);
      assert(!flags.verbose);
    }

    it("parses the allocation failure index") {
      Flags flags;
      char* argv[] = {"ifj26", "--debugForceNthMallocFail", "4"};

      assert(parse_cli_args(3, argv, &flags));
      asserteq(flags.forceNthMallocFail, 4);
      assert(!flags.verbose);
    }

    it("parses the verbose option") {
      Flags flags;
      char* argv[] = {"ifj26", "--verbose"};

      assert(parse_cli_args(2, argv, &flags));
      asserteq(flags.forceNthMallocFail, 0);
      assert(flags.verbose);
    }

    it("parses the disable optimizations option") {
      Flags flags;
      char* argv[] = {"ifj26", "-o0"};

      assert(parse_cli_args(2, argv, &flags));
      assert(flags.disableOptimizations);
    }

    it("accepts supported options in either order") {
      Flags flags;
      char* verbose_first[] = {"ifj26", "--verbose",
                               "--debugForceNthMallocFail", "2"};
      char* fail_first[] = {"ifj26", "--debugForceNthMallocFail", "2",
                            "--verbose"};

      assert(parse_cli_args(4, verbose_first, &flags));
      asserteq(flags.forceNthMallocFail, 2);
      assert(flags.verbose);

      assert(parse_cli_args(4, fail_first, &flags));
      asserteq(flags.forceNthMallocFail, 2);
      assert(flags.verbose);
    }

    it("rejects a missing allocation failure index") {
      Flags flags;
      char* argv[] = {"ifj26", "--debugForceNthMallocFail"};

      assert(!parse_cli_args(2, argv, &flags));
    }

    it("rejects invalid allocation failure indices") {
      Flags flags;
      char* invalid_values[] = {"0", "-1", "not-a-number",
                                "999999999999999999999999999999999999"};

      for (size_t i = 0; i < 4; i++) {
        char* argv[] = {"ifj26", "--debugForceNthMallocFail",
                        invalid_values[i]};
        assert(!parse_cli_args(3, argv, &flags));
      }
    }

    it("rejects unknown options") {
      Flags flags;
      char* argv[] = {"ifj26", "--unknown"};

      assert(!parse_cli_args(2, argv, &flags));
    }
  }

  subdesc(THROW) {
    it("throws an error") {
      assertThrow(E_LEXER_ERROR, { THROW(E_LEXER_ERROR, "test error"); });
    };

    it("throws an error from a nested call") {
      assertThrow(E_LEXER_ERROR, { function_with_error(ctx); });
    }

    it("throws an error from a deeply nested call") {
      assertThrow(E_LEXER_ERROR, { function_with_nested_error(ctx); });
    }
  }

  subdesc(TRY_MALLOC) {
    static char* ptr;

    before_each() {
      ptr = NULL;
    }

    after_each() {
      free(ptr);
    }

    it("allocates without throwing") {
      bool reached = false;

      assertNoThrow({
        ptr = TRY_MALLOC(sizeof(char) * 10);

        reached = true;
      });

      assert(reached);
      assert(ptr != NULL);
    }

    it("throws on allocation failure") {
      ctx->flags.forceNthMallocFail = 1;

      assertThrow(E_INTERNAL_ERROR, {
        ptr = TRY_MALLOC(sizeof(char) * 10);

        fail("Allocation should have failed");
      });

      asserteq(ptr, NULL);
    }
  }

  subdesc(TRY_MALLOC_OR) {
    static char* ptr;

    before_each() {
      ptr = NULL;
    }

    after_each() {
      free(ptr);
    }

    it("allocates without throwing") {
      bool reached = false;

      assertNoThrow({
        ptr = TRY_MALLOC_OR(sizeof(char) * 10, {});

        reached = true;
      });

      assert(reached);
      assert(ptr != NULL);
    }

    it("does not run cleanup on successful allocation") {
      bool reached = false;

      assertNoThrow({
        ptr = TRY_MALLOC_OR(sizeof(char) * 10,
                            { fail("Cleanup should not be run"); });

        reached = true;
      });

      assert(reached);
      assert(ptr != NULL);
    }

    it("runs cleanup before jumping on allocation failure") {
      ctx->flags.forceNthMallocFail = 1;

      bool reached = false;

      assertThrow(E_INTERNAL_ERROR, {
        ptr = TRY_MALLOC_OR(sizeof(char) * 10, { reached = true; });

        fail("Allocation should have failed");
      });

      assert(reached);
      asserteq(ptr, NULL);
    }
  }

  subdesc(TRY_REALLOC) {
    static char* ptr;

    before_each() {
      // Allocate the buffer before each test
      ptr = malloc(sizeof(char) * 10);

      // Check for unsuccessful allocation, as that would void the test result
      assertneq(ptr, NULL,
                "Failed to allocate memory required to conduct the unit test");

      // Fill the buffer with test pattern
      for (int i = 0; i < 9; i++)
        ptr[i] = TEST_PATTERN[i];
      ptr[9] = '\0';
    }

    after_each() {
      // Free the buffer after each test
      free(ptr);
    }

    it("reallocates memory correctly (increasing size)") {
      bool reached = false;

      assertNoThrow({
        TRY_REALLOC(ptr, sizeof(char) * 20);

        reached = true;
      });

      assert(reached);
      asserteq_str(ptr, TEST_PATTERN);

      ptr[19] = '0';
      assert(ptr[19] == '0');
    }

    it("reallocates memory correctly (decreasing size)") {
      bool reached = false;

      assertNoThrow({
        TRY_REALLOC(ptr, sizeof(char) * 5);

        reached = true;
      });

      assert(reached);

      for (int i = 0; i < 5; i++)
        assert(ptr[i] == TEST_PATTERN[i]);
    }

    it("throws on reallocation failure") {
      ctx->flags.forceNthMallocFail = 1;

      assertThrow(E_INTERNAL_ERROR, {
        TRY_REALLOC(ptr, sizeof(char) * 20);

        fail("Realloc should have thrown an error");
      });
    }

    it("keeps buffer contents after failure") {
      char* oldBuffer = ptr;

      ctx->flags.forceNthMallocFail = 1;

      assertThrow(E_INTERNAL_ERROR, {
        TRY_REALLOC(ptr, sizeof(char) * 20);

        fail("Realloc should have thrown an error");
      });

      asserteq_str(ptr, TEST_PATTERN);
      asserteq_ptr(ptr, oldBuffer);
    }

    // Override so it doesn't allocate the buffer for the following test
    before_each() {
      ptr = NULL;
    }

    it("does not realloc NULL pointer") {
      assertThrow(E_INTERNAL_ERROR, {
        TRY_REALLOC(ptr, sizeof(char) * 20);

        fail("Realloc should have thrown an error");
      });

      asserteq_ptr(ptr, NULL);
    }
  }
}

snow_main();
