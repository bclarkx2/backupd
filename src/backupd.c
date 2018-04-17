
/***************************************************
 * backupd.c
 * Author: Brian Clark
 * Date: 4/12/18
 * Description: Linux filesystem monitoring daemon
 */

#include <stdio.h>              // I/O
#include <stdlib.h>             // exit
#include <sys/types.h>          // pid_t
#include <sys/stat.h>           // umask
#include <unistd.h>             // fork

// Error codes
#define SUCCESS 0
#define FAILURE 1
#define FORK_FAILURE 2
#define LOG_OPEN_FAILURE 3


int main(int arc, char* argv[]){


    // Steps

    // Fork parent process, kill parent
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

    // Use umask to acquire appropriate file permissionss
    // (ex: want to be able to write to logs)
    umask(0);  // 0 should allow full access to created files

    // Open log files
    FILE* f;
    f = fopen("backupd.log", "a+");
    if (f == NULL){
        fprintf(stderr, "Log file: cannot open\n");
        exit(LOG_OPEN_FAILURE);
    }
    fprintf(stdout, "Creating log file\n");
    fprintf(f, "Starting up backupd.\n");

    // Use setsid to detach from calling TTY

    // Catch signals

    // Fork again and let parent terminate

    // Change daemon's working directory 
 

    // Close any inherited file descriptors

    printf("I AM DAEMON\n");

    return 0;
}
