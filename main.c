#define PICO_BT_IMPLEMENTATION
#include <stdio.h>
#include "pico_bt.h"
#include <unistd.h>
void sleep_ms(int ms) { usleep(ms * 1000); }

/* data */
typedef struct {
    int target_rpm;
    float rpm;
    int timer;
    float kp;
} data;

    // if (!arena->timer)
    // {
    //     printf("[check_rpm]     FAILURE: timout\n");
    //     return FAILURE;
        // arena->timer--;

    // }

enum STATUS time_remains(struct Node * self, void *data_arena)
{
    data *arena = (data *)data_arena;
    if (arena->timer--)
    {
        printf("[time_remains]      SUCCESS: timer = %d\n", arena->timer);
        return SUCCESS;
    }
    else
    {
        printf("[time_remains]      FAILURE: TIMEOUT");
        return FAILURE;
    }
}

enum STATUS rpm_not_reached(struct Node * self, void *data_arena) 
{
    data *arena = (data *)data_arena;

    if (arena->rpm != (float)arena->target_rpm) 
    {
        printf("[rpm_not_reached]   SUCCESS: rpm == %.2f, target = %d\n", arena->rpm, arena->target_rpm);
        sleep_ms(100);
        return SUCCESS;
    }

    else
    {
        printf("[rpm_not_reached]   FAILURE:  rpm == %.2f, target = %d\n", arena->rpm, arena->target_rpm);
        return FAILURE;
    }
}

enum STATUS rpm_updated(struct Node * self, void *data_arena) 
{
    data *arena = (data *)data_arena;
    arena->rpm += arena->kp * ((float)arena->target_rpm - arena->rpm);
    return SUCCESS;
}

int main() {
    struct Node n_time_remains     = { .tick_cb = time_remains,    .n_children = 0 };
    struct Node n_rpm_not_reached  = { .tick_cb = rpm_not_reached, .n_children = 0 };
    struct Node n_rpm_updated      = { .tick_cb = rpm_updated,     .n_children = 0 };

    struct Node * children[] = { &n_time_remains, &n_rpm_not_reached, &n_rpm_updated };
    
    struct Node rpm_sequence = {
        .tick_cb = sequence,
        .children = children,
        .n_children = 3
    };

    data s_data;

    printf("\n--- TEST ---\n");
    s_data.target_rpm = 10;
    s_data.rpm = 0.0;
    s_data.kp = 0.6;
    s_data.timer = 10;
    
    enum STATUS res;

    while (res != FAILURE)
    {
        res = rpm_sequence.tick_cb(&rpm_sequence, &s_data);

    }
    // printf("Result: %s\n", res == SUCCESS ? "SUCCESS" : "FAILURE");

    return 0;
}
