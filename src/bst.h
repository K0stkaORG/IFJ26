/**
 * @file bst.h
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 9. 10. 2026
 *
 * @brief Header file for the height-balanced search tree.
 */

#ifndef BST_H
#define BST_H

#include "ctx.h"

/**
 * @brief Node of the BST
 */
typedef struct BstNode BstNode;
struct BstNode {
  void* key;
  void* data;
  BstNode* child_left;
  BstNode* child_right;
  int height;
  int balance;
};

/**
 * @brief Type for function that compares keys and returns result in int.
 */
typedef int (*BstKeyComparator)(const void* a, const void* b);

/**
 * @brief Type for function that destructs key.
 */
typedef void (*BstKeyDestructor)(void* key);

/**
 * @brief Type for function that destructs data.
 */
typedef void (*BstDataDestructor)(void* data);

/**
 * @brief Height-balanced BST defined by its root
 */
typedef struct Bst {
  BstNode* root;
  BstKeyComparator key_comparator;
  BstKeyDestructor key_destructor;
  BstDataDestructor data_destructor;
} Bst;

void bst_init(Bst* tree,
              BstKeyComparator key_comparator,
              BstKeyDestructor key_destructor,
              BstDataDestructor data_destructor);
void bst_insert(Ctx* ctx, Bst* tree, void* key, void* data);
void* bst_search(Bst* tree, void* key);
void bst_destroy(Bst* tree);

#endif
