/**
 * @file symtable.c
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 12. 10. 2026
 *
 * @brief Compiler symbol table.
 */

#include "symtable.h"
#include <stdlib.h>
#include <string.h>
#include "bst.h"
#include "ctx.h"

static void symtabledata_create(SymtableData* data) {
  void;
}
static void symtabledata_destructor(void* data) {
  void;
}

void symtable_init(Ctx* ctx, Symtable* table, Symtable* parent) {
  if (table == NULL)
    THROW(E_INTERNAL_ERROR, "attempted to initialize NULL symtable");
  bst_init(&table->tree, (BstKeyComparator)strcmp, (BstKeyDestructor)free,
           (BstDataDestructor)symtabledata_destructor);
  table->parent = parent;
}
