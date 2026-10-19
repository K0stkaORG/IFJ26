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
  char* key;
  void* data;
  BstNode* child_left;
  BstNode* child_right;
  int height;
  int balance;
};

/**
 * @brief Height-balanced BST defined by its root
 */
typedef struct Bst {
  BstNode* root;
} Bst;

void bst_init(Bst* tree);
void bst_insert(Ctx* ctx, Bst* tree, const char* key, void* data);
void* bst_search(Bst* tree, const char* key);
void bst_destroy(Bst* tree);

#endif
