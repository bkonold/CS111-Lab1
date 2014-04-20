#ifndef PARALLEL_H
#define PARALLEL_H

#include "stack.h"
#include "command.h"

typedef struct stack* graphnode_list_t;
typedef struct stack* string_list_t;

struct graphnode {
	command_t cmd;
	graphnode_list_t dependencies;
	string_list_t readList;
	string_list_t writeList;
};

struct graph {
	graphnode_list_t independent;
	graphnode_list_t dependent;
};

typedef struct graph* graph_t;
typedef struct graphnode* graphnode_t;

void build_io_lists(command_t, string_list_t, string_list_t);

void build_dependency_graph(graph_t graph, graphnode_list_t, command_t, string_list_t, string_list_t);

#endif