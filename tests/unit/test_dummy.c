/**
 * @file test_dummy.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 8. 10. 2026
 *
 * @brief Dummy unit test
 *
 * Dummy unit test acting as a placeholder until actual tests are written.
 */

#define SNOW_ENABLED 1
#include "snow/snow.h"

describe(dummy) {
  it("adds integers") {
    asserteq(1 + 2, 3);
  }

  it("compares strings by content") {
    asserteq_str("abc", "abc");
  }
}

snow_main();
