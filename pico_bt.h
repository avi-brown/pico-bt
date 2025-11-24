#ifndef PICO_BT_H
#define PICO_BT_H

enum STATUS 
{
    SUCCESS,
    RUNNING,
    FAILURE
};

struct Node;

typedef enum STATUS (*tick)(struct Node * self, void * arena);

struct Node 
{
    tick tick_cb;
    struct Node ** children;
    int n_children;
};

enum STATUS sequence(struct Node * self, void * arena);

/* ------------------------ */
#ifdef PICO_BT_IMPLEMENTATION

enum STATUS sequence(struct Node * self, void * arena) 
{
    for (int child = 0; child < self->n_children; child++) 
    {
        struct Node * current_child = self->children[child];
        enum STATUS result = current_child->tick_cb(current_child, arena);
        if (result != SUCCESS) 
        {
            return result;
        }
    }

    return SUCCESS;
}

#endif
/* ------------------------ */

#endif