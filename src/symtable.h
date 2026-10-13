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

typedef struct SymtableResult SymtableResult;

void symtable_create(Symtable* table);
void symtable_destroy(Symtable* table);
void symtable_insert(Symtable* table, SymtableData data);
SymtableResult symtable_lookup(Symtable* table);  // + lookup key

#endif
