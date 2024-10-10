/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * jitc.c
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include "system.h"
#include "jitc.h"

/**
 * Needs:
 *   fork()
 *   execv()
 *   waitpid()
 *   WIFEXITED()
 *   WEXITSTATUS()
 *   dlopen()
 *   dlclose()
 *   dlsym()
 */

/* research the above Needed API and design accordingly */


struct jitc{
    void *handle;
};


int jitc_compile(const char *input, const char *output) {
    pid_t process_id; 
    int status;
    char *input_file = (char *)input;
    char *output_file = (char *)output;

    process_id = fork(); 
    
    switch (process_id) {
    case -1:
        EXIT("Fork failed");
        break;

    case 0: {
        /* Child process: compiles the input file using gcc via execv function to generate the out.so file. */
        char *args[8];
        args[0] = "gcc"; 
        args[1] = "-O3"; 
        args[2] = "-fpic"; 
        args[3] = "-shared"; 
        args[4] = "-o"; 
        args[5] = output_file; 
        args[6] = input_file; 
        args[7] = NULL; 

        execvp("gcc", args); 
        exit(EXIT_FAILURE); 
        break; 
    }
        
    default: {
        waitpid(-1, &status, 0); 
        
        if (WIFEXITED(status) || WEXITSTATUS(status)) {
            /* Check if the child exited normally and return the status code */
            return WEXITSTATUS(status);
        } else {
            EXIT("Child process did not execute successfully");
        }
        break;
    }
    }
}

struct jitc *jitc_open(const char *pathname) {
    /* Allocate memory for the jitc struct */
    struct jitc *soModuleHandle = (struct jitc *)malloc(sizeof(struct jitc));
    soModuleHandle->handle = dlopen(pathname, RTLD_LAZY | RTLD_LOCAL); /*  to reference to symbols (like functions or variables) until they are actually needed at runtime. */
    
    if (!soModuleHandle) {
        free(soModuleHandle); 
        return NULL; 
    }
    
    return soModuleHandle; 
}

void jitc_close(struct jitc *jitc) {
    if (jitc != NULL) {
        /* Close the dynamic module handle and free the allocated memory */
        dlclose(jitc->handle); /* Close the shared object */
        free(jitc); 
        return; 
    } else {
        EXIT("The out.so file handle did not close properly");
    }
}

long jitc_lookup(struct jitc *jitc, const char *symbol) {

    void *symbol_address = dlsym(jitc->handle, symbol);
    
    if (symbol_address == NULL) {
        return 0; 
    }
    
    return (long)symbol_address; 
}
