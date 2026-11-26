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
 * @pre param_element_types in function data must be on heap
 * @pre param_list_depths in function data must be on heap
 */
static void symtabledata_destructor(void* data) {
  SymtableData* dataptr = (SymtableData*)data;
  if (dataptr != NULL) {
    if (dataptr->symbol_type == SYMBOL_FUNCTION) {
      if (dataptr->param_types != NULL)
        free(dataptr->param_types);
      if (dataptr->param_element_types != NULL)
        free(dataptr->param_element_types);
      if (dataptr->param_list_depths != NULL)
        free(dataptr->param_list_depths);
    }
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

  SymtableData* dataptr = TRY_MALLOC_OR(sizeof(SymtableData), {
    if (data.param_types != NULL)
      free(data.param_types);
    if (data.param_element_types != NULL)
      free(data.param_element_types);
    if (data.param_list_depths != NULL)
      free(data.param_list_depths);
  });
  *dataptr = data;

  char* identptr = TRY_MALLOC_OR(strlen(identifier) + 1,
                                 { symtabledata_destructor(dataptr); });
  strcpy(identptr, identifier);

  bst_insert(ctx, &table->tree, identptr, dataptr);
}

SymtableData* symtable_lookup(Symtable* table,
                              char* identifier,
                              bool recursive) {
  if (table == NULL || identifier == NULL)
    return NULL;
  if (recursive) {
    SymtableData* dataptr = (SymtableData*)bst_search(&table->tree, identifier);
    if (dataptr == NULL && table->parent != NULL)
      dataptr = symtable_lookup(table->parent, identifier, true);
    return dataptr;
  } else {
    SymtableData* dataptr = (SymtableData*)bst_search(&table->tree, identifier);
    return dataptr;
  }
}
