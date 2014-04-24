#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include "parallel.h"
#include "stack.h"
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static bool* sharedFinished;

void
build_io_lists(command_t cmd, string_list_t readList, string_list_t writeList) {
	if (cmd->type == SIMPLE_COMMAND) {
		int i = 1;
		if (strcmp(cmd->u.word[0], "exec") == 0)
			i = 2;

		while (cmd->u.word[i]) {
			if (cmd->u.word[i][0] != '-') {
				push_back(readList, cmd->u.word[i]);
			}
			i++;
		}
		if (cmd->output)
			push_back(writeList, cmd->output);
		if (cmd->input)
			push_back(readList, cmd->input);
	}
	else if (cmd->type == SUBSHELL_COMMAND) {
		if (cmd->input)
			push_back(readList, cmd->input);
		if (cmd->output)
			push_back(writeList, cmd->output);
		build_io_lists(cmd->u.subshell_command, readList, writeList);
	}
	else {
		build_io_lists(cmd->u.command[0], readList, writeList);
		build_io_lists(cmd->u.command[1], readList, writeList);
	}
}

bool
intersect(string_list_t first, string_list_t second) {
	if (empty(first) || empty(second))
		return false;

	node_t currReadNode = first->head;
	while (currReadNode) {

		char* currReadWord = currReadNode->item;
		node_t currWriteNode = second->head;

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
add_to_graph(graphnode_list_t adjList, command_t cmd, string_list_t readList, string_list_t writeList) {
	graphnode_t newNode = checked_malloc(sizeof(struct graphnode));
	newNode->cmd = cmd;
	newNode->readList = readList;
	newNode->writeList = writeList;
	newNode->aid = -1;
	newNode->dependencies = create_list();

	node_t currNode = adjList->head;
	while (currNode) {
		graphnode_t currGraphNode = currNode->item;

		if (intersect(newNode->readList, currGraphNode->writeList)     /* read after write  */
			|| intersect(newNode->writeList, currGraphNode->writeList) /* write after write */
			|| intersect(newNode->writeList, currGraphNode->readList)  /* write after read  */) {

			push_back(newNode->dependencies, currGraphNode);
		}

		currNode = currNode->next;
	}

	push_back(adjList, newNode);

}

void execute_parallel(command_stream_t commandStream) {
	graphnode_list_t adjList = create_list();
	command_t cmd;
	while ((cmd = read_command_stream (commandStream))) {
		string_list_t readList = create_list(); 
		string_list_t writeList = create_list();
		build_io_lists(cmd, readList, writeList);
		add_to_graph(adjList, cmd, readList, writeList);
	}

	int adjListSize = size(adjList);

	sharedFinished = mmap(NULL, adjListSize * sizeof(bool), 
				    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

	int i;
	for (i = 0; i < adjListSize; i++) {
		sharedFinished[i] = false;
	}


    unsigned int numStartedProcesses = 0;

	i = 0;
	node_t currNode = adjList->head;
	while (currNode) {
		graphnode_t currGraphNode = currNode->item;
		currGraphNode->aid = i;

        if (numStartedProcesses >= MAX_PROCS) {
            // wait for a spot to open up
            wait(NULL);
        }

		pid_t p = fork();

		if (p == 0) {
			execute_node(currGraphNode);
		}
		else if (p > 0) {
            numStartedProcesses++;
			currNode = currNode->next;
		}
		i++;
	}
	while (true) {
		int status;
		pid_t done = wait(&status);
		if (done == -1 && errno == ECHILD) {
			break;
		}
	}
}

void
execute_node(graphnode_t node) {
	// wait for dependencies to finish
	node_t currNode = node->dependencies->head;
	while (currNode) {
		graphnode_t currDependencyNode = currNode->item;

		while(!sharedFinished[currDependencyNode->aid])
			continue;

		currNode = currNode->next;
	}
	execute_command(node->cmd);
	sharedFinished[node->aid] = true;
	exit(node->cmd->status);
}
