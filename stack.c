#include "stack.h"
#include "stdlib.h"
#include "alloc.h"

command_t 
pop(cmd_stk_t stk) {
    if (!stk || !stk->head)
        return NULL;
    node_t poppedNode = stk->head;
    stk->head = poppedNode->next;
    command_t poppedCmd = poppedNode->cmd;
    free(poppedNode);
    if (empty(stk))
        stk->tail = NULL;
    else
        stk->head->prev = NULL;
    return poppedCmd;
}

int
empty(cmd_stk_t stk) {
    if (!stk)
        return 0;
    if (!stk->head)
        return 1;
    return 0;
}

command_t
peek(cmd_stk_t stk) {
    if (!stk || !stk->head)
        return NULL;
    return stk->head->cmd;
}

command_t 
getFirst(cmd_stk_t stk) {
    return peek(stk);
}

void
push(cmd_stk_t stk, command_t cmd) {
    if (stk) {
        node_t newNode = create_node(cmd, stk->head, NULL);
        if (empty(stk))
            stk->tail = newNode;
        else
            stk->head->prev = newNode;
        stk->head = newNode;
    }
}

void 
push_back(cmd_stk_t stk, command_t cmd) {
    if (stk) {
        if (empty(stk)) 
            push(stk, cmd);
        else {
            node_t newNode = create_node(cmd, NULL, stk->tail);
            stk->tail->next = newNode;
            stk->tail = newNode;
        }
    }
}

cmd_stk_t
create_stack() {
   cmd_stk_t newStack = (cmd_stk_t) checked_malloc(sizeof(struct stack));
   newStack->head = NULL;
   newStack->tail = NULL;
   return newStack;
}

node_t
create_node(command_t cmd, node_t next, node_t prev) {
    node_t newNode = (node_t) checked_malloc(sizeof(struct node));
    newNode->next = next;
    newNode->prev = prev;
    newNode->cmd = cmd;
    return newNode;
}