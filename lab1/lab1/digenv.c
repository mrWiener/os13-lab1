//
//  main.c
//  lab1
//
//  Created by Lucas Wiener & Mathias Lindblom on 3/21/13.
//  Copyright (c) 2013 os13. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//gets the value of a given env variable name.
//param name: is the name of the env variable name to get the value of
//param envp: the env array given at startup
//return: a pointer to the value. NULL if not found or error.
const char *getEnvValue(char *name, const char **envp) {
    if(name == NULL || envp == NULL) {
        return NULL;
    }
    
    char *pos = NULL;
    for (int i = 0; envp[i] != NULL; i++) {
        if((pos = strstr(envp[i], name)) != NULL) {
            //we have found the "name" env variable. Now read the value.
            
            //point the pos pointer x chars ahead. x = length of name + 1 (since we wanna start read after '=' sign)
            pos += strlen(name) + 1;
            return pos; //return a pointer to the beginning of the env value of the env variable "name"
        }
    }
    
    return NULL;
}

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
    int i;
    
    const char *pager = getPager();

    return 0;
}