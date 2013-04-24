/*
 *  digenv.c
 *  lab1
 *
 *  Created by Lucas Wiener & Mathias Lindblom.
 *
 *  digenv offers a more sophisticated way of reading environment variables of a system.
 *  The program first reads the variables and then performs a grep with given arguments. If no arguments
 *  are given, the program instead pipes the data through the cat command. Then the data gets sorted by
 *  executing the sort program, and last the data is presented with the pager specified by the "PAGER"
 *  environment variable. If no such variable exists, the less program is chosen. If the execution of less
 *  fails, the more program is chosen.
 *
 *  The program can be described as the system command: printenv | grep <args> | sort | PAGER
 *
 *  See grep manual for <args>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

/* Define some helper constants. */
#define STANDARD_INPUT      0
#define STANDARD_OUTPUT     1
#define STANDARD_ERROR      2

#define PIPE_IN             0
#define PIPE_OUT            1

#define FALLBACK_PAGER      "less"
#define FALLBACK_PAGER_2    "more"

#define COMMAND_PRINTENV    "printenv"
#define COMMAND_GREP        "grep"
#define COMMAND_CAT         "cat"
#define COMMAND_SORT        "sort"

#define ENV_PAGER           "PAGER"

/*
A macro to print better error messages and exit the process on error.
when parameter r == -1 the process gets killed and a error message is presented.
*/
#define CHECK(r) { if(r == -1) {perror(""); fprintf(stderr, "line: %d.\n", __LINE__); exit(1);} }

/*
Gets name of the pager to use. First tries to read env PAGER, and then falls back to less.
returns the value specified in env[PAGER]. Otherwise less.
*/
const char *getPager() {
    /* Read which pager to use from users env. */
    char *pager = getenv(ENV_PAGER);
    
    /* If no specified pager is set, less should be used. */
    if(pager == NULL) {
        pager = FALLBACK_PAGER;
    }
    
    return pager;
}

/* 
Program main entry point. 
The main method will perform all of the digenv logic.
*/
int main(int argc, char ** argv, const char **envp) {
    pid_t   pid;        /* Child process id. */
    int     status;     /* Will be used when calling wait to read child exist status. */

    int     fd_penv[2];     /* Array to hold pipe file descriptors for printenv-child. */
    int     fd_grep[2];     /* Array to hold pipe file descriptors for grep-child. */
    int     fd_sort[2];     /* Array to hold pipe file descriptors for sort-child. */
    
    /* Setup pipe for printenv-child. */
    CHECK(pipe(fd_penv));    
    
    /* Create child process for printenv-child. */
    CHECK((pid = fork()))
    
    if(pid == 0) {
        /* printenv-child area */
        
        /* Child will only write to pipe, so close input pipe. */
        CHECK(close(fd_penv[PIPE_IN]));
        
        /* Make alias stdout -> output pipe. this closes stdout. When exec prints to stdout it instead prints to child output pipe. */
        CHECK(dup2(fd_penv[PIPE_OUT], STANDARD_OUTPUT));
        
        /* Execute printenv with no arguments. this prints to stdout which in turns prints to child output pipe. */
        CHECK(execlp(COMMAND_PRINTENV, COMMAND_PRINTENV, (char*)NULL));
    } else {
        /* Parent area. Variable pid now contains the process id of the printenv-child. */
        
        /* Parent will only read from pipe, so close output pipe. */
        CHECK(close(fd_penv[PIPE_OUT]));
        
        /* wait for printenv-child process to terminate. */
        CHECK(wait(&status));

        /* Check termination status. */
        if(!WIFEXITED(status)) {
            /* Child process did not terminate by calling exit. Force error. */
            CHECK(-1);
        } else {
            /* Child process did exit normally, check the exit value. */
            int exit_value = WEXITSTATUS(status);

            if(exit_value == 1) {
                /*
                The printenv program exited with an error.
                just exit the program with the same value, since printenv will handle the error printing.
                */
                return exit_value;
            }
        }
        
        /* Setup pipe for grep-child. */
        CHECK(pipe(fd_grep));
        
        /* Create child process grep-child. */
        CHECK((pid = fork()));
        
        if(pid == 0) {
            /* grep-child area. */
            
            /* grep-child will be reading from printenv-child input pipe, so close grep-child input pipe. */
            CHECK(close(fd_grep[PIPE_IN]));
            
            /* Make alias stdin -> printenv-child input pipe. This closes stdin. When exec reads from stdin it will instead read from printenv-child input pipe. */
            CHECK(dup2(fd_penv[PIPE_IN], STANDARD_INPUT));
            
            /* Make alias stdout -> output pipe of grep-child. This closes stdout. When exec prints to stdout it instead prints to grep-child output pipe. */
            CHECK(dup2(fd_grep[PIPE_OUT], STANDARD_OUTPUT));
            
            if(argc > 1) {
                /* The user specified arguments, pass them to a grep command. */
               
                /* Change the first argument to COMMAND_GREP, since exec will overwrite this program anyway. */
                argv[0] = COMMAND_GREP;
                
                /* Execute grep with the args */
                CHECK(execvp(COMMAND_GREP, argv));
            } else {
                /* No arguments, just pipe input to output by doing a cat command (will echo input to output in this case) */
                CHECK(execlp(COMMAND_CAT, COMMAND_CAT, (char*)NULL));
            }
        } else {
            /* Parent area. The variable pid now contains the process id of grep-child. */
            
            /* Program is done reading from printenv-child. */
            CHECK(close(fd_penv[PIPE_IN]));
            
            /* Parent will only be reading from grep-child so close output pipe. */
            CHECK(close(fd_grep[PIPE_OUT]));
            
            /* Wait for grep-child process to terminate. */
            CHECK(wait(&status));

            /* Check termination status. */
            if(!WIFEXITED(status)) {
                /* Child process did not terminate by calling exit. Force error. */
                CHECK(-1);
            } else {
                /* Child process did exit normally, check the exit value. */
                int exit_value = WEXITSTATUS(status);
                int limit = 0; /* cat exits with value > 0 on error. */

                if(argc > 1) {
                    limit = 1; /* If the program got arguments, grep is executed instead of cat. grep exits with value > 1 on error. */
                }

                if(exit_value > limit) {
                    /*
                    grep/cat exited with an error. Most likely illegal arguments.
                    Just exit the program with the same value as grep/cat, since grep/cat will handle the error printing.
                    */
                    return exit_value;
                }
            }
            
            /* Setup pipe for sort-child. */
            CHECK(pipe(fd_sort));
            
            /* Create child process sort-child. */
            CHECK((pid = fork()));

            if(pid == 0) {
                /* sort-child area */
                
                /* sort-child will be reading from grep-child input pipe, so close sort-child input pipe. */
                CHECK(close(fd_sort[PIPE_IN]));
                
                /* Make alias stdin -> grep-child input pipe. This closes stdin. When exec reads from stdin it will instead read from grep-child input pipe. */
                CHECK(dup2(fd_grep[PIPE_IN], STANDARD_INPUT));
                
                /* Make alias stdout -> sort-child output pipe. this closes stdout. When exec prints to stdout it instead prints to sort-child output pipe. */
                CHECK(dup2(fd_sort[PIPE_OUT], STANDARD_OUTPUT));
                
                /* Execute the sort command. */
                execlp(COMMAND_SORT, COMMAND_SORT, (char*)NULL);
            } else {
                /* Parent area. */
                
                /* Program is done reading from grep-child. */
                CHECK(close(fd_grep[PIPE_IN]));
                
                /* Parent will only be reading from sort-child so close output pipe. */
                CHECK(close(fd_sort[PIPE_OUT]));
                
                /* Wait for sort-child process to terminate. */
                CHECK(wait(&status));

                /* Check termination status. */
                if(!WIFEXITED(status)) {
                    /* Child process did not terminate by calling exit. Force error. */
                    CHECK(-1);
                } else {
                    /* Child process did exit normally, check the exit value. */
                    int exit_value = WEXITSTATUS(status);

                    if(exit_value > 1) {
                        /* 
                        sort exited with an error.
                        Just exit the program with the same value as sort, since sort will handle the error printing.
                        */
                        return exit_value;
                    }
                }
                
                /* Create child process pager-child. */
                CHECK((pid = fork()));
                
                if(pid == 0) {
                    /* pager-child area. */
                    
                    /* Make alias stdin -> sort-child input pipe. This closes stdin. When exec reads from stdin it will instead read from sort-child input pipe. */
                    CHECK(dup2(fd_sort[PIPE_IN], STANDARD_INPUT));
                
                    /* Get the pager to use. */
                    const char *pager = getPager();

                    /* Execute the pager command. */
                    if(execlp(pager, pager, (char*)NULL) == -1) {

                        /* En error occured during executing the program, check the error code. */
                        if(errno == ENOENT) {
                            /* No such file or directory. Either the user defined pager or less do not exist on system. Try with more instead. */
                            pager = FALLBACK_PAGER_2;
                            CHECK(execlp(pager, pager, (char*)NULL));
                        } else {
                            /* something else went wrong, force an error. */
                            CHECK(-1);
                        }
                    }
                } else {
                    /* Parent area */
                    
                    /* Program is done with reading from sort-child. */
                    CHECK(close(fd_sort[PIPE_IN]));
                    
                    /* Wait for pager-child process to terminate. */
                    CHECK(wait(&status));

                    /* Check termination status. */
                    if(!WIFEXITED(status)) {
                        /* Child process did not terminate by calling exit. Force error. */
                        CHECK(-1);
                    } else {
                        /* Child process did exit normally, check the exit value. */
                        int exit_value = WEXITSTATUS(status);

                        if(exit_value != 0) {
                            /* 
                            pager exited with an error.
                            Just exit the program with the same value as pager, since pager will handle the error printing.
                            */
                            return exit_value;
                        }
                    }
                }
            }
        }
    }

    /* Exit normally. */
    return 0;
}