/**
 * @file symtable.c
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 12. 10. 2026
 *
 * @brief Headers for compiler symbol table.
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "bst.h"

typedef struct Symtable {
  Bst* tree;
} Symtable;

typedef struct SymtableData SymtableData;

Symtable* symtable_create(void);
void symtable_destroy(Symtable* table);
void symtable_insert(Symtable* table, SymtableData data);
SymtableData symtable_lookup(Symtable* table);  // + lookup key

#endif
