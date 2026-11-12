/**
 * @file test_bst.c
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 5. 11. 2026
 *
 * @brief Unit tests for symbol table
 */

#include "array.h"
#define SNOW_ENABLED 1
#include "snow/snow.h"
#include "symtable.h"

describe(symtable) {
  subdesc(symtable_init) {
    Symtable table;
    Symtable table2;
    it("symtable initialization") {
      symtable_init(&table, NULL);
      asserteq_ptr(table.tree.root, NULL);
      asserteq_ptr(table.parent, NULL);
    }
    it("symtable initialization with parent") {
      symtable_init(&table2, &table);
      asserteq_ptr(table2.tree.root, NULL);
      asserteq_ptr(table2.parent, &table);
      asserteq_ptr(table2.parent->tree.root, NULL);
      asserteq_ptr(table2.parent->parent, NULL);
    }
  }
  subdesc(symtable_destroy) {}
  subdesc(symtable_insert) {}
  subdesc(symtable_lookup) {}
}

snow_main();
