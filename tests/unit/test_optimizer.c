/**
 * @file test_optimizer.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Unit test for the optimizer context
 */

#define SNOW_ENABLED 1
#include "snow/snow.h"

#include "optimizer.h"

#include "helpers/mock_ir.h"  // generate_mock_ir

describe(optimizer) {
  subdesc(optimizer_init) {
    it("initializes optimizer context to default values") {
      optimizer_init(&ctx->optimizer);

      assert(ctx->optimizer.value_data == NULL);
    }
  }

  subdesc(optimizer_prepare) {
    it("allocates value_uses buffer based on largest function value_count") {
      generate_mock_ir(&ctx->ir);

      size_t max_values = 0;
      for (IrFunctionId fid = 0; fid < ctx->ir.functions.count; fid++) {
        IrFunction* function = &ctx->ir.functions.data[fid];

        if (function->value_count > max_values)
          max_values = function->value_count;
      }

      optimizer_prepare(ctx);

      assert(ctx->optimizer.value_data != NULL);

      for (size_t i = 0; i < max_values; i++)
        ctx->optimizer.value_data[i] = i + 1;
      for (size_t i = 0; i < max_values; i++)
        assert(ctx->optimizer.value_data[i] == i + 1);
    }

    it("throws on allocation failure") {
      generate_mock_ir(&ctx->ir);
      ctx->flags.forceNthMallocFail = 1;

      assertThrow(E_INTERNAL_ERROR, { optimizer_prepare(ctx); });

      assert(ctx->optimizer.value_data == NULL);
    }
  }

  subdesc(optimizer_free) {
    it("frees empty optimizer context correctly") {
      optimizer_free(&ctx->optimizer);

      assert(ctx->optimizer.value_data == NULL);
    }

    it("frees all allocated memory") {
      generate_mock_ir(&ctx->ir);
      optimizer_prepare(ctx);
      assert(ctx->optimizer.value_data != NULL);

      optimizer_free(&ctx->optimizer);

      assert(ctx->optimizer.value_data == NULL);
    }

    it("is safe to call twice") {
      generate_mock_ir(&ctx->ir);
      optimizer_prepare(ctx);

      optimizer_free(&ctx->optimizer);
      optimizer_free(&ctx->optimizer);

      assert(ctx->optimizer.value_data == NULL);
    }
  }

  after_each() {
    optimizer_free(&ctx->optimizer);
  }
}

snow_main();
