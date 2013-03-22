//
//  digenv.c
//  lab1
//
//  Created by Lucas Wiener & Mathias Lindblom on 3/21/13.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

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

//a macro to print better error messages and exit the process on error.
//when parameter r == -1 the process gets killed and a error message is presented.
#define CHECK(r) { if(r == -1) {perror(""); fprintf(stderr, "line: %d.\n", __LINE__); exit(1);} }

//gets name of the pager to use. First tries to read env PAGER, and then falls back to less.
//return: the value specified in env[PAGER]. Otherwise less.
const char *getPager() {
    //read which pager to use from users env.
    const char *pager = getenv(ENV_PAGER);
    
    //if no specified pager is set, less should be used.
    if(pager == NULL) {
        pager = FALLBACK_PAGER;
        
        //TODO: check if less exists on system? In that case use more.
    }
    
    return pager;
}

//program main entry point
int main(int argc, const char * argv[], const char **envp) {
    //get the pager to use
    const char *pager = getPager();
    
    int     fd[2];      //pipe for child 1
    pid_t   pid;        //child process id
    int     status;     //will be used when calling wait
    
    
    char *t = (char*)argv[0];
    char *t2 = t + 1;
    
    if(t2 == NULL)
        printf("hej");
    
    //setup pipe for child 1
    CHECK(pipe(fd));
    
    //create child process 1
    CHECK((pid = fork()))
    
    if(pid == 0) {
        //child area.
        
        //child will only write to pipe, so close input pipe
        CHECK(close(fd[PIPE_IN]));
        
        //make alias stdout -> output pipe. this closes stdout. When exec prints to stdout it instead prints to child 1 output pipe.
        CHECK(dup2(fd[PIPE_OUT], STANDARD_OUTPUT));
        
        //obtain env variables. this prints to stdout which in turns prints to output pipe.
        CHECK(execlp(COMMAND_PRINTENV, COMMAND_PRINTENV, '\0'));
        
        //we are now done with the output pipe.
        CHECK(close(fd[PIPE_OUT]));
    } else {
        //parent area. childpid now contains the process id of the child.
        
        //wait for child process 1 to finish.
        CHECK(wait(&status));
        CHECK(status);
        
        //parent will only read from pipe, so close output pipe
        CHECK(close(fd[PIPE_OUT]));
        
        //variables to be used for child 2
        int fd2[2];     //pipe for child 2
        pid_t pid2;     //process id for child 2
        
        //setup pipe for child 2
        CHECK(pipe(fd2));
        
        //create child process 2
        CHECK((pid2 = fork()));
        
        if(pid2 == 0) {
            //child area.
            
            //child 2 will be reading from child 1 input pipe, so close child 2 input pipe
            CHECK(close(fd2[PIPE_IN]));
            
            //make alias stdin -> child 1 input pipe. This closes stdin. When exec reads from stdin it will instead read from child 1 input pipe.
            CHECK(dup2(fd[PIPE_IN], STANDARD_INPUT));
            
            //make alias stdout -> output pipe. this closes stdout. When exec prints to stdout it instead prints to child 2 output pipe.
            CHECK(dup2(fd2[PIPE_OUT], STANDARD_OUTPUT));
            
            if(argc > 1) {
                //the user specified arguments, pass them to a grep command
                
                //build a custom args list to pass to grep, request memory for it
                char **args = (char**)malloc((argc + 1) * sizeof(*args));
                
                //first argument should be name of program "grep"
                args[0] = COMMAND_GREP;
                
                //copy the rest of the digenv arguments to the grep args
                for (int i = 1; i < argc; ++i) {
                    args[i] = (char*)argv[i];
                }
                
                //execvp needs the array of args to be NULL terminated
                args[argc] = NULL;
                
                //execute grep with the args
                CHECK(execvp(COMMAND_GREP, args));
            
                //we no longer needs the args array, so free the memory
                free(args);
            } else {
                //no arguments, just pass input to output by doing a cat command (will echo input to output in this case)
                CHECK(execlp(COMMAND_CAT, COMMAND_CAT, '\0'));
            }
            
            //we are done writing to child 2 output pipe. close it.
            CHECK(close(fd2[PIPE_OUT]));
            
            //we are done reading from child 1 input pipe
            CHECK(close(fd[PIPE_IN]));
        } else {
            //parent area. childpid now contains the process id of the child.
            
            //wait for child 2 process to finish.
            CHECK(wait(&status));
            CHECK(status);
            
            //we will only be reading from child 2 so close child 2 output pipe.
            CHECK(close(fd2[PIPE_OUT]));
            
            //variables to be used for child 3
            int fd3[2];     //pipe for child 3
            pid_t pid3;     //process id for child 3
            
            //setup pipe for child 3
            CHECK(pipe(fd3));
            
            //create child process 3
            CHECK((pid3 = fork()));

            if(pid3 == 0) {
                //child area
                
                //child 3 will be reading from child 2 input pipe, so close child 3 input pipe
                CHECK(close(fd3[PIPE_IN]));
                
                //make alias stdin -> child 2 input pipe. This closes stdin. When exec reads from stdin it will instead read from child 2 input pipe.
                CHECK(dup2(fd2[PIPE_IN], STANDARD_INPUT));
                
                //make alias stdout -> output pipe. this closes stdout. When exec prints to stdout it instead prints to child 3 output pipe.
                CHECK(dup2(fd3[PIPE_OUT], STANDARD_OUTPUT));
                
                //execute the sort command
                execlp(COMMAND_SORT, COMMAND_SORT, '\0');
                
                //we are done writing to child 3 output pipe. close it.
                CHECK(close(fd3[PIPE_OUT]));
                
                //we are done reading from child 2 input pipe
                CHECK(close(fd2[PIPE_IN]));
            } else {
                //parent area
                
                //wait for child 3 process to finish.
                CHECK(wait(&status));
                CHECK(status);
                
                //we will only be reading from child 3 so close child 3 output pipe.
                CHECK(close(fd3[PIPE_OUT]));
                
                //variables to be used for child 4
                int fd4[2];     //pipe for child 4
                pid_t pid4;     //process id for child 4
                
                //setup pipe for child 4
                CHECK(pipe(fd4));
                
                //create child process 4
                CHECK((pid4 = fork()));
                
                if(pid4 == 0) {
                    //child area
                    
                    //child 4 will be reading from child 3 input pipe, so close child 4 input pipe
                    CHECK(close(fd4[PIPE_IN]));
                    
                    //make alias stdin -> child 3 input pipe. This closes stdin. When exec reads from stdin it will instead read from child 3 input pipe.
                    CHECK(dup2(fd3[PIPE_IN], STANDARD_INPUT));
                    
                    //we will not write to the child 4 output pipe. This will be handled by the pager in the terminal via stdout.
                    CHECK(close(fd4[PIPE_OUT]));
                
                    //execute the pager command
                    if(execlp(pager, pager, '\0') == -1) {
                        if(errno == ENOENT) {
                            //no such file or directory. Either the user defined pager or less do not exist on system. try with more instead.
                            pager = FALLBACK_PAGER_2;
                            CHECK(execlp(pager, pager));
                        } else {
                            //something else went wrong, force an error.
                            CHECK(-1);
                        }
                    }
                    
                    //we are done reading from child 3 input pipe
                    CHECK(close(fd3[PIPE_IN]));

                } else {
                    //parent area
                    
                    //wait for child 4 process to finish.
                    CHECK(wait(&status));
                    CHECK(status);
                    
                    //we will not read or read from child (this will be done in the terminal by the pager instead. close both pipes.
                    CHECK(close(fd4[PIPE_OUT]));
                    CHECK(close(fd4[PIPE_IN]));
                }
            }
        }
    }

    //exit normally
    return 0;
}