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

//program main entry point
int main(int argc, const char * argv[], const char **envp) {
    int i;
    for (i = 0; envp[i] != NULL; i++) {
        printf("%2d:%s\n", i, envp[i]);
    }
    
    printf("\n\nPATH: %s", getEnvValue("PATH", envp));

    return 0;
}