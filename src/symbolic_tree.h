#include <stdint.h>
#include <stdbool.h>
#include <gmp.h>
#include "error.h"

#ifndef SYMBOLIC_TREE_H
#define SYMBOLIC_TREE_H

/**
 * Type for symbolic variables' coefficients
 */
typedef mpz_t coefs_t;

/**
 * Type for symbolic variables
 */
typedef uint64_t vars_t;

/**
 * Type for symbolic tree node type
 */
typedef enum {
    ST_ADD,
    ST_SUB,
    ST_MUL,
    ST_VAL
} stnode_t;

/**
 * Type for symbolic tree node value
 */
typedef struct stnode_val {
    coefs_t coef;
    vars_t var;
} stnode_val_t;

/**
 * Type for symbolic tree node
 */
typedef struct stree {
    stnode_val_t *val;
    stnode_t type;
    struct stree *ls;
    struct stree *rs;
} stree_t;

/**
 * Max. string size when converting expression from tree to string
 */
#define MAX_ST_TO_STR_LEN 2000

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