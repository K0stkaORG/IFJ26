/**
 * @file ctx.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 9. 10. 2026
 *
 * @brief Shared memory and error context for the IFJ26 compiler
 *
 * This file contains the shape of the shared memory and error context for the
 * IFJ26 compiler. It also contains macros for simple and unified error
 * handling.
 */

#ifndef CTX_H
#define CTX_H

/**
 * @brief Shared memory and error context for the IFJ26 compiler
 */
typedef struct {
  int dummy;
} Ctx;

/**
 * @brief Initializes the shared context to default values
 *
 * @param[out] ctx Address of the context to initialize
 */
void ctx_init(Ctx* ctx);

#endif
