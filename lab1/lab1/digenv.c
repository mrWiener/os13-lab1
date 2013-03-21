//
//  main.c
//  lab1
//
//  Created by Lucas Wiener & Mathias Lindblom on 3/21/13.
//  Copyright (c) 2013 os13. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define STANDARD_INPUT      0
#define STANDARD_OUTPUT     1
#define STANDARD_ERROR      2

#define PIPE_IN             0
#define PIPE_OUT            1

//gets name of the pager to use. First tries to read env PAGER, and then falls back to less.
//return: the value specified in env[PAGER]. Otherwise less.
const char *getPager() {
    //read which pager to use from users env.
    const char *pager = getenv("PAGER");
    
    //if no specified pager is set, less should be used.
    if(pager == NULL) {
        pager = "less";
        
        //TODO: check if less exists on system? In that case use more.
    }
    
    return pager;
}

//print the specified error and exists the program
void error(char *error) {
    perror(error);
    exit(1);
}

//program main entry point
int main(int argc, const char * argv[], const char **envp) {
    //get the pager to use
    const char *pager = getPager();
    
    //setup pipeline for printenv call
    int     fd[2];
    pid_t   childpid;
    
    if(pipe(fd) == -1) {
        //unable to setup pipe.
        error("pipe init failed: printenv");
    }
    
    if((childpid = fork()) == -1) {
        //unable to start the child process
        error("fork failed: printenv");
    }
    
    if(childpid == 0) {
        //child area.
        
        //child will only write to pipe, so close input fd
        if(close(fd[PIPE_IN]) == -1) {
            error("failed to close input fd: printenv");
        }
        
        //close stdout and duplicate it the output side of stdout to fd output
        if(dup2(STANDARD_OUTPUT, fd[PIPE_OUT]) == -1) {
            error("failed to dup2: printenv");
        }
        
        //all env variables should be obtained and printed to pipe.
        if(execlp("printenv", "printenv") == -1) {
            error("failed to execute: printenv");
        }
        
        close(fd[1]);
    } else {
        //parent area. childpid now contains the process id of the child.
        
        //parent will only read from pipe, so close output fd
        if(close(fd[PIPE_OUT]) == -1) {
            error("failed to close output fd: printenv");
        }
        
        char readbuffer[1024];
        int nbytes = read(fd[PIPE_IN], readbuffer, sizeof(readbuffer));
        close(fd[PIPE_IN]);
        printf("Result: %s", readbuffer);
    }


    return 0;
}