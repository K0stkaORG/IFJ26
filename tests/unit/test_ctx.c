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

describe(ctx) {
  Ctx context;
  Ctx* ctx = &context;

  it("initializes correctly") {
    ctx_init(ctx);

    asserteq(ctx->exit_code, E_OK);
  }
}

snow_main();
