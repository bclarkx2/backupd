
/***************************************************
 * backupd.c
 * Author: Brian Clark
 * Date: 4/12/18
 * Description: Linux filesystem monitoring daemon
 */

#include <stdio.h>              // I/O
#include <stdlib.h>             // exit
#include <sys/types.h>          // pid_t
#include <unistd.h>             // fork

// Error codes
#define SUCCESS 0
#define FAILURE 1
#define FORK_FAILURE 2


int main(int arc, char* argv[]){


    // Steps

    // Fork parent process
    pid_t pid;

    pid = fork();

    if (pid < 0){
        fprintf(stderr, "Fork unsuccessful\n");
        exit(FORK_FAILURE);
    }
    else if (pid > 0){
        printf("Fork successful: killing parent\n");
        exit(EXIT_SUCCESS);
    }

    printf("Fork successful: continuing with child\n");

    // Terminate parent iff forking was successful

    // Use setsid to detach from calling TTY

    // Catch signals

    // Fork again and let parent terminate

    // Change daemon's working directory 
 
    // Use umask to acquire appropriate file permissionss

    // Close any inherited file descriptors

    printf("I AM DAEMON\n");

    return 0;
}
