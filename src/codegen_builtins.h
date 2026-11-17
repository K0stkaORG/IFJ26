/**
 * @file codegen_builtins.h
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 17. 11. 2026
 *
 * @brief IFJcode26 built-in functions codegen
 */

#ifndef CODEGEN_BUILTINS_H
#define CODEGEN_BUILTINS_H

#include <stdio.h>
#include "ir.h"

void cg_generate_builtin(FILE* out, IrFunctionType type);

#endif
