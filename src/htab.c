#include "htab.h"

#define AVG_LEN_MAX 2 // Max allowed average list length
#define RESIZE_COEF 2
/**
 * Obtains a properly casted key from the item ptr
 */
#define htab_get_st_key(item_p) ((htab_st_key_t)(item_p->data.key))
/**
 * Obtains a properly casted key from the item ptr
 */
#define htab_get_m_key(item_p) ((htab_m_key_t)(item_p->data.key))

htab_t* htab_init(size_t n)
{
    htab_t *t = my_malloc(sizeof(htab_t));

    t->arr_ptr = my_malloc(sizeof(htab_item_t*) * n);
    memset(t->arr_ptr, 0, sizeof(htab_item_t*) * n);

    t->size = 0;
    t->arr_size = n;

    return t;
}

void htab_clear(htab_t *t)
{
    htab_item_t *curr;

    for (size_t i = 0; i < t->arr_size; i++) {
        while (t->arr_ptr[i] != NULL) {
            curr = t->arr_ptr[i];
            t->arr_ptr[i] = t->arr_ptr[i]->next;
            
            free(curr->data.key);
            free(curr);
        }
    }
}

void htab_free(htab_t *t)
{
    if (t->arr_ptr != NULL) {
        htab_clear(t);
        free(t->arr_ptr);
    }
    free(t);
}

/**
 * The hash function used when operating with symbolic hash table items
 */
static size_t htab_st_hash_func(htab_key_t key_raw)
{
    htab_st_key_t key = (htab_st_key_t) key_raw;
    size_t val = 0;
    if (key->type == ST_VAL) {
        val = MY_HASH_COMB(val, key->val->var); // var is also uint64_t, so no need to cast
        val = MY_HASH_COMB_GMP(val, key->val->coef);
    }
    else {
        val = MY_HASH_COMB(val, key->ls);
        val = MY_HASH_COMB(val, (uint64_t) key->type);
        val = MY_HASH_COMB(val, key->rs);
    }
    return val;
}

/**
 * The hash function used when operating with measure hash table items
 */
static size_t htab_m_hash_func(htab_key_t key_raw) {
    return my_str_hash((htab_m_key_t) key_raw);
}

/**
 * Changes the size of the table's array and moves items from the original lists.
 * If the new size is 0, deletes the array.
 */
static void htab_resize(htab_t *t, size_t newn, size_t (*hash_func)(htab_key_t))
{
    if (newn == 0) {
        htab_clear(t);
        free(t->arr_ptr);
        t->arr_size = 0;
        t->size = 0;
        t->arr_ptr = NULL;
        return;
    }

    // alloc and init new array
    htab_item_t **new_arr_ptr;
    new_arr_ptr = my_malloc(newn * sizeof(htab_item_t*));
    memset(new_arr_ptr, 0, newn * sizeof(htab_item_t*));

    // index change
    htab_item_t *curr;
    htab_item_t *temp; // aux ptr to the elem in the new arr
    size_t new_index; // aux var for the new index of the current elem
    
    for (size_t i = 0; i < t->arr_size; i++) {
        curr = t->arr_ptr[i];

        while (curr != NULL) {
            new_index = hash_func(curr->data.key) % newn;

            // insert into new array
            if (new_arr_ptr[new_index] == NULL) {
                new_arr_ptr[new_index] = curr;
                curr = curr->next; //preserve the original list
                new_arr_ptr[new_index]->next = NULL;
            }
            else {
                temp = new_arr_ptr[new_index];
                while (temp->next != NULL) {
                    temp = temp->next;
                }
                temp->next = curr;
                curr = curr->next; // preserve the original list
                temp->next->next = NULL;
            }
        }
    }

    free(t->arr_ptr);
    t->arr_ptr = new_arr_ptr;
    t->arr_size = newn;
    return;
}

void htab_st_lookup_add(htab_t *t, htab_st_key_t key)
{
    htab_item_t *item = t->arr_ptr[htab_st_hash_func((htab_key_t)key) % t->arr_size];

    // find the item
    while (item != NULL) {
        if (htab_get_st_key(item) == key) {
            item->data.value++;
            return;
        }
        item = item->next;
    }

    item = my_malloc(sizeof(htab_item_t));
    // item init
    item->data.key = (htab_key_t) key;
    item->data.value = 1;
    item->next = NULL;

    // insert new item into the table
    size_t new_index = htab_st_hash_func((htab_key_t)key) % t->arr_size;
    if (t->arr_ptr[new_index] == NULL) {
        t->arr_ptr[new_index] = item;
    }
    else {
        htab_item_t *temp = t->arr_ptr[new_index];
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = item;
    }

    t->size++;
    if (((t->size + 0.0)/ t->arr_size) > AVG_LEN_MAX) { // +0.0 because of non-integer division
        htab_resize(t, t->size * RESIZE_COEF, htab_st_hash_func);
    }
}

void htab_m_lookup_add(htab_t *t, htab_m_key_t key)
{
    htab_item_t *item = t->arr_ptr[htab_m_hash_func(key) % t->arr_size];

    // find the item
    while (item != NULL) {
        if (strcmp(item->data.key,key) == 0) {
            item->data.value++;
            return;
        }
        item = item->next;
    }

    item = my_malloc(sizeof(htab_item_t));
    // item init
    htab_m_key_t key_temp = my_malloc(sizeof(char) * (strlen(key) + 1));
    item->data.key = (htab_key_t)(strcpy(key_temp, key));
    item->data.value = 1;
    item->next = NULL;

    // insert new item into the table
    size_t new_index = htab_m_hash_func(key) % t->arr_size;
    if (t->arr_ptr[new_index] == NULL) {
        t->arr_ptr[new_index] = item;
    }
    else {
        htab_item_t *temp = t->arr_ptr[new_index];
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = item;
    }

    t->size++;
    if (((t->size + 0.0) / t->arr_size) > AVG_LEN_MAX) { // +0.0 because of non-integer division
        htab_resize(t, t->size * RESIZE_COEF, htab_m_hash_func);
    }
}

void htab_m_print_all(htab_t *t, FILE *output)
{
    fprintf(output, "Sampled results:\n");

    htab_item_t *curr;
    for (size_t i = 0; i < t->arr_size; i++) {
        curr = t->arr_ptr[i];

        while (curr != NULL) {
            fprintf(output,"    \'");
            // key stored as LSBF
            for(int i=strlen(curr->data.key); i >= 0 ;i--) {
                putc(htab_get_m_key(curr)[i], output);
            }
            fprintf(output,"\'    %d\n", curr->data.value);
            curr = curr->next;
        }
    }
    return;
}

/* end of "htab.c" */