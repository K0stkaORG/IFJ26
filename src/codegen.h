/**
 * @file codegen.h
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 14. 11. 2026
 *
 * @brief IFJcode26 generator from Intermediate Representation
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ctx.h"

/**
 * @brief IFJcode26 generator
 * @param ctx Compiler context
 * @param output File to output to
 */
void cg_generate(Ctx* ctx, FILE* output);

#endif
