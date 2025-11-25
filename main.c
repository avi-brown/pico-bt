#include <stdio.h>
#include <unistd.h> /* for sleep */

#define PICO_BT_IMPLEMENTATION
#include "pico_bt.h"

/* --- Context --- */

typedef struct 
{
    int battery;
    int enemy_visible; 
} RobotContext;

/* --- Leaf Nodes --- */

/* Condition: Returns SUCCESS if battery is > 30 */
enum STATUS check_battery_good(struct Node * self, void * arena) 
{
    RobotContext * bot = (RobotContext *)arena;
    if (bot->battery > 30) return YES;
    return NO;
}

/* Action: Charges battery. Returns RUNNING until 100 */
enum STATUS action_recharge(struct Node * self, void * arena) 
{
    RobotContext * bot = (RobotContext *)arena;
    printf("[Action] Recharging... (%d%%)\n", bot->battery);
    bot->battery += 10;
    if (bot->battery >= 100) 
    {
        bot->battery = 100;
        printf("[Action] Battery Full.\n");
        return SUCCESS;
    }
    return RUNNING;
}

/* Condition: Returns SUCCESS if enemy flag is set */
enum STATUS check_enemy(struct Node * self, void * arena) 
{
    RobotContext * bot = (RobotContext *)arena;
    if (bot->enemy_visible) return YES;
    return NO;
}

/* Action: Attacks. Drains battery slightly */
enum STATUS action_attack(struct Node * self, void * arena) 
{
    RobotContext * bot = (RobotContext *)arena;
    printf("[Action] ATTACKING ENEMY!\n");
    bot->battery -= 5; 
    return SUCCESS;
}

/* Action: Patrols. Drains battery */
enum STATUS action_patrol(struct Node * self, void * arena) 
{
    RobotContext * bot = (RobotContext *)arena;
    printf("[Action] Patrolling area...\n");
    bot->battery -= 2; 
    return SUCCESS;
}

/* --- Main --- */

int main() 
{
    RobotContext bot = { 100, 0 }; /* Full battery, no enemy */

    /* 
       Tree Structure:
       Root (Selector)
        |-- SelfPreservation (Sequence)
        |    |-- Inverter (Not Good Battery?) -> check_battery_good
        |    |-- action_recharge
        |
        |-- Combat (Sequence)
        |    |-- check_enemy
        |    |-- action_attack
        |
        |-- action_patrol
    */

    struct Node * root =
        SELECTOR(

            SEQUENCE(
                INVERTER(
                    LEAF(check_battery_good)
                ),
                LEAF(action_recharge)
            ),

            SEQUENCE(
                LEAF(check_enemy),
                LEAF(action_attack)
            ),

            LEAF(action_patrol)

        );

    /* Simulation Loop */
    printf("Starting Robot Logic...\n");

    for (int i = 0; i < 60; i++) 
    {
        printf("\nTick %d [Batt: %d | Enemy: %d]: ", i, bot.battery, bot.enemy_visible);
        
        /* Simulate random enemy appearance at tick 5, gone at 10 */
        if (i == 5) bot.enemy_visible = 1;
        if (i == 10) bot.enemy_visible = 0;

        enum STATUS status = root->tick_cb(root, &bot);
        
        /* Speed up drain to test recharge logic sooner */
        if (bot.battery > 50 && status != RUNNING) bot.battery -= 5; 

        sleep(1); 
    }

    return 0;
}