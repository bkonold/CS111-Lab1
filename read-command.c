// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "string.h"
#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
// 

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

const char COMPLETE_CMD_DELIM = '~';
const void* OPEN_PAREN_COMMAND = NULL;

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

command_t
combine_command(command_t first, command_t second, command_t op) {
    op->u.command[0] = first;
    op->u.command[1] = second;
    return op;
}

int
precedence(command_t operator) {
    if (operator) {
        switch (operator->type) {
            case SEQUENCE_COMMAND:
                return 1;
            case AND_COMMAND:
            case OR_COMMAND:
                return 2;
            case PIPE_COMMAND:
                return 3;
            default:
                return 0;
        }
    }
    // add an error() here. "defensive programming"
    return -1;
}

/**
* replace 2 or more newlines with COMPLETE_CMD_DELIM
*/
void
replace_multiple_newlines(char* str) {
    if (str) {
        int length = strlen(str);
        int i;
        for (i = 0; i < length; i++) {
            // two consecutive newlines
            if ((str[i] == '\n') && (i != length-1) && ((str[i+1] == '\n'))) {
                str[i] = COMPLETE_CMD_DELIM;
                int j;
                // replace 2nd newline, and trailing whitespace, with delim
                for (j = i+1; str[j] == '\n' || str[j] == ' ' || str[j] == '\t'; j++) {
                    str[j] = COMPLETE_CMD_DELIM;
                }
                i = j-1;
            }
        }
    }
}

/**
* replace backslash newlines with backtick
*/
void 
replace_backslash_newline(char* str) {
    if (str) {
        int length = strlen(str);
        int i;
        for (i = 0; i < length; i++) {
            // two consecutive newlines
            if ((str[i] == '\\') && (i != length-1) && ((str[i+1] == '\n'))) {
                str[i+1] = '`';
                str[i] = '`';
                i++;
            }
        }
    }
}

/**
* replace any whitespace after an operator with spaces
*/
void
replace_whitespace_after_op(char* str) {
    if (str) {
        int length = strlen(str);
        int i;
        for (i = 0; i < length; i++) {
            // two consecutive newlines
            if (str[i] == '&' || str[i] == '|') {
                int j;
                for (j = i+1; str[j] == '\n' || str[j] == ' ' || str[j] == '\t'; j++) {
                    str[j] = ' ';
                }
                i = j-1;
            }
        }
    }
}

/**
* split up str by COMPLETE_CMD_DELIM and return an array of c-strings
*/
char**
tokenize_complete_cmds(char* str) {
    if (str) {
        int cmdCount = 0;
        int length = strlen(str);

        // determine size of char** array
        int i = 0;
        for (i = 0; i < length; ++i) {
            if (str[i] == COMPLETE_CMD_DELIM)
                cmdCount++;
        }

        char** cmdArray = (char**) malloc((cmdCount + 1) * sizeof(char*));

        // to find length later
        cmdArray[cmdCount] = NULL;

        int cmdIndex;
        char* complete_cmd = strtok (str, &COMPLETE_CMD_DELIM);

        // while there are tokens left, put in cmdArray
        for (cmdIndex = 0; complete_cmd != NULL; cmdIndex++) {
            cmdArray[cmdIndex] = complete_cmd;
            complete_cmd = strtok (NULL, &COMPLETE_CMD_DELIM);
        }

        return cmdArray;
    }
    else
        return NULL;
}

/**
* starting from *index, return next non-backtick character
*/
int
get_next_nonbacktick_char(char* str, int* index) {
    int c;
    do {
        c = str[(*index)++];
    } while (c == '`');
    return c;
}

/**
* starting from *index, return next non-whitespace character
*/
int
get_next_nonwhitespace_byte(char* str, int* index) {
    int c;
    do {
        c = str[(*index)++];
    } while (c == ' ' || c == '\t');
    return c;
}

char* 
file_to_str(int (*get_next_byte) (void *),
            void *get_next_byte_argument) {

    fseek(get_next_byte_argument, 0L, SEEK_END);
    size_t size = ftell(get_next_byte_argument);
    fseek(get_next_byte_argument, 0L, SEEK_SET);

    char* toReturn = (char*) malloc(size);

    int c = get_next_byte(get_next_byte_argument);

    int i = 0;
    while (c != EOF) {
        toReturn[i++] = c;
        c = get_next_byte(get_next_byte_argument);
    }
    toReturn[i] = '\0';

    return toReturn;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                     void *get_next_byte_argument)
{

         char* str = file_to_str(get_next_byte, get_next_byte_argument);
     printf("%s", str);
  /* FIXME: Replace this with your implementation.  You may need to
    add auxiliary functions and otherwise modify the source code.
    You can also use external functions defined in the GNU C Library. */

    /* validation */


    /* split by \n\n and put into array */

    /* loop through the array, making each a command tree */


    ////error (1, 0, "command reading not yet implemented");
    return 0;
}

command_t
read_command_stream (command_stream_t s)
{
    /* FIXME: Replace this with your implementation too.  */
    ////error (1, 0, "command reading not yet implemented");
    return 0;
}


/**********************/
/***     Tests      ***/
/**********************/

void
test_replace_backslash_newline() {
    char testArray[] = "ec\\\n\\\nho ghee";
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    replace_backslash_newline(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_replace_newlines() {
    char testArray[] = "a |\nb \n\n \n b && c || e | f \n\n\n\n";
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    replace_multiple_newlines(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_replace_whitespace_after_op() {
    char testArray[] = "a | \\\n \\\n \t \n \n b \n\n e || f | \\\n     g";
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    replace_whitespace_after_op(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_tokenize_complete_cmds() {
    //char testArray[] = "a || b \n\n \n c && \n d || e | f \n\n\n echo ghee \n\n";
    char testArray[] = 
    "true\n\ng++ -c foo.c\n\n: : :\n\n\n\n\ncat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!\n\na b<c > d\n\ncat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!\n\na&&b||\nc &&\n d | e && f|\n\ng<h\n\n# This is a weird example: nobody would ever want to run this.\na<b>c|d<e>f|g<h>i";

    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);

    replace_multiple_newlines(testArray);
    char** newArray = tokenize_complete_cmds(testArray);

    printf("COMMAND ARRAY\n----------------\n");
    int i;
    for (i = 0; newArray[i] != NULL; ++i) {
        printf("%s\t\t%d\n", newArray[i], i);
    }
}

// int 
// main(int argc, char const *argv[])
// {
//     // const char* script_name = argv[1];
//     // FILE *script_stream = fopen (script_name, "r");

//     // char* str = file_to_str(get_next_byte, script_stream);
//     // printf("%s", str);
//     test_tokenize_complete_cmds();
//     return 0;
// }
