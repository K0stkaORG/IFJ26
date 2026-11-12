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

/**
 * @brief Destructs symtable data
 * @param data Data to destruct
 * @pre param_types in function data must be on heap
 */
static void symtabledata_destructor(void* data) {
  SymtableData* dataptr = (SymtableData*)data;
  if (dataptr != NULL) {
    if (dataptr->symbol_type == SYMBOL_FUNCTION && dataptr->param_types != NULL)
      free(dataptr->param_types);
    free(dataptr);
  }
}

void symtable_init(Ctx* ctx, Symtable* table, Symtable* parent) {
  if (table == NULL)
    THROW(E_INTERNAL_ERROR, "attempted to initialize NULL symtable");
  bst_init(&table->tree, (BstKeyComparator)strcmp, (BstKeyDestructor)free,
           (BstDataDestructor)symtabledata_destructor);
  table->parent = parent;
}

void symtable_destroy(Symtable* table) {
  if (table == NULL)
    return;
  bst_destroy(&table->tree);
  table->parent = NULL;
}

void symtable_insert(Ctx* ctx,
                     Symtable* table,
                     char* identifier,
                     SymtableData data) {
  if (table == NULL)
    THROW(E_INTERNAL_ERROR, "attempted to insert into NULL symtable");
  if (identifier == NULL)
    THROW(E_INTERNAL_ERROR,
          "attempted to insert NULL identifier into symtable");

  SymtableData* dataptr = TRY_MALLOC(sizeof(SymtableData));

  char* identptr = TRY_MALLOC_OR(strlen(identifier) + 1, {
    if (dataptr != NULL)
      free(dataptr);
  });

  *dataptr = data;
  strcpy(identptr, identifier);

  bst_insert(ctx, &table->tree, identptr, dataptr);
}
