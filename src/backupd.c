
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
#define CLI_FAILURE 6
#define CONFIG_FAILURE 7

// Constants
#define LINE_WIDTH 80
const char* STRING_CONFIG_FMT = "%*s %2000[^\n]%*c";


//Globals

FILE* p_log;                      // Global log file ptr


void start_log(char* fp){
    p_log = fopen(fp, "a+");
    if (p_log == NULL){
        fprintf(stderr, "Log file: cannot open\n");
        exit(LOG_OPEN_FAILURE);
    }
    fprintf(stdout, "Creating log file\n");
    fprintf(p_log, "Starting up backupd.\n");
}

void logger(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    vfprintf(p_log, fmt, args);
    va_end(args);
    fflush(p_log);
}

void signal_handler(int sig){
    switch(sig){
        case SIGHUP:
            logger("SIGHUP received\n");
            break;
        case SIGTERM:
            logger("SIGTERM received\n");
            exit(EXIT_SUCCESS);
            break;
        default:
            logger("Strange signal received: %d\n", sig);
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

// Close any inherited file descriptors
void close_fds(){
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

/******************************************
 * Config
 */

typedef struct config {
    char log_loc[LINE_WIDTH];
    char msg[LINE_WIDTH];
    char watch_dir[LINE_WIDTH];
} config;

void str_config(FILE* config_file,
                char* str_config,
                int length){
    char line[length];
    if (fgets(line, length, config_file) == NULL){
        fprintf(stderr, "Could not read required config line\n");
        exit(CONFIG_FAILURE);
    }
    if (sscanf(line, STRING_CONFIG_FMT, str_config) == EOF){
        fprintf(stderr, "Cound not parse config line\n");
        exit(CONFIG_FAILURE);
    }
}

void read_config(const char* config_loc, config* c){
    FILE* config_file = fopen(config_loc, "r");

    str_config(config_file, c->log_loc, sizeof(c->log_loc));
    str_config(config_file, c->msg, sizeof(c->msg));
    str_config(config_file, c->watch_dir, sizeof(c->watch_dir));

    fclose(config_file);
}


/******************************************
 * CLI
 */
const char* config_location(int argc, char* argv[]){

    if (argc != 2){
        fprintf(stderr, "Enter config location as arg\n");
        exit(CLI_FAILURE);
    }
    else{
        return argv[1];
    }
}


int main(int argc, char* argv[]){

    const char* config_loc = config_location(argc, argv);

    config c;
    read_config(config_loc, &c);


    daemonize();
    start_log(c.log_loc);
    close_fds();

    // Initialization
    logger("Initializing backupd\n");
    logger("watch_dir: %s\n", c.watch_dir);

    // Main loop
    while (1){
        logger(c.msg);
        logger("\n");
        sleep(3);
    }

    exit(SUCCESS);
}
