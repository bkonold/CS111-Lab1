#include "command-internals.h"
#ifndef STACK_H
#define STACK_H

typedef struct command *command_t;

struct node {
    struct node* next;
    struct node* prev;
    command_t cmd;
};

struct stack {
    struct node* head;
    struct node* tail;
};

typedef struct node* node_t;

typedef struct stack* cmd_stk_t;

cmd_stk_t create_stack(void);

node_t create_node(command_t cmd, node_t next, node_t prev);

command_t pop(cmd_stk_t stk);

command_t peek(cmd_stk_t stk);

command_t getFirst(cmd_stk_t stk);

void push(cmd_stk_t stk, command_t cmd);

void push_back(cmd_stk_t, command_t cmd);

int empty(cmd_stk_t stk);

#endif