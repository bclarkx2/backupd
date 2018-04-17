
/***************************************************
 * backupd.c
 * Author: Brian Clark
 * Date: 4/12/18
 * Description: Linux filesystem monitoring daemon
 */

#include <signal.h>             // signal
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
#define SID_FAILURE 4
#define CHDIR_FAILURE 5

FILE* f;

void signal_handler(int sig){
    switch(sig){
        case SIGHUP:
            fprintf(f, "SIGHUP received\n");
            break;
        case SIGTERM:
            fprintf(f, "SIGTERM received\n");
            exit(EXIT_SUCCESS);
            break;
        default:
            fprintf(f, "Strange signal received: %d\n", sig);
    }
}


int main(int arc, char* argv[]){


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
    f = fopen("backupd.log", "a+");
    if (f == NULL){
        fprintf(stderr, "Log file: cannot open\n");
        exit(LOG_OPEN_FAILURE);
    }
    fprintf(stdout, "Creating log file\n");
    fprintf(f, "Starting up backupd.\n");

    // Use setsid to detach from calling TTY
    pid_t sid;
    sid = setsid();
    if (sid < 0){
        const char* msg = "SID: unable to set unique session id";
        fprintf(stderr, "%s\n", msg);
        fprintf(f, "%s\n", msg);
        exit(SID_FAILURE);
    }
    printf("SID: set unique session id\n");


    // Change daemon's working directory 

    int moved = chdir("/");
    if (moved < 0){
        const char* msg = "chdir: unable to change working directory";
        fprintf(stderr, "%s\n", msg);
        fprintf(f, "%s\n", msg);
        exit(CHDIR_FAILURE);
    }
    printf("chdir: changed working directory\n");


    // Close any inherited file descriptors

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);


    // Catch signals
    signal(SIGCHLD, SIG_IGN);           // ignore dead child
    signal(SIGHUP, signal_handler);     // hand of interrupt
    signal(SIGTERM, signal_handler);    //


    // Initialization
    fprintf(f, "Initializing backupd\n");


    // Main loop
    while (1){
        fprintf(f, "I AM DAEMON\n");
        fflush(f);
        sleep(3);
    }

    exit(SUCCESS);
}
