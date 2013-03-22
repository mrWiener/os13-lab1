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

//a macro to print better error messages and exit the process on error.
//when parameter r == -1 the process gets killed and a error message is presented.
#define CHECK(r) { if(r == -1) {perror(""); fprintf(stderr, "line: %d.\n", __LINE__); exit(1);} }

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

//program main entry point
int main(int argc, const char * argv[], const char **envp) {
    //get the pager to use
    const char *pager = getPager();
    
    int     fd[2];      //pipe for child 1
    pid_t   pid;        //child process id
    int     status;     //will be used when calling wait
    
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
        CHECK(execlp("printenv", "printenv", '\0'));
        
        //we are now done with the output pipe.
        CHECK(close(fd[PIPE_OUT]));
    } else {
        //parent area. childpid now contains the process id of the child.
        
        //wait for child process 1 to finish.
        wait(&status);
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
            
            //sort the env variables by using command sort.
            CHECK(execlp("sort", "sort", '\0'));
            
            //we are done writing to child 2 output pipe. close it.
            CHECK(close(fd2[PIPE_OUT]));
        } else {
            //parent area. childpid now contains the process id of the child.
            
            //wait for child 2 process to finish.
            wait(&status);
            CHECK(status);
            
            //we will only be reading from child 2 so close child 2 output pipe.
            CHECK(close(fd2[PIPE_OUT]));
            
            //read from child 2 input pipe.
            char readbuffer[1024];
            read(fd2[PIPE_IN], readbuffer, sizeof(readbuffer));
            
            //done reading. close child 2 input pipe.
            CHECK(close(fd2[PIPE_IN]));
            
            //print result to stdout.
            printf("%s", readbuffer);
        }
    }

    return 0;
}