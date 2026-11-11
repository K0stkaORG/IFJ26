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

#include <stdbool.h>
#include <stddef.h>
#include "bst.h"
#include "ir.h"

/**
 * @brief Type of symbol stored in the symbol table
 */
typedef enum { SYMBOL_FUNCTION, SYMBOL_VARIABLE } SymbolType;

/**
 * @brief Symbol table
 * @param tree Underlying binary search tree
 * @param parent Parent local scope or NULL if global table
 */
typedef struct Symtable {
  Bst tree;
  struct Symtable* parent;
} Symtable;

/**
 * @brief Data for symbol table
 * @param symbol_type type of symbol: function or variable
 * @param ir_type type in IR
 * @param ir_index index in IR
 * @param param_count for functions: number of parameters function accepts
 * @param param_types for functions: array of types of function params
 * @param is_val for variables: is value or variable (true if value)
 */
typedef struct SymtableData {
  // common
  SymbolType symbol_type;
  IrType ir_type;
  size_t ir_index;

  // functions
  size_t param_count;
  IrType* param_types;

  // variables
  bool is_val;
} SymtableData;

typedef struct SymtableResult SymtableResult;

void symtable_create(Symtable* table);
void symtable_destroy(Symtable* table);
void symtable_insert(Symtable* table, SymtableData data);
SymtableResult symtable_lookup(Symtable* table);  // + lookup key

#endif
