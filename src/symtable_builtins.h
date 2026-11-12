/**
 * @file symtable_builtins.h
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 12. 11. 2026
 *
 * @brief Built-in compiler functions for symbol table
 */

#ifndef SYMTABLE_BUILTINS_H
#define SYMTABLE_BUILTINS_H
typedef struct Ctx Ctx;
typedef struct Symtable Symtable;

/**
 * @brief Populate symtable with IFJ26 built-in functions
 * @param table Initialized global symtable pointer
 * @pre Symtable must be initialized
 */
void symtable_builtins_populate(Symtable* global_table);

#endif
