/**
 * @file test_bst.c
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 5. 11. 2026
 *
 * @brief Unit tests for symbol table
 */

#include "snow/snow.h"
#include "symtable.h"

describe(symtable) {
  subdesc(symtable_create) {
    Symtable table;
    (void)table;
  }
  subdesc(symtable_destroy) {}
  subdesc(symtable_insert) {}
  subdesc(symtable_lookup) {}
}
