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

typedef struct Ctx Ctx;

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
 * @brief Node of the BST
 */
typedef struct BstNode BstNode;

/**
 * @brief Height-balanced BST defined by its root
 */
typedef struct Bst {
  BstNode* root;
  BstKeyComparator key_comparator;
  BstKeyDestructor key_destructor;
  BstDataDestructor data_destructor;
} Bst;

/**
 * @brief Initialize a new BST
 * @param tree Pointer to the tree to initialize
 * @param key_comparator Function that compares abstract keys
 * @param key_destructor Function that destructs key, or NULL if not owned
 * @param data_destructor Function that destructs data, or NULL if not owned
 * @pre tree != NULL
 * @pre key_comparator != NULL and must be a function that returns int
 */
void bst_init(Bst* tree,
              BstKeyComparator key_comparator,
              BstKeyDestructor key_destructor,
              BstDataDestructor data_destructor);

/**
 * @brief Insert data under key into BST
 * @param ctx Error context
 * @param tree Pointer to the tree
 * @param key Key of the node
 * @param data Data stored in the node
 * @pre tree != NULL, key != NULL
 * @pre If key destructor is set, key must be allocated on heap.
 * @pre If data destructor is set, data must be allocated on heap.
 * @pre Data must not be reinserted under different key.
 * @pre Reusing key will result in original data loss.
 */
void bst_insert(Ctx* ctx, Bst* tree, void* key, void* data);

/**
 * @brief Search data in BST by key
 * @param tree Pointer to the tree to search in
 * @param key Key to search for in the tree
 * @return Pointer to the data if found, or NULL
 */
void* bst_search(Bst* tree, void* key);

/**
 * @brief Destructor for BST
 * @param tree Pointer to the tree to destroy
 */
void bst_destroy(Bst* tree);

#endif
