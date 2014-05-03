// UCLA CS 111 Lab 1 main program

#include <errno.h>

#include <getopt.h>
#include <stdio.h>

#include "command.h"
#include "parallel.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
    printf("usage: %s [-pt] SCRIPT-FILE", program_name);
    exit(1);
}

static int
get_next_byte (void *stream)
{
    return getc (stream);
}

int
main (int argc, char **argv)
{
    int command_number = 1;
    bool print_tree = false;
    bool time_travel = false;
    program_name = argv[0];

    for (;;)
        switch (getopt (argc, argv, "pt")) {
            case 'p': print_tree = true; break;
            case 't': time_travel = true; break;
            default: usage (); break;
            case -1: goto options_exhausted;
        }
    options_exhausted:;

    // There must be exactly one file argument.
    if (optind != argc - 1)
        usage ();

    script_name = argv[optind];
    FILE *script_stream = fopen (script_name, "r");
    if (! script_stream) {
        printf("%s: cannot open", script_name);
        exit(1);
    }
    command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);

    command_t last_command = NULL;
    command_t command;
    if (!time_travel)
        while ((command = read_command_stream (command_stream))) {
            if (print_tree) {
                printf ("# %d\n", command_number++);
                print_command (command);
            }
            else if (!time_travel) {
                last_command = command;
                execute_command (command);
            }
        }
    else
        execute_parallel(command_stream);

    return print_tree || !last_command ? 0 : command_status (last_command);
}
