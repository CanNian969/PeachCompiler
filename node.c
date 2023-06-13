#include "compiler.h"
#include <assert.h>
#include "helpers/vector.h"

struct vector *node_vector = NULL;
struct vector *node_vector_root = NULL;

void node_set_vector(struct vector *vec, struct vector *vec_root)
{
    node_vector = vec;
    node_vector_root = vec_root;
}

void node_push(struct node *node)
{
    vector_push(node_vector, &node);
}

struct node *node_peek_or_null()
{
    return vector_back_ptr_or_null(node_vector);
}

struct node *node_peek()
{
    return *(struct node **)vector_back(node_vector);
}

struct node *node_pop()
{
    struct node *last_node = vector_back_ptr(node_vector);
    struct node *last_node_root = vector_empty(node_vector_root) ? NULL : vector_back_ptr(node_vector_root);
    vector_pop(node_vector);

    if (last_node == last_node_root)
    {
        vector_pop(node_vector_root);
    }
    return last_node;
}

struct node *node_create(struct node *_node)
{
    struct node *node = malloc(sizeof(struct node));
    memcpy(node, _node, sizeof(struct node));
#warning "We should set the binded owner and binded function here"
    node_push(node);
    return node;
}