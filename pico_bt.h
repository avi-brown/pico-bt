#ifndef PICO_BT_H
#define PICO_BT_H

/* ------------------------ */
#ifndef CONTEXT
#define CONTEXT void
#endif

#define _ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,    \
               _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26,     \
               _27, _28, _29, _30, _31, _32, N, ...)                           \
    N

#define _RSEQ_N()                                                              \
    32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15,    \
        14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define _NARG_(...) _ARG_N(__VA_ARGS__)
#define NUM_ARGS(...) _NARG_(__VA_ARGS__, _RSEQ_N())

#define LEAF(fn) &((struct Node){.tick = fn, .n_children = 0, .children = 0})

#define COMPOSITE(fn, ...)                                                     \
    &((struct Node){.tick = fn,                                                \
                    .n_children = NUM_ARGS(__VA_ARGS__),                       \
                    .children = (struct Node *[]){__VA_ARGS__},                \
                    .current_child = 0})

#define SEQUENCE(...) COMPOSITE(sequence, __VA_ARGS__)
#define MEM_SEQUENCE(...) COMPOSITE(mem_sequence, __VA_ARGS__)
#define SELECTOR(...) COMPOSITE(selector, __VA_ARGS__)
#define INVERTER(...) COMPOSITE(inverter, __VA_ARGS__)
#define REPEATER(...) COMPOSITE(repeater, __VA_ARGS__)
/* ------------------------ */

enum STATUS { FAILURE, SUCCESS, RUNNING };

#define YES SUCCESS
#define NO FAILURE

struct Node;

typedef enum STATUS (*tick_cb)(struct Node *self, CONTEXT *context);

struct Node {
    tick_cb tick;
    struct Node **children;
    int n_children;
    int current_child;
};

enum STATUS sequence(struct Node *self, CONTEXT *context);
enum STATUS mem_sequence(struct Node *self, CONTEXT *context);
enum STATUS selector(struct Node *self, CONTEXT *context);
enum STATUS inverter(struct Node *self, CONTEXT *context);
enum STATUS repeater(struct Node *self, CONTEXT *context);

enum STATUS sequence(struct Node *self, CONTEXT *context) {
    for (int child = 0; child < self->n_children; child++) {
        struct Node *current_child = self->children[child];
        enum STATUS result = current_child->tick(current_child, context);
        if (result != SUCCESS) {
            return result;
        }
    }

    return SUCCESS;
}

enum STATUS mem_sequence(struct Node *self, CONTEXT *context) {
    for (int child = self->current_child; child < self->n_children; child++) {
        struct Node *current_child = self->children[child];
        enum STATUS result = current_child->tick(current_child, context);
        if (result != SUCCESS) {
            self->current_child = child;
            return result;
        }
    }

    if (self->current_child == self->n_children - 1) {
        self->current_child = 0;
    }

    return SUCCESS;
}

enum STATUS selector(struct Node *self, CONTEXT *context) {
    for (int child = 0; child < self->n_children; child++) {
        struct Node *current_child = self->children[child];
        enum STATUS result = current_child->tick(current_child, context);
        if (result != FAILURE) {
            return result;
        }
    }

    return FAILURE;
}

enum STATUS inverter(struct Node *self, CONTEXT *context) {
    struct Node *child = self->children[0]; /* Inverters have a single child */
    enum STATUS result = child->tick(child, context);
    switch (result) {
    case SUCCESS:
        return FAILURE;
    case FAILURE:
        return SUCCESS;
    case RUNNING:
        return RUNNING;
    default:
        return FAILURE;
    }
}

enum STATUS repeater(struct Node *self, CONTEXT *context) {
    struct Node *child = self->children[0]; /* Repeaters have a single child */
    enum STATUS result = child->tick(child, context);

    if (result == SUCCESS) {
        return RUNNING;
    }

    else {
        return result;
    }
}

#endif
