
/***************************************************
 * backupd.c
 * Author: Brian Clark
 * Date: 4/12/18
 * Description: Linux filesystem monitoring daemon
 */

#include <signal.h>             // signal
#include <stdarg.h>             // varargs
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

FILE* f;                        // Global log file ptr

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

void daemonize(){
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

    // Use setsid to detach from calling TTY
    pid_t sid;
    sid = setsid();
    if (sid < 0){
        const char* msg = "SID: unable to set unique session id";
        fprintf(stderr, "%s\n", msg);
        exit(SID_FAILURE);
    }
    printf("SID: set unique session id\n");


    // Change daemon's working directory 

    int moved = chdir("/");
    if (moved < 0){
        const char* msg = "chdir: unable to change working directory";
        fprintf(stderr, "%s\n", msg);
        exit(CHDIR_FAILURE);
    }
    printf("chdir: changed working directory\n");


    // Catch signals
    signal(SIGCHLD, SIG_IGN);           // ignore dead child
    signal(SIGHUP, signal_handler);     // hand of interrupt
    signal(SIGTERM, signal_handler);    //
}

void start_log(const char* fp){
    f = fopen(fp, "a+");
    if (f == NULL){
        fprintf(stderr, "Log file: cannot open\n");
        exit(LOG_OPEN_FAILURE);
    }
    fprintf(stdout, "Creating log file\n");
    fprintf(f, "Starting up backupd.\n");
}

void logger(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fflush(f);
}


// Close any inherited file descriptors
void close_fds(){
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main(int arc, char* argv[]){

    daemonize();
    start_log("/home/brian/school/338/final/backupd/out/backupd.log");
    close_fds();

    // Initialization
    logger("Initializing backupd\n");


    // Main loop
    while (1){
        logger("I AM DAEMON\n");
        sleep(3);
    }

    exit(SUCCESS);
}
