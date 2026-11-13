/**
 * @file symtable_builtins.c
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 12. 11. 2026
 *
 * @brief Built-in compiler functions for symbol table
 */

#include "symtable_builtins.h"
#include <stddef.h>
#include <string.h>
#include "ctx.h"
#include "ir.h"
#include "symtable.h"

static void symtable_create_builtin(Ctx* ctx,
                                    Symtable* table,
                                    char* name,
                                    IrFunctionType fx_type,
                                    IrType return_type,
                                    size_t param_count,
                                    IrType* params) {
  SymtableData data;
  data.symbol_type = SYMBOL_FUNCTION;
  data.ir_type = return_type;
  data.ir_index = fx_type;
  data.param_count = param_count;
  data.is_val = false;

  if (param_count > 0) {
    IrType* param_arr = TRY_MALLOC(sizeof(IrType) * param_count);
    memcpy(param_arr, params, sizeof(IrType) * param_count);
    data.param_types = param_arr;
  } else {
    data.param_types = NULL;
  }

  symtable_insert(ctx, table, name, data);
}

void symtable_builtins_populate(Ctx* ctx, Symtable* global_table) {
  if (global_table == NULL)
    return;
}
