# pico-bt

**Tiny header-only behavior tree library for embedded systems**

Single-file, zero dependencies, designed for microcontrollers and resource-constrained environments.

## Behavior Trees

Behavior trees execute from top to bottom, returning one of three states:
- `SUCCESS` - Task completed successfully
- `FAILURE` - Task failed
- `RUNNING` - Task in progress (continue next tick)

## Node Types

### Sequence

"AND" gate: Executes children left-to-right. Stops on first `FAILURE` or `RUNNING`.

```
SEQUENCE(A, B, C)
     |
  +--+--+
  A  B  C

Flow:
A=SUCCESS → B=SUCCESS → C=SUCCESS   → Returns SUCCESS
A=SUCCESS → B=FAILURE               → Returns FAILURE
A=SUCCESS → B=RUNNING               → Returns RUNNING
A=FAILURE                           → Returns FAILURE
```

**Use case:** Execute a series of actions that must all succeed (e.g., "check battery, move to target, pickup item").

### Selector

"OR" gate: Executes children until one succeeds. Stops on first `SUCCESS` or `RUNNING`.

```
SELECTOR(A, B, C)
     |
  +--+--+
  A  B  C

Flow:
A=FAILURE → B=FAILURE → C=SUCCESS   → Returns SUCCESS
A=FAILURE → B=SUCCESS               → Returns SUCCESS
A=SUCCESS                           → Returns SUCCESS
A=RUNNING                           → Returns RUNNING
```

**Use case:** Try alternatives until one works (e.g., "handle emergency, do work, idle behavior").


### Memory Sequence

Like `SEQUENCE`, but if a node returns `SUCCESS` the tree will skip it on the next frame. 


### Inverter

Flips `SUCCESS` and `FAILURE`, passes through `RUNNING`.

```
INVERTER(A)
    |
    A

A=SUCCESS → Returns FAILURE
A=FAILURE → Returns SUCCESS
A=RUNNING → Returns RUNNING
```

### Repeater

Returns `RUNNING` on child `SUCCESS`. Continues running unless a failure occurs.

```
REPEATER(A)
    |
    A

A=SUCCESS → Returns RUNNING (keep repeating)
A=FAILURE → Returns FAILURE
A=RUNNING → Returns RUNNING
```

**Use case:** Continuous behaviors (e.g., patrol route indefinitely).

## Context (Blackboard)

The context is a shared memory space accessible by all nodes in the tree, implementing the "blackboard" pattern common in behavior trees.

### How It Works

- The `CONTEXT` type is defined via preprocessor macro before including the header
- It can be any data structure you need - the library treats it as a void pointer
- All leaf functions receive a pointer to this context
- Nodes read from and write to the context to coordinate behavior

### Usage

```c
// Define your application-specific context
typedef struct {
    int battery;
    float position_x, position_y;
    bool target_found;
    void *sensor_data;
} MyContext;

// Tell pico-bt about your context type
#define CONTEXT MyContext
#include "pico_bt.h"

// All leaf functions can now access the context
enum STATUS check_battery(struct Node *self, CONTEXT *ctx) {
    return (ctx->battery > 20) ? SUCCESS : FAILURE;
}

enum STATUS update_position(struct Node *self, CONTEXT *ctx) {
    ctx->position_x += 0.1;
    ctx->position_y += 0.1;
    return SUCCESS;
}
```

### Default Behavior

If you don't define `CONTEXT`, it defaults to `void*`:

```c
#include "pico_bt.h"  // CONTEXT is void* by default
```

This allows maximum flexibility - you can cast it to whatever you need in each leaf function.

## Reference

### Macros

```c
LEAF(function)                    // Create a leaf node
SEQUENCE(child1, child2, ...)     // Standard sequence (resets each tick)
MEM_SEQUENCE(child1, child2, ...) // Memory sequence (remembers position)
SELECTOR(child1, child2, ...)     // Fallback selector
INVERTER(child)                   // NOT decorator
REPEATER(child)                   // Infinite loop decorator
```

### Leaf Function Signature

```c
enum STATUS my_behavior(struct Node *self, CONTEXT *ctx) {
    // Access context: ctx->your_field
    // Modify state: ctx->your_field = value
    return SUCCESS; // or FAILURE or RUNNING
}
```
