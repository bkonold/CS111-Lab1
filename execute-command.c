// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

int execute_command(command_t);


void
setup_redirects(command_t c)
{
	// redirect file to stdin
	if (c->input) {
		int inputFile = open(c->input, O_RDONLY, 0444);
		if (inputFile < 0) {
			exit(1);
		}
		if (dup2(inputFile, 0) < 0) {
			exit(1);
		}
		close(inputFile);
	}
	// redirect stdout to file
	if (c->output) {
		int outputFile = open(c->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (outputFile < 0) {
			exit(1);
		}
		if (dup2(outputFile, 1) < 0) {
			exit(1);
		}
		close(outputFile);
	}
}

void 
execute_simple(command_t c)
{
	int status;
	int p = fork();
	if (p == 0) {
		setup_redirects(c);
		if (strcmp(c->u.word[0], "exec") == 0)
			execvp(c->u.word[1], &c->u.word[1]);
		else
			execvp(c->u.word[0], c->u.word);
		exit(1);
	}

	waitpid(p, &status, 0);
	c->status = WEXITSTATUS(status);
}

void 
execute_subshell(command_t c)
{
	int p = fork();
	if (p == 0) {
		setup_redirects(c);
		c->status = execute_command(c->u.subshell_command);
		exit(0);
	}
	int status;
	waitpid(p, &status, 0);
}

void 
execute_and(command_t c)
{
	int status = execute_command(c->u.command[0]);

	if (status == 0)
		status = execute_command(c->u.command[1]);
	
	c->status = status;
}

void 
execute_or(command_t c)
{
	int status = execute_command(c->u.command[0]);

	if (status != 0)
		status = execute_command(c->u.command[1]);

	c->status = status;
}

void 
execute_sequence(command_t c)
{
	int status = execute_command(c->u.command[0]);
	status = execute_command(c->u.command[1]);
	c->status = status;
}

void 
execute_pipe(command_t c)
{
	pid_t returnedPid;
	pid_t firstPid;
	pid_t secondPid;
	int buffer[2];
	int eStatus;

	if (pipe(buffer) < 0) {
		// error (1, errno, "pipe was not created");
		exit(1);
	}

	firstPid = fork();
	if (firstPid < 0) {
		//error(1, errno, "fork was unsuccessful");
		exit(1);
    }
	else if (firstPid == 0) {//child executes command on the right of the pipe
		close(buffer[1]); //close unused write end

        //redirect standard input to the read end of the pipe
        //so that input of the command (on the right of the pipe)
        //comes from the pipe
		if (dup2(buffer[0], 0) < 0) {
			//error(1, errno, "error with dup2");
			exit(1);
		}
		execute_command(c->u.command[1]);
		exit(c->u.command[1]->status);
	}
	else  {
		// Parent process
		secondPid = fork(); //fork another child process
                            //have that child process executes command on the left of the pipe
		if (secondPid < 0) {
			//error(1, 0, "fork was unsuccessful");
			exit(1);
		}
        else if (secondPid == 0) {
			close(buffer[0]); //close unused read end
			if(dup2(buffer[1], 1) < 0) {//redirect standard output to write end of the pipe 
				// error (1, errno, "error with dup2");
				exit(1);
            }
			execute_command(c->u.command[0]);
			exit(c->u.command[0]->status);
		}
		else {
			// Finishing processes
			returnedPid = waitpid(-1, &eStatus, 0); //this is equivalent to wait(&eStatus);
                        //we now have 2 children. This waitpid will suspend the execution of
                        //the calling process until one of its children terminates
                        //(the other may not terminate yet)

			//Close pipe
			close(buffer[0]);
			close(buffer[1]);

			if (secondPid == returnedPid) {
			    //wait for the remaining child process to terminate
				waitpid(firstPid, &eStatus, 0); 
				c->status = WEXITSTATUS(eStatus);
				return;
			}
			
			if (firstPid == returnedPid) {
			    //wait for the remaining child process to terminate
   				waitpid(secondPid, &eStatus, 0);
				c->status = WEXITSTATUS(eStatus);
				return;
			}
		}
	}	
}

int
command_status (command_t c)
{
	return c->status;
}

int 
execute_command(command_t c)
{
	switch(c->type) {

		case SIMPLE_COMMAND:
			execute_simple(c);
			break;
		case SUBSHELL_COMMAND:
			execute_subshell(c);
			break;
		case AND_COMMAND:
			execute_and(c);
			break;
		case OR_COMMAND:
			execute_or(c);
			break;
		case SEQUENCE_COMMAND:
			execute_sequence(c);
			break;
		case PIPE_COMMAND:
			execute_pipe(c);
			break;
		default:
			//error(1, 0, "Not a valid command");
			exit(1);
	}
	return c->status;
}
