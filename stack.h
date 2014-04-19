#include "command-internals.h"
#ifndef STACK_H
#define STACK_H

/* stack holds void* so as to be more generic */

struct node {
    struct node* next;
    struct node* prev;
    void* item;
};

struct stack {
    struct node* head;
    struct node* tail;
};

typedef struct node* node_t;

typedef struct stack* stk_t;

stk_t create_stack(void);

stk_t create_list(void);

node_t create_node(void* cmd, node_t next, node_t prev);

void* pop(stk_t stk);

void* peek(stk_t stk);

void* getFirst(stk_t stk);

void push(stk_t stk, void* cmd);

void push_front(stk_t, void* cmd);

void push_back(stk_t, void* cmd);

int empty(stk_t stk);

#endif