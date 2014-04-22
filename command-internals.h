#ifndef COMMAND_INTERNALS_H
#define COMMAND_INTERNALS_H

#include "stack.h"

// UCLA CS 111 Lab 1 command internals

enum command_type
{
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
};

enum output_permission
{
    WRITE,
    OVERWRITE,
    APPEND,
};

struct fdpair {
    int from;
    int to;
};

typedef struct fdpair* fdpair_t;

typedef struct stack* fdpair_list_t;

// Data associated with a command.
struct command
{
    enum command_type type;

    // Exit status, or -1 if not known (e.g., because it has not exited yet).
    int status;

    // I/O redirections, or null if none.
    char *input;
    char *output;

    enum output_permission outPerm;

    fdpair_list_t outputClones;
    fdpair_list_t inputClones;

    union
    {
        // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
        struct command *command[2];

        // for SIMPLE_COMMAND:
        char **word;

        // for SUBSHELL_COMMAND:
        struct command *subshell_command;
    } u;
};

#endif