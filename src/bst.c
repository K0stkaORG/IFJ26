/**
 * @file bst.c
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 9. 10. 2026
 *
 * @brief Height-balanced search tree.
 */

#include "bst.h"
#include <stddef.h>

/**
 * @brief Initialize a new BST node
 * @param node Pointer to the node to initialize
 * @param key Lookup key for the node in the BST
 * @param data Data payload of the node
 */
static void bst_node_init(BstNode* node, char* key, void* data) {
  node->balance = 0;
  node->height = 1;
  node->child_left = NULL;
  node->child_right = NULL;
  node->key = key;
  node->data = data;
}

/**
 * @brief Get BST node height
 * @param node Pointer to the node
 * @return Height of the subtree rooted at the node, or 0 if NULL
 */
static int bst_node_get_height(BstNode* node) {
  if (node == NULL)
    return 0;
  return node->height;
}

/**
 * @brief Get BST node balance
 * @param node Pointer to the node
 * @return Balance factor, or 0 if NULL
 */
static int bst_node_get_balance(BstNode* node) {
  if (node == NULL)
    return 0;
  return bst_node_get_height(node->child_left) -
         bst_node_get_height(node->child_right);
}

/**
 * @brief Initialize a new BST
 * @param tree Pointer to the tree to initialize
 */
void bst_init(Bst* tree) {
  tree->root = NULL;
}
