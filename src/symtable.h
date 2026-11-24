/**
 * @file symtable.h
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

typedef struct Ctx Ctx;

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
 * @param element_type type of element (list)
 * @param param_element_types types of elements in parameters (lists as params)
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

  // lists
  IrType element_type;
  IrType* param_element_types;
} SymtableData;

/**
 * @brief Initializes symbol table
 * @param ctx Context
 * @param table Table to initialize
 * @param parent Parent table or NULL if global
 * @pre table != NULL
 */
void symtable_init(Ctx* ctx, Symtable* table, Symtable* parent);

/**
 * @brief Destroys symbol table
 * @param table Table to destroy
 */
void symtable_destroy(Symtable* table);

/**
 * @brief Inserts data into symbol table
 * @param ctx Context
 * @param table Table to insert into
 * @param identifier String identifier
 * @param data Symbol table data
 * @pre Table must be initialized
 * @pre Caller retains ownership of identifier string
 * @pre Data passed by value. param_types and param_element_types in the struct
 * must be allocated on heap by caller and ownership is transferred to symtable.
 */
void symtable_insert(Ctx* ctx,
                     Symtable* table,
                     char* identifier,
                     SymtableData data);

/**
 * @brief Lookups data in the symbol table
 * @param table Table for lookup start
 * @param identifier String identifier to look up
 * @param recursive false if searching only local scope, true for recursive
 * search in all scopes
 * @pre If table is not NULL, it must be initialized
 * @return Pointer to SymtableData if found or NULL. Symtable retains ownership.
 */
SymtableData* symtable_lookup(Symtable* table,
                              char* identifier,
                              bool recursive);

#endif
