#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

	int sysrtn = system(cmd); //system() function will either return the exit status of the ran command, normally 0

	if (sysrtn != EXIT_SUCCESS) { //If the system() function returns anything but 0, return false indicating error
		return false;
	}

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
	
	pid_t pid = fork();
	int waitstat;

	if (pid == -1){ //Have to explicitly check for error code -1
		perror("ERROR with fork()");
		return false;
	}

	if (!pid){ //We can assume here that if pid is anything but zero it should be a PID since -1 was checked and errored
		int execrt = execv(command[0], command);
		
		if (execrt == -1){ //Have to explicitly check for error code -1
			perror("ERROR with execv()");
			exit(EXIT_FAILURE); //Using exit() to exit fully as return would only go up 1 if statement
		}
	} 
	
	pid = wait(&waitstat); //We can assume that since no other child process was created, wait() will work
	
	if(pid == -1){ //Check if wait() exits abnormally
		perror("ERROR with wait()");
		return false;
	}
	
	if(WEXITSTATUS(waitstat) != EXIT_SUCCESS){ // Check to see if the program ran in the child function exited abnormally
		perror("EXIT ERROR from child function");
		return false;
	}


    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

	pid_t pid = fork();
	int waitstat;

	fflush(stdout); //Prevent weird printf case as noted by assignment troubleshoot
 	if (pid == -1){ //Have to explicitly check for error code -1
 		perror("ERROR with fork()");
 		return false;
 	}
 	if (!pid){ //We can assume here that if pid is anything but zero it should be a PID since -1 was checked and errored
		int file = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
		
		if(file == -1){ //Check is int file returns a -1 for error
			perror("ERROR with file open()");
			exit(1);
		}

		if(dup2(file, 1) == -1){ //Check to see if the duplication program exited successfully
			perror("ERROR with duplication");
			exit(1);
		}
		int execrt = execv(command[0], command);
 		if (execrt == -1){ //Have to explicitly check for error code -1
			perror("ERROR with execv()");
 			exit(1); //Using exit() to exit fully as return would only go up 1 if statement
 		}
 	}

 	pid = wait(&waitstat); //We can assume that since no other child process was created, wait() will work
 	if(pid == -1){ //Check if wait() exits abnormally
 		perror("ERROR with wait()");
		return false;
	}

    va_end(args);

    return true;
}
