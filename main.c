#include <stdio.h>
#include <unistd.h>

#define PICO_BT_IMPLEMENTATION

typedef struct
{
    int distance_from_box1;
    int distance_from_box2;
    int items_in_bag;
    int battery_level;
    char last_action[64];
} Context;

#define CONTEXT Context
#include "pico_bt.h"

const char * status_to_str(enum STATUS s)
{
    switch (s)
    {
        case FAILURE: return "FAILURE";
        case SUCCESS: return "SUCCESS";
        case RUNNING: return "RUNNING";
        default:      return "UNKNOWN";
    }
}

void render_tui(int tick, enum STATUS root_status, Context * context)
{
    /* clear screen and move cursor home */
    printf("\033[2J\033[H");

    printf("+----------------------------------------------------------+\n");
    printf("| Tick: %-4d | Root status: %-8s                   |\n",
           tick, status_to_str(root_status));
    printf("+----------------+----------------+------------------------+\n");
    printf("| Box1 distance  | Box2 distance  | Items in bag           |\n");
    printf("+----------------+----------------+------------------------+\n");
    printf("| %14d | %14d | %22d |\n",
           context->distance_from_box1,
           context->distance_from_box2,
           context->items_in_bag);
    printf("+----------------+-----------------------------------------+\n");
    printf("| Battery level  | %-39d|\n", context->battery_level);
    printf("+----------------------------------------------------------+\n");
    printf("| Last action: %-40s |\n", context->last_action);
    printf("+----------------------------------------------------------+\n");

    fflush(stdout);
}

enum STATUS at_box1(struct Node * self, Context * context)
{
    if (context->distance_from_box1 <= 0)
    {
        snprintf(context->last_action, sizeof(context->last_action),
                 "at_box1: dist=%d -> YES", context->distance_from_box1);
        return YES;
    }

    else
    {
        snprintf(context->last_action, sizeof(context->last_action),
                 "at_box1: dist=%d -> NO", context->distance_from_box1);
        return NO;
    }
}

enum STATUS at_box2(struct Node * self, Context * context)
{
    if (context->distance_from_box2 <= 0)
    {
        snprintf(context->last_action, sizeof(context->last_action),
                 "at_box2: dist=%d -> YES", context->distance_from_box2);
        return YES;
    }

    else
    {
        snprintf(context->last_action, sizeof(context->last_action),
                 "at_box2: dist=%d -> NO", context->distance_from_box2);
        return NO;
    }
}

enum STATUS travel_to_box1(struct Node * self, Context * context)
{
    context->distance_from_box1 == 1 ? context->distance_from_box1 = 0 : context->distance_from_box1--;
    snprintf(context->last_action, sizeof(context->last_action),
             "travel_to_box1: dist=%d -> RUNNING", context->distance_from_box1);
    context->battery_level--;
    return RUNNING;
}

enum STATUS travel_to_box2(struct Node * self, Context * context)
{
    context->distance_from_box2 == 1 ? context->distance_from_box2 = 0 : context->distance_from_box2--;
    snprintf(context->last_action, sizeof(context->last_action),
             "travel_to_box2: dist=%d -> RUNNING", context->distance_from_box2);
    context->battery_level--;
    return RUNNING;
}

enum STATUS move_item_into_bag(struct Node * self, Context * context)
{
    context->items_in_bag++;
    snprintf(context->last_action, sizeof(context->last_action),
             "move_item_into_bag: items=%d -> SUCCESS", context->items_in_bag);
    context->battery_level--;
    return SUCCESS;
}

enum STATUS bag_full(struct Node * self, Context * context)
{
    if (context->items_in_bag >= 5)
    {
        snprintf(context->last_action, sizeof(context->last_action),
                 "bag_full: items=%d -> YES", context->items_in_bag);
        return YES;
    }

    else
    {
        snprintf(context->last_action, sizeof(context->last_action),
                 "bag_full: items=%d -> NO", context->items_in_bag);
        return NO;
    }
}

enum STATUS battery_empty(struct Node * self, Context * context)
{
    if (context->battery_level <= 0)
    {
        snprintf(context->last_action, sizeof(context->last_action),
                 "battery_empty: level=%d -> YES", context->battery_level);
        return YES;
    }

    else
    {
        snprintf(context->last_action, sizeof(context->last_action),
                 "battery_empty: level=%d -> NO", context->battery_level);
        return NO;
    }
}


int main()
{
    Context context = { 5, 6, 0, 20, "start", };

    struct Node * root =
        SELECTOR(
            LEAF(battery_empty),

            SEQUENCE(
                SELECTOR(
                    LEAF(at_box1),
                    LEAF(travel_to_box1),
                ),
                SELECTOR(
                    LEAF(bag_full),
                    REPEATER(LEAF(move_item_into_bag)),
                ),
                SELECTOR(
                    LEAF(at_box2),
                    LEAF(travel_to_box2),
                )

            )
        );

    enum STATUS root_status = RUNNING;
    int tick = 0;
    while (1)
    {
        root_status = root->behavior(root, &context);

        render_tui(tick, root_status, &context);

        if (root_status != RUNNING)
        {
            break;
        }

        tick++;
        sleep(1);
    }

    return 0;
}
