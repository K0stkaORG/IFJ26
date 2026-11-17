/**
 * @file lowering.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 9. 10. 2026
 *
 * @brief Functions for lowering the AST to IR
 *
 * This file contains public interface for lowering the AST to IR.
 */

#ifndef LOWERING_H
#define LOWERING_H

#include "ctx.h"

/**
 * @brief Lower the AST to IR
 *
 * This function takes the AST stored in the context and lowers it to IR (also
 * stored it in the context). It does not do any optimization. It does type
 * checking.
 *
 * @param[in,out] ctx Context of the compiler
 */
void lower_ast(Ctx* ctx);

#endif
