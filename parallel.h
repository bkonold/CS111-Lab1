#ifndef PARALLEL_H
#define PARALLEL_H

#include "stack.h"
#include "command.h"
#include <stdlib.h>

typedef struct stack* graphnode_list_t;
typedef struct stack* string_list_t;

struct graphnode {
	command_t cmd;
	graphnode_list_t dependencies;
	string_list_t readList;
	string_list_t writeList;
	pid_t pid;
};

struct graph {
	graphnode_list_t independent;
	graphnode_list_t dependent;
};

typedef struct graph* graph_t;
typedef struct graphnode* graphnode_t;

void build_io_lists(command_t, string_list_t, string_list_t);

void add_to_graph(/*graph_t graph, */graphnode_list_t, command_t, string_list_t, string_list_t);

void execute_node(graphnode_t node);

void execute_parallel(command_stream_t commandStream);

#endif