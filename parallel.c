#include "alloc.h"
#include "command-internals.h"
#include "parallel.h"
#include "stack.h"
#include <string.h>

void
build_io_lists(command_t cmd, string_list_t readList, string_list_t writeList) {
	if (cmd->type == SIMPLE_COMMAND) {
		int i = 0;
		if (strcmp(cmd->u.word[0], "exec") == 0)
			i = 1;

		while (cmd->u.word[i]) {
			if (cmd->u.word[i][0] != '-') {
				push_back(readList, cmd->u.word[i]);
			}
		}
		push_back(writeList, cmd->output);
	}
	else if (cmd->type == SUBSHELL_COMMAND) {
		push_back(readList, cmd->input);
		push_back(writeList, cmd->output);
		build_io_lists(cmd->u.subshell_command, readList, writeList);
	}
	else {
		build_io_lists(cmd->u.command[0], readList, writeList);
		build_io_lists(cmd->u.command[1], readList, writeList);
	}
}

bool
intersect(string_list_t readList, string_list_t writeList) {
	if (empty(readList) || empty(writeList))
		return false;

	node_t currReadNode = peek(readList);
	while (currReadNode) {

		char* currReadWord = currReadNode->item;
		node_t currWriteNode = peek(writeList);

		while (currWriteNode) {
			char* currWriteWord = currWriteNode->item;

			if (strcmp(currReadWord, currWriteWord) == 0)
				return true;

			currWriteNode = currWriteNode->next;
		}

		currReadNode = currReadNode->next;
	}

	return false;
}

void
build_dependency_graph(graph_t graph, graphnode_list_t adjList, command_t cmd, string_list_t readList, string_list_t writeList) {
	graphnode_t newNode = checked_malloc(sizeof(struct graphnode));
	newNode->cmd = cmd;
	newNode->readList = readList;
	newNode->writeList = writeList;

	node_t currNode = adjList->head;
	while (currNode) {
		graphnode_t currGraphNode = currNode->item;

		if (intersect(newNode->readList, currGraphNode->writeList)     /* read after write  */
			|| intersect(newNode->writeList, currGraphNode->writeList) /* write after write */
			|| intersect(newNode->writeList, currGraphNode->readList)  /* write after read  */) {

			push_back(newNode->dependencies, currGraphNode);
		}
	}

	push_back(adjList, newNode);
	if (empty(newNode->dependencies))
		push_back(graph->independent, newNode);
	else
		push_back(graph->dependent, newNode);

}