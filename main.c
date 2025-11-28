#include "pico/stdlib.h"

typedef struct {
    int dir;
    int n_leds;
    int index;
} Context;

#define CONTEXT Context
#include "pico_bt.h"

#define LEFT 0
#define RIGHT 1
#define DIR_PIN 8

enum STATUS update_dir(struct Node *self, Context *ctx) {
    ctx->dir = gpio_get(DIR_PIN);
    return SUCCESS;
}

enum STATUS step_leds(struct Node *self, Context *ctx) {

    for (int led = 0; led < ctx->n_leds; led++) {
        if (led == ctx->index) {
            gpio_put(led, 1);
        }

        else {
            gpio_put(led, 0);
        }
    }

    if (ctx->dir == LEFT) {
        ctx->index++;
        if (ctx->index == ctx->n_leds) {
            ctx->index = 0;
        }
    } else {
        ctx->index--;
        if (ctx->index < 0) {
            ctx->index = ctx->n_leds - 1;
        }
    }

    return RUNNING;
}

int main() {
    Context ctx = {.index = 0, .n_leds = 8, .dir = LEFT};

    for (int i = 0; i < ctx.n_leds; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
        gpio_put(i, 0);
    }

    gpio_init(DIR_PIN);
    gpio_set_dir(DIR_PIN, GPIO_IN);
    gpio_pull_down(DIR_PIN);

    struct Node *tree = SEQUENCE(LEAF(update_dir), LEAF(step_leds));

    while (tree->tick(tree, &ctx) != FAILURE) {
        sleep_ms(100);
    }
}
