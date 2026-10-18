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
#include <stdlib.h>
#include <string.h>

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
 * @brief Destructs a BST subtree defined by root node
 * @param node Pointer to the root node
 */
static void bst_node_destroy_subtree(BstNode* node) {
  if (node == NULL)
    return;
  bst_node_destroy_subtree(node->child_left);
  bst_node_destroy_subtree(node->child_right);
  if (node->data != NULL)
    free(node->data);
  if (node->key != NULL)
    free(node->key);
  free(node);
}

/**
 * @brief Initialize a new BST
 * @param tree Pointer to the tree to initialize
 */
void bst_init(Bst* tree) {
  tree->root = NULL;
}

/**
 * @brief Search data in BST by key
 * @param tree Pointer to the tree to search in
 * @param key Key to search for in the tree
 * @return Pointer to the data if found, or NULL
 */
void* bst_search(Bst* tree, const char* key) {
  if (tree == NULL)
    return NULL;

  BstNode* current_node = tree->root;
  while (current_node != NULL) {
    int comparison = strcmp(key, current_node->key);
    if (comparison == 0)
      return current_node->data;
    else if (comparison < 0)
      current_node = current_node->child_left;
    else if (comparison > 0)
      current_node = current_node->child_right;
  }
  return NULL;
}

/**
 * @brief Destructor for BST
 * @param tree Pointer to the tree to destroy
 */
void bst_destroy(Bst* tree) {
  if (tree == NULL)
    return;
  bst_node_destroy_subtree(tree->root);
  tree->root = NULL;
}
