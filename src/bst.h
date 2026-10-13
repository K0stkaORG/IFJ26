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

typedef struct BstNode BstNode;
struct BstNode {
  char* key;
  void* data;
  BstNode* child_left;
  BstNode* child_right;
  int height;
  int balance;
};

typedef struct Bst {
  BstNode* root;
} Bst;

void bst_create(Bst* tree);
void bst_insert(Bst* tree, const char* key, void* data);
void* bst_search(Bst* tree, const char* key);
void bst_destroy(Bst* tree);

#endif
