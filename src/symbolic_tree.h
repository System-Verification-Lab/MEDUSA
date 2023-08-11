#include <stdint.h>
#include <stdbool.h>
#include <gmp.h>
#include "symbolic_tree_types.h"
#include "htab.h"
#include "error.h"

#ifndef SYMBOLIC_TREE_H
#define SYMBOLIC_TREE_H

/**
 * Max. string size when converting expression from tree to string
 */
#define MAX_ST_TO_STR_LEN 2000

/**
 * Initializes the symbolic hash table - must be initialized before any symbolic value MTBDD can be initialized!
 */
void st_htab_init();

/**
 * Clears the symbolic hash table - deletes all items (the array size is not changed)
 */
void st_htab_clear();

/**
 * Deletes the symbolic hash table
 */
void st_htab_delete();

/**
 * Creates a new tree for the given complex number coefficient variable
 */
stree_t* st_create_val(vars_t v);

/**
 * Creates a new tree and initializes it from the data provided
 */
stree_t* st_init(stree_t *t);

/**
 * Performs the given operation on the two trees (a op b) and returns the result
 */
stree_t* st_op(stree_t *a, stree_t *b, stnode_t op);

/**
 * Performs multiplication of the tree leaf values
 */
void st_coef_mul(stree_t *t, int64_t c);

/**
 * Returns true if the two trees are equal
 */
bool st_cmp(stree_t *a, stree_t *b);

/**
 * Returns the string representation of the expression represented by tree
 */
char* st_to_str(stree_t *data);

/**
 * Deletes the given tree
 */
void st_delete(stree_t *t);

#endif
/* end of "symbolic_tree.h" */