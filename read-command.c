// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "string.h"
#include "alloc.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
// 

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
enum operator_type {
    OR,
    AND,
    PIPE,
    SEQUENCE,
    INDIRECT,
    OUTDIRECT,
    NONE,
};

// **************** TODO CHANGE MALLOC TO CHECKED_MALLOC ***************

const char COMPLETE_CMD_DELIM = '~';
const void* OPEN_PAREN_COMMAND = NULL;

static int
get_next_byte (void *stream) {
    return getc (stream);
}

/**
* print error message and exit
*/
void
error_and_quit(char* message, int lineNum) {
    printf("Error at %d: %s\n", lineNum, message);
    exit(1);
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
* reads the content of get_next_byte_argument into a c-string
*/
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

/**
* combines first and second into a single command (op)
*/
command_t
combine_command(command_t first, command_t second, command_t op) {
    op->u.command[0] = first;
    op->u.command[1] = second;
    return op;
}

/**
* returns an int corresponding to the precedence of operator
*/
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
* called when an operator is found when building a command tree
*/
void
push_new_operator(command_t op, cmd_stk_t cmdStack, cmd_stk_t opStack) {
    if (empty(opStack) || (precedence(op) > precedence(peek(opStack)))) {
        push(opStack, op);
    } else {
        while (peek(opStack) != OPEN_PAREN_COMMAND && precedence(op) <= precedence(peek(opStack))) {
            command_t topOperator = pop(opStack);
            command_t secondCmd = pop(cmdStack);
            command_t firstCmd = pop(cmdStack);

            command_t combinedCmd = combine_command(firstCmd, secondCmd, topOperator);
            push(cmdStack, combinedCmd);

            if (empty(opStack)) 
                break;
        }
        push(opStack, op);
    }
}

/**
* called when a close paren is found when building a command tree
*/
void 
handle_close_paren(cmd_stk_t cmdStack, cmd_stk_t opStack) {
    command_t topOperator = pop(opStack);

    while (topOperator != OPEN_PAREN_COMMAND) {
        command_t secondCmd = pop(cmdStack);
        command_t firstCmd = pop(cmdStack);
        command_t combinedCmd = combine_command(firstCmd, secondCmd, topOperator);

        push(cmdStack, combinedCmd);
        topOperator = pop(opStack);
    }

    command_t subshellCmd = (command_t) malloc(sizeof(struct command));

    subshellCmd->type = SUBSHELL_COMMAND;
    subshellCmd->input = subshellCmd->output = NULL;
    subshellCmd->u.subshell_command = pop(cmdStack);

    push(cmdStack, subshellCmd);
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
get_next_nonwhitespace_char(char* str, int* index) {
    int c;
    do {
        c = get_next_nonbacktick_char(str, index);
    } while (c == ' ' || c == '\t');
    return c;
}


/**
* returns true if c is alphanumeric or any of the following: ! % + , - . / : @ ^ _
*/
bool
is_valid_word_char(int c) {
    if (isalnum(c) || c == '!' || c == '%' || c == '+' || c == ',' || c == '-' 
        || c == '.'  || c == '/' || c == ':' || c == '@' || c == '^' || c == '_') {
        return true;
    }
    return false;
}


/**
* validate the input for valid POSIX shell syntax. fail if not
*/
// TODO: what if str is empty (or just whitespace) ?
void
validate(char* str) {
    if (str) {
        int index = 0;
        int lineNum = 1;
        int unmatchedParenCount = 0;

        while (str[index] == ' ' || str[index] == '\t' || str[index] == '\n')
            index++;

        if (str[index] != '(' && !is_valid_word_char(str[index])) {
            error_and_quit("Script starts with invalid char", lineNum);
            return;
        }

        bool inWordNow = false;
        enum operator_type lastSeenOp = NONE;
        
        while (str[index]) {
            if (is_valid_word_char(str[index])) {
                if (lastSeenOp == INDIRECT || lastSeenOp == OUTDIRECT) {
                    error_and_quit("Too many arguments to I/O redirect", lineNum);
                    return;
                }

                inWordNow = true;
            }
            else if (str[index] == '&') {
                if (!inWordNow) {
                    error_and_quit("Too few operands", lineNum);
                    return;
                }

                if (str[index+1] != '&') {
                    error_and_quit("Invalid operator found", lineNum);
                    return;
                }

                lastSeenOp = AND;
                index++;
                inWordNow = false;
            }
            else if (str[index] == '|') {
                if (!inWordNow) {
                    error_and_quit("Too few operands", lineNum);
                    return;
                }

                // found OR
                if (str[index+1] == '|') {
                    lastSeenOp = OR;
                    index++;
                } 
                // found PIPE
                else {
                    lastSeenOp = PIPE;
                }

                inWordNow = false;
            }
            else if (str[index] == '<') {
                if (!inWordNow) {
                    error_and_quit("Too few operands", lineNum);
                    return;
                }

                if (lastSeenOp == INDIRECT || lastSeenOp == OUTDIRECT) {
                    error_and_quit("Invalid use of input redirect", lineNum);
                    return;
                }

                do {
                    index++;
                } while (str[index] == ' ' || str[index] == '\t');

                if (!is_valid_word_char(str[index])) {
                    error_and_quit("No file specified", lineNum);
                    return;
                }

                do {
                    index++;
                } while (is_valid_word_char(str[index]));

                /* inWordNow is left as true */

                lastSeenOp = INDIRECT;
                continue;
            }
            else if (str[index] == '>') {
                if (!inWordNow) {
                    error_and_quit("Too few operands", lineNum);
                    return;
                }

                if (lastSeenOp == OUTDIRECT) {
                    error_and_quit("Output redirect cannot follow output redirect", lineNum);
                    return;
                }

                do {
                    index++;
                } while (str[index] == ' ' || str[index] == '\t');

                if (!is_valid_word_char(str[index])) {
                    error_and_quit("No file specified", lineNum);
                    return;
                }

                do {
                    index++;
                } while (is_valid_word_char(str[index]));

                lastSeenOp = OUTDIRECT;
                continue;                

                /* inWordNow is left as true */

            }
            else if (str[index] == ';') {
                if (!inWordNow) {
                    error_and_quit("Too few operands", lineNum);
                    return;
                }

                lastSeenOp = SEQUENCE;                
                inWordNow = false;
            }
            else if (str[index] == '(') {
                if (inWordNow) {
                    error_and_quit("Too few operators", lineNum);
                    return;
                }

                unmatchedParenCount++;

                inWordNow = false;

            }
            else if (str[index] == ')') {
                if (!inWordNow) {
                    error_and_quit("Too few operands", lineNum);
                    return;
                }

                unmatchedParenCount--;

                if (unmatchedParenCount < 0) {
                    error_and_quit("Unmatched parentheses", lineNum);
                    return;
                }

                /* inWordNow is left as true */
                
            }
            else if (str[index] == '\t' || str[index] == ' ') {
                /* do nothing */
            }
            else if (str[index] == '#') {
                do {
                    index++;
                } while (str[index] && str[index] != '\n');
                
                continue;
            }
            else if (str[index] == '\n') {
                // delimeter for complete command
                if (inWordNow && str[index+1] == '\n') {
                    lineNum++;
                    do {
                        index++;
                        if (str[index] == '\n')
                            lineNum++;
                    } while (str[index] == '\n' || str[index] == ' ' || str[index] == '\t');     
                    if (str[index] != '(' && !is_valid_word_char(str[index]) && str[index] != '#') {
                        error_and_quit("Script starts with invalid char", lineNum);
                    }
                    bool inWordNow = false;
                    lastSeenOp = NONE;
                    continue;
                }
                if (inWordNow)
                    lastSeenOp = SEQUENCE;
                inWordNow = false;
                lineNum++;
            }
            // invalid character
            else {
                error_and_quit("Invalid character found ", lineNum);
                return;
            }
            index++;
        }

        if (!inWordNow) {
            error_and_quit("Too few operands", lineNum);
            return;
        }

        if (unmatchedParenCount > 0) {
            error_and_quit("Unmatched parentheses", lineNum);
            return;
        }

    }
    printf("VALID AS FUCK\n");
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                     void *get_next_byte_argument)
{

//THESE SHOULD FAIL
/*    char** test = (char *[]){
        "`",
        ">",
        "<",
        "a >b <",
        ";",
        "; a",
        "a ||",
        "a\n|| b",
        "a\n| b",
        "a\n; b",
        "a;;b",
        "a&&&b",
        "a|||b",
        "|a",
        "< a",
        "&& a",
        "||a",
        "(a|b",
        "a;b)",
        "( (a)",
        "a>>>b"
    };*/

    //char* test = "true\n\ng++ -c foo.c\n\n: : :\n\ncat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!\n\na b<c > d\n\ncat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!\n\na&&b||\n c &&\n  d | e && f|\n\ng<h\n\n# This is a weird example: nobody would ever want to run this.\na<b>c|d<e>f|g<h>i";
    char* test = "echo b && #lol\nb";
    char* test3 = "echo b && \n\n\n #g;e \necho ghee";
    //char* test = "a || \n\n b";


    // TODO-TUAN
    char* test2 = "a \n      \n b";
    validate(test);
    
  /* FIXME: Replace this with your implementation.  You may need to
    add auxiliary functions and otherwise modify the source code.
    You can also use external functions defined in the GNU C Library. */

    /* cleanup */

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
    printf("TEST REPLACE BACKSLASH NEWLINE\n--------------------------------\n\n");
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    replace_backslash_newline(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_replace_multiple_newlines() {
    char testArray[] = "a |\nb \n\n \n b && c || e | f \n\n\n\n";
    printf("TEST REPLACE MULTIPLE NEWLINES\n--------------------------------\n\n");
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    replace_multiple_newlines(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_replace_whitespace_after_op() {
    char testArray[] = "a | \n \n \t \n \n b \n\n e || f | \n     g";
    printf("TEST REPLACE WHITESPACE AFTER OP\n--------------------------------\n\n");
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    replace_whitespace_after_op(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_tokenize_complete_cmds() {
    char testArray[] = "a || b \n\n \n c && \n d || e | f \n\n\n echo ghee \n\n";
    //char testArray[] = 
    //"true\n\ng++ -c foo.c\n\n: : :\n\n\n\n\ncat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!\n\na b<c > d\n\ncat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!\n\na&&b||\nc &&\n d | e && f|\n\ng<h\n\n# This is a weird example: nobody would ever want to run this.\na<b>c|d<e>f|g<h>i";
    printf("TEST TOKENIZE COMPLETE COMMANDS\n--------------------------------\n\n");
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);

    replace_multiple_newlines(testArray);
    replace_whitespace_after_op(testArray);
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

//     char* test = "`";

//     validate(test);

//     return 0;
// }
