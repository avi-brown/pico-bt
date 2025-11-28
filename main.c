#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

/* =========================================================================
   1. CONTEXT & WORLD DEFINITION
   ========================================================================= */

#define MAP_W 12
#define MAP_H 10

typedef struct {
    /* Robot State */
    int x, y;
    int battery;
    int trash_collected;
    char face[8];        /* Current expression: [o_o], [-_-], etc */
    char status_msg[64]; /* Current thought bubble */
    
    /* Navigation Memory */
    int target_x;
    int target_y;
    int has_target;      /* 1 if we are locked onto a destination */

    /* The World Data (0=Empty, 1=Trash, 2=Charger) */
    int grid[MAP_H][MAP_W]; 
} Context;

/* Configure Library */
#define CONTEXT Context
#include "pico_bt.h"

/* =========================================================================
   2. VISUALIZATION (THE CUTE TUI)
   ========================================================================= */

void set_face(Context * ctx, const char * new_face) {
    snprintf(ctx->face, sizeof(ctx->face), "%s", new_face);
}

void say(Context * ctx, const char * msg) {
    snprintf(ctx->status_msg, sizeof(ctx->status_msg), "%s", msg);
}

void render_world(Context * ctx) {
    /* Move cursor home */
    printf("\033[2J\033[H"); 

    /* Header */
    printf(" +------------------------------------------+\n");
    printf(" | \033[1;36mBEEP-BOOP THE CLEANER BOT v1.0\033[0m           |\n");
    printf(" +------------------------------------------+\n");

    /* Map Rendering */
    for(int y=0; y<MAP_H; y++) {
        printf(" | ");
        for(int x=0; x<MAP_W; x++) {
            if (x == ctx->x && y == ctx->y) {
                /* Draw Robot with current Face, cropped to 3 chars to keep cell width consistent */
                char cell_face[4] = "   ";
                size_t len = strlen(ctx->face);
                if (len >= 4 && ctx->face[0] == '[' && ctx->face[len - 1] == ']') {
                    /* Use inner 3 characters, e.g. [o_o] -> o_o */
                    memcpy(cell_face, ctx->face + 1, 3);
                    cell_face[3] = '\0';
                } else {
                    snprintf(cell_face, sizeof(cell_face), "%.3s", ctx->face);
                }
                printf("\033[1;36m%s\033[0m", cell_face); 
            } else if (ctx->grid[y][x] == 1) {
                /* Trash */
                printf("\033[33m ★ \033[0m");
            } else if (ctx->grid[y][x] == 2) {
                /* Charger */
                printf("\033[1;32m C \033[0m");
            } else {
                /* Empty */
                printf("\033[90m · \033[0m");
            }
        }
        printf(" |\n");
    }

    /* Status Bar */
    printf(" +------------------------------------------+\n");
    
    /* Battery Bar */
    printf(" | BATTERY: [");
    int bars = ctx->battery / 10;
    const char * col = (ctx->battery < 30) ? "\033[31m" : "\033[32m";
    for(int i=0; i<10; i++) printf("%s%s", col, (i<bars)?"#":" ");
    /* Pad so the whole line width matches other boxed lines */
    printf("\033[0m] %3d%%               |\n", ctx->battery);

    /* Pad TRASH line to same total width as header/status lines */
    printf(" | TRASH:   \033[33m%2d\033[0m collected                    |\n", ctx->trash_collected);
    printf(" +------------------------------------------+\n");
    printf(" | STATUS: \033[35m%-32s\033[0m |\n", ctx->status_msg);
    printf(" +------------------------------------------+\n");
    
    fflush(stdout);
}

/* =========================================================================
   3. HELPERS
   ========================================================================= */

/* Returns SUCCESS if we arrived, RUNNING if moving */
enum STATUS move_step(Context * ctx) {
    if (ctx->x == ctx->target_x && ctx->y == ctx->target_y) return SUCCESS;

    /* Simple movement logic */
    if (ctx->x < ctx->target_x) ctx->x++;
    else if (ctx->x > ctx->target_x) ctx->x--;
    
    if (ctx->y < ctx->target_y) ctx->y++;
    else if (ctx->y > ctx->target_y) ctx->y--;

    /* Moving consumes battery */
    if (ctx->battery > 0) ctx->battery--;
    
    return RUNNING;
}

/* Find coordinates of nearest item type (1=Trash, 2=Charger) */
int scan_grid(Context * ctx, int type, int *out_x, int *out_y) {
    int min_dist = 9999;
    int found = 0;

    for(int y=0; y<MAP_H; y++) {
        for(int x=0; x<MAP_W; x++) {
            if (ctx->grid[y][x] == type) {
                int dist = abs(ctx->x - x) + abs(ctx->y - y);
                if (dist < min_dist) {
                    min_dist = dist;
                    *out_x = x;
                    *out_y = y;
                    found = 1;
                }
            }
        }
    }
    return found;
}

/* =========================================================================
   4. BEHAVIOR NODES
   ========================================================================= */

/* --- PRIORITY 1: SELF PRESERVATION --- */

enum STATUS is_battery_low(struct Node * self, Context * ctx) {
    if (ctx->battery < 20) {
        set_face(ctx, "[T_T]");
        say(ctx, "BATTERY CRITICAL! NEED JUICE!");
        return YES;
    }
    return NO;
}

enum STATUS find_charger(struct Node * self, Context * ctx) {
    if (scan_grid(ctx, 2, &ctx->target_x, &ctx->target_y)) {
        say(ctx, "Charger located...");
        return SUCCESS;
    }
    say(ctx, "NO CHARGER FOUND! PANIC!");
    return FAILURE;
}

enum STATUS perform_charging(struct Node * self, Context * ctx) {
    /* Are we actually at the charger? */
    if (ctx->grid[ctx->y][ctx->x] != 2) return FAILURE;

    if (ctx->battery >= 100) {
        ctx->battery = 100;
        set_face(ctx, "[^o^]");
        say(ctx, "Fully Charged! Ready to work.");
        return SUCCESS;
    }
    
    ctx->battery += 5;
    set_face(ctx, "[zZz]");
    say(ctx, "Charging... zzz...");
    return RUNNING;
}

/* --- PRIORITY 2: WORK (CLEANING) --- */

enum STATUS scan_for_trash(struct Node * self, Context * ctx) {
    if (scan_grid(ctx, 1, &ctx->target_x, &ctx->target_y)) {
        set_face(ctx, "[o_o]");
        say(ctx, "Trash detected. Target Acquired.");
        return SUCCESS;
    }
    return FAILURE;
}

enum STATUS pickup_trash(struct Node * self, Context * ctx) {
    /* Check if trash is actually here */
    if (ctx->grid[ctx->y][ctx->x] == 1) {
        ctx->grid[ctx->y][ctx->x] = 0; /* Remove trash */
        ctx->trash_collected++;
        ctx->battery -= 2; /* Work costs energy */
        set_face(ctx, "[>_<]");
        say(ctx, "YOINK! Trash collected.");
        return SUCCESS;
    }
    return FAILURE; /* Trash gone? */
}

/* --- PRIORITY 3: IDLE --- */

enum STATUS pick_random_spot(struct Node * self, Context * ctx) {
    /* Only pick a spot if we don't have one (or arrived at old one) */
    if (ctx->x == ctx->target_x && ctx->y == ctx->target_y) {
        ctx->target_x = rand() % MAP_W;
        ctx->target_y = rand() % MAP_H;
        set_face(ctx, "[~_~]");
        say(ctx, "Wandering around...");
    }
    return SUCCESS;
}

/* --- GENERIC ACTIONS --- */

enum STATUS move_to_target(struct Node * self, Context * ctx) {
    if (ctx->x == ctx->target_x && ctx->y == ctx->target_y) return SUCCESS;
    
    say(ctx, "Moving...");
    return move_step(ctx);
}

/* =========================================================================
   5. MAIN
   ========================================================================= */

int main() {
    srand(time(NULL));

    /* Init Context */
    Context ctx = {0};
    ctx.x = 0; ctx.y = 0;
    ctx.battery = 50;
    ctx.trash_collected = 0;
    set_face(&ctx, "[o_o]");
    
    /* Place Charger at bottom right */
    ctx.grid[MAP_H-1][MAP_W-1] = 2;

    /* Sprinkle some trash */
    for(int i=0; i<5; i++) 
        ctx.grid[rand()%(MAP_H-1)][rand()%(MAP_W-1)] = 1;

    /* 
       THE BRAIN 
       Root (Selector)
        1. Survival: If Low Battery -> Find Charger -> Move -> Charge
        2. Work: If Trash Found -> Move -> Pickup
        3. Idle: Pick Random Spot -> Move
    */
    struct Node * brain = 
        SELECTOR(
            /* 1. Survival */
            MEM_SEQUENCE(
                LEAF(is_battery_low),
                LEAF(find_charger),
                LEAF(move_to_target),
                LEAF(perform_charging) /* Returns RUNNING until full */
            ),
            
            /* 2. Cleaning Work */
            SEQUENCE(
                LEAF(scan_for_trash),
                LEAF(move_to_target),
                LEAF(pickup_trash)
            ),
            
            /* 3. Idle / Wander */
            SEQUENCE(
                LEAF(pick_random_spot),
                LEAF(move_to_target)
            )
        );

    while(1) {
        /* Add new trash randomly to keep game going */
        if (rand() % 20 == 0) {
            int tx = rand() % MAP_W;
            int ty = rand() % MAP_H;
            if (ctx.grid[ty][tx] == 0) ctx.grid[ty][tx] = 1;
        }

        brain->behavior(brain, &ctx);
        
        render_world(&ctx);
        usleep(200000); /* 200ms Tick */
    }

    return 0;
}
