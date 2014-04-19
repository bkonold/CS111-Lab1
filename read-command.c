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
    CLOSE_PAREN,
    NONE,
};

// **************** TODO CHANGE checked_malloc TO CHECKED_MALLOC ***************

const char* COMPLETE_CMD_DELIM_STR = "~";
const command_t OPEN_PAREN_COMMAND = NULL;

/**
* print error message and exit
*/
void
error_and_quit(char* message, int lineNum) {
    fprintf(stderr, "%d: %s\n", lineNum, message);
    exit(1);
}

/**
* replace all whitespace before ')'
*/
void
replace_whitespace_before_close_paren(char* str) {
    if (str) {
        int length = strlen(str) - 1;
        int i;
        for (i = length; i > 0; i--) {
            // two consecutive newlines
            if (str[i] == ')') {
                int j;
                // replace all whitespace characters preceeding a close paren with spaces
                for (j = i-1; str[j] == '\n' || str[j] == ' ' || str[j] == '\t'; j--) {
                    str[j] = ' ';
                }
                i = j + 1;
            }
        }
    }
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
                str[i] = COMPLETE_CMD_DELIM_STR[0];
                int j;
                // replace 2nd newline, and trailing whitespace, with delim
                for (j = i+1; str[j] == '\n' || str[j] == ' ' || str[j] == '\t'; j++) {

                    str[j] = COMPLETE_CMD_DELIM_STR[0];
                }
                i = j-1;
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
            // ignore comments
            if (str[i] == '#') {
                do {
                    i++;
                } while (str[i] && str[i] != '\n');
                i--;
            }
            // two consecutive newlines
            if (str[i] == '&' || str[i] == '|' || str[i] == '(' || str[i] == ';') {
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
        int cmdCount = 1;
        int length = strlen(str);

        // determine size of char** array
        int i = 0;
        for (i = 0; i < length; ++i) {
            if (str[i] == COMPLETE_CMD_DELIM_STR[0]) {
                cmdCount++;
                while (str[i] == COMPLETE_CMD_DELIM_STR[0]) {
                    i++;
                }
                continue;
            }
        }

        char** cmdArray = checked_malloc((cmdCount + 1) * sizeof(char*));

        // to find length later
        cmdArray[cmdCount] = NULL;

        int cmdIndex;
        char* complete_cmd = strtok (str, COMPLETE_CMD_DELIM_STR);
        // while there are tokens left, put in cmdArray
        for (cmdIndex = 0; complete_cmd != NULL && cmdIndex < cmdCount; cmdIndex++) {
            cmdArray[cmdIndex] = complete_cmd;
            //printf("%s\n", complete_cmd);
            complete_cmd = strtok (NULL, COMPLETE_CMD_DELIM_STR);
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

    char* toReturn = checked_malloc(size+1);

    int c = get_next_byte(get_next_byte_argument);

    int i = 0;
    while (c != EOF) {
        toReturn[i++] = c;
        c = get_next_byte(get_next_byte_argument);
    }
    toReturn[i] = '\0';
    i--;
    while (toReturn[i] == ' ' || toReturn[i] == '\t' || toReturn[i] == '\n') {
        toReturn[i] = '\0';
        i--;
    }
    i = 0;
    while (toReturn[i] && (toReturn[i] == '\n' || toReturn[i] == '\t' || toReturn[i] == ' '))
        toReturn[i++] = ' ';

    return toReturn;
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
validate(const char* str) {
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
                } else if (lastSeenOp == CLOSE_PAREN) {
                    error_and_quit("Too few operators", lineNum);
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

                lastSeenOp = CLOSE_PAREN;
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
            else if (str[index] == '\\') {
                if (str[index+1] != '\n') {
                    error_and_quit("", lineNum);
                }
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
                    inWordNow = false;
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
    //printf("ABSOLUTELY VALID\n");
}

/**
* starting from *index, return next non-whitespace character
*/
int
get_next_nonwhitespace_char(const char* str, int* index) {
    int c;

    do {
        (*index)++;
        c = str[(*index)];
    } while (c == ' ' || c == '\t');

    return c;
}

/**
* return the next word starting from index
*/
char*
get_next_word(const char* str, int* index) {
    int wordEndIndex = *index;

    while (is_valid_word_char(str[wordEndIndex])) {
        wordEndIndex++;
    }

    //wordEndIndex now just past the word
    char* word = checked_malloc((wordEndIndex - (*index)) + 1);

    int i;
    for (i = *index; i < wordEndIndex; i++)
        word[i-(*index)] = str[i];
    word[i-(*index)] = '\0';

    // skip over whitespace to start of next token
    while (str[wordEndIndex] == ' ' || str[wordEndIndex] == '\t') {
        wordEndIndex++;
    }

    *index = wordEndIndex;

    return word;
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

    command_t subshellCmd = checked_malloc(sizeof(struct command));

    subshellCmd->type = SUBSHELL_COMMAND;
    subshellCmd->input = subshellCmd->output = NULL;
    subshellCmd->u.subshell_command = pop(cmdStack);

    push(cmdStack, subshellCmd);
}

/**
* given a string corresponding to a valid complete command, build a command tree
*/
command_t
parse_complete_command(const char* str) {
    int index = -1;

    cmd_stk_t cmdStack = create_stack();
    cmd_stk_t opStack = create_stack();

    get_next_nonwhitespace_char(str, &index);

    while(str[index]) {

        //printf("%c\t%d\n", str[index], index);

        if (str[index] == '(') {
            push(opStack, OPEN_PAREN_COMMAND);
            get_next_nonwhitespace_char(str, &index);
        } 
        else if (str[index] == '<') {
            get_next_nonwhitespace_char(str, &index);
            char *inputFileName = get_next_word(str, &index);
            command_t cmd = pop(cmdStack);
            cmd->input = inputFileName;
            push(cmdStack, cmd);
        }
        else if (str[index] == '>') {
            get_next_nonwhitespace_char(str, &index);
            char *outputFileName = get_next_word(str, &index);
            command_t cmd = pop(cmdStack);
            cmd->output = outputFileName;
            push(cmdStack, cmd);
        }
        // simple command
        else if (is_valid_word_char(str[index])) {

            char** words = NULL;
            int wordCount = 0;
            
            while (is_valid_word_char(str[index])) {
                wordCount++;
                // +1 to leave space for '\0' at end
                words = checked_realloc(words, (wordCount+1)* sizeof(char*));

                char* word = get_next_word(str, &index);

                words[wordCount-1] = word;
            }
            words[wordCount] = NULL;

            command_t cmd = checked_malloc(sizeof(struct command));
            cmd->type = SIMPLE_COMMAND;
            cmd->u.word = words;
            cmd->status = -1;
            cmd->input = NULL;
            cmd->output = NULL;

            push(cmdStack, cmd);
            continue;
        }
        else if (str[index] == ')') {
            handle_close_paren(cmdStack, opStack);
            get_next_nonwhitespace_char(str, &index);
            continue;
        } 
        else if (str[index] == '&') {
            command_t op = checked_malloc(sizeof(struct command));
            op->type = AND_COMMAND;
            op->status = -1;
            op->input = NULL;
            op->output = NULL;

            push_new_operator(op, cmdStack, opStack);

            index++;
            get_next_nonwhitespace_char(str, &index);
            continue;
        }
        else if (str[index] == '|') {
            command_t op = checked_malloc(sizeof(struct command));
            op->type = PIPE_COMMAND;

            if (str[index+1] == '|') {
                 op->type = OR_COMMAND;
                 index++;
            }

            op->status = -1;
            op->input = NULL;
            op->output = NULL;

            push_new_operator(op, cmdStack, opStack);

            get_next_nonwhitespace_char(str, &index);
            continue;
        }
        else if (str[index] == ';' || str[index] == '\n') {
            command_t op = checked_malloc(sizeof(struct command));
            op->type = SEQUENCE_COMMAND;
            op->status = -1;
            op->input = NULL;
            op->output = NULL;

            push_new_operator(op, cmdStack, opStack);

            get_next_nonwhitespace_char(str, &index);
            continue;
        }
        else if (str[index] == '#') {
            do {
                index++;
            } while (str[index] && str[index] != '\n');
            
            get_next_nonwhitespace_char(str, &index);
            continue;
        }
    }

    //combine all operators sitting on opStack
    while (!empty(opStack)) {
        command_t topOperator = pop(opStack);
        command_t secondCmd = pop(cmdStack);
        command_t firstCmd = pop(cmdStack);

        command_t combinedCmd = combine_command(firstCmd, secondCmd, topOperator);
        push(cmdStack, combinedCmd);
    }
    return pop(cmdStack);
}

/**********************/
/***     Tests      ***/
/**********************/

void
test_replace_backslash_newline() {
    char testArray[] = "ec\\\n\\\nho test";
    printf("TEST REPLACE BACKSLASH NEWLINE\n--------------------------------\n\n");
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    //replace_backslash_newline(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_replace_whitespace_after_op() {
    //char testArray[] = "a | \n \n \t \n \n b \n\n e || f | \n     g";
    char testArray[] = "a || b \n\n c && \n d || e | f \n\n\n echo test \n\n";
    printf("TEST REPLACE WHITESPACE AFTER OP\n--------------------------------\n\n");
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    replace_whitespace_after_op(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_replace_multiple_newlines() {
    //char testArray[] = "a |\nb \n\n \n b && c || e | f \n\n\n\n";
    char testArray[] = "a || b \n\n c && \n d || e | f \n\n\n echo test \n\n";
    printf("TEST REPLACE MULTIPLE NEWLINES\n--------------------------------\n\n");
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
    replace_whitespace_after_op(testArray);
    replace_multiple_newlines(testArray);
    printf("NEW COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);
}

void 
test_tokenize_complete_cmds() {
    //char testArray[] = "a || b \n\n c && \n d || \n e | f \n\n\n echo test \n\n";
    char testArray[] = "echo test\necho test";
    //"true\n\ng++ -c foo.c\n\n: : :\n\n\n\n\ncat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!\n\na b<c > d\n\ncat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!\n\na&&b||\nc &&\n d | e && f|\n\ng<h\n\n# This is a weird example: nobody would ever want to run this.\na<b>c|d<e>f|g<h>i";
    printf("TEST TOKENIZE COMPLETE COMMANDS\n--------------------------------\n\n");
    printf("ORIGINAL COMMAND STRING\n----------------\n");
    printf("%s\n", testArray);

    replace_whitespace_after_op(testArray);
    replace_multiple_newlines(testArray);
    char** newArray = tokenize_complete_cmds(testArray);

    printf("COMMAND ARRAY\n----------------\n");
    int i;
    for (i = 0; newArray[i] != NULL; ++i) {
        printf("%s\t\t%d\n", newArray[i], i);
    }
}

void
test_validate_fail() {
//THESE SHOULD FAIL - from test-p-bad.sh
    char** bad = (char *[]){
        "a\t>\t>\t>b",
        "a>b<c",
        "(\t&&)",
        "(\n\t\n)"
    };

    int i;
    for (i = 0; i < 4; ++i) {
        printf("--------\n%d\n", i);
        validate(bad[i]);
        printf("%s\n--------\n\n\n", bad[i]);
    }
// THESE SHOULD PASS - from test-p-ok.sh
    //char* test = "true\n\ng++ -c foo.c\n\n: : :\n\ncat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!\n\na b<c > d\n\ncat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!\n\na&&b||\n c &&\n  d | e && f|\n\ng<h\n\n# This is a weird example: nobody would ever want to run this.\na<b>c|d<e>f|g<h>i";
    //char* test = "echo b && #lol\nb";
    //char* test3 = "echo b && \n\n\n #g;e \necho test";
    //char* test = "a || \n\n b";


    // TODO-TUAN
    //char* test2 = "a \n      \n b";
    //validate(test);
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                     void *get_next_byte_argument)
{    
  /* FIXME: Replace this with your implementation.  You may need to
    add auxiliary functions and otherwise modify the source code.
    You can also use external functions defined in the GNU C Library. */

    //printf("at the beginning\n\n");

    // convert file to c-string
    char* scriptStr = file_to_str(get_next_byte, get_next_byte_argument);
    
    replace_whitespace_before_close_paren(scriptStr);
    
    /* validation */
    validate(scriptStr);

    //printf("after validation\n\n");

    /* cleanup a little bit before splitting by \n\n */
    replace_whitespace_after_op(scriptStr);
    replace_multiple_newlines(scriptStr);

    // printf("after cleanup\n\n");
    // printf("%s\n", scriptStr);

    /* split by \n\n and put into array */
    char** completeCmds = tokenize_complete_cmds(scriptStr);

    // printf("after tokenizing\n\n");
    // printf("COMMAND ARRAY\n----------------\n");

    int j;
    for (j = 0; completeCmds[j] != NULL; j++) {
        // printf("%s\t\t%d\n", completeCmds[j], j);
    }

    command_stream_t commandStream = create_stack();

    //printf("created command stream\n\n");

    /* loop through the array, making each a command tree */
    int i;
    for (i = 0; i < j; ++i) {
        // printf("building command %d\n\n", i);
        push_back(commandStream, parse_complete_command(completeCmds[i]));
    }

    return commandStream;

    ////error (1, 0, "command reading not yet implemented");
}

command_t
read_command_stream (command_stream_t s)
{
    if (!empty(s)) {
        return pop(s);
    }
    //error (1, 0, "command reading not yet implemented");
    return NULL;
}