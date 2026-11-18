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
 * @param out File to output to
 * @param ctx Compiler context
 */
void cg_generate(FILE* out, Ctx* ctx);

#endif
