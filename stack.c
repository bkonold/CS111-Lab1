#include "stack.h"
#include "stdlib.h"
#include "alloc.h"

void* 
pop(stk_t stk) {
    if (!stk || !stk->head)
        return NULL;
    node_t poppedNode = stk->head;
    stk->head = poppedNode->next;
    void* poppedItem = poppedNode->item;
    free(poppedNode);
    if (empty(stk))
        stk->tail = NULL;
    else
        stk->head->prev = NULL;
    return poppedItem;
}

int
empty(stk_t stk) {
    if (!stk)
        return 0;
    if (!stk->head)
        return 1;
    return 0;
}

void*
peek(stk_t stk) {
    if (!stk || !stk->head)
        return NULL;
    return stk->head->item;
}

void
push(stk_t stk, void* item) {
    if (stk) {
        node_t newNode = create_node(item, stk->head, NULL);
        if (empty(stk))
            stk->tail = newNode;
        else
            stk->head->prev = newNode;
        stk->head = newNode;
    }
}

void
push_front(stk_t stk, void* item) {
    push(stk, item);
}

void 
push_back(stk_t stk, void* item) {
    if (stk) {
        if (empty(stk)) 
            push(stk, item);
        else {
            node_t newNode = create_node(item, NULL, stk->tail);
            stk->tail->next = newNode;
            stk->tail = newNode;
        }
    }
}

int
size(stk_t stk) {
    if (!empty(stk)) {
        node_t currNode = stk->head;
        int size = 0;
        while (currNode) {
            size++;
            currNode = currNode->next;
        }
        return size;
    }
    return -1;
}

stk_t
create_stack() {
   stk_t newStack = checked_malloc(sizeof(struct stack));
   newStack->head = NULL;
   newStack->tail = NULL;
   return newStack;
}

stk_t
create_list() {
    return create_stack();
}

node_t
create_node(void* item, node_t next, node_t prev) {
    node_t newNode = checked_malloc(sizeof(struct node));
    newNode->next = next;
    newNode->prev = prev;
    newNode->item = item;
    return newNode;
}