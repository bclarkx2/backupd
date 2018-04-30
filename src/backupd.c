
/***************************************************
 * backupd.c
 * Author: Brian Clark
 * Date: 4/12/18
 * Description: Linux filesystem monitoring daemon
 */

#include <semaphore.h>          // sem_t
#include <signal.h>             // signal
#include <stdarg.h>             // varargs
#include <stdio.h>              // I/O
#include <stdlib.h>             // exit
#include <sys/types.h>          // pid_t
#include <sys/inotify.h>        // inotify
#include <sys/ioctl.h>          // ioctl
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
#define EVENT_SIZE sizeof(struct inotify_event)
#define MAX_TARGETS 10
const char* STR_CFG_FMT = "%*s %2000[^\n]%*c";
const char* INT_CFG_FMT = "%*s %d";
const char* TARGET_CFG_FMT = "%*s %s";


//Globals

FILE* p_log;                      // Global log file ptr
int incident_counter = 0;         // record num instances
sem_t incident_sem;               // keep incident_counter safe


/*****************************
 * Logging
 */

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


/******************************
 * Incident counter
 */

int get_incidents(){
    return incident_counter;
}

void reset_incidents(){
    sem_wait(&incident_sem);
    incident_counter = 0;
    sem_post(&incident_sem);
}

void increment_incidents(){
    sem_wait(&incident_sem);
    incident_counter++;
    sem_post(&incident_sem);
}


/******************************
 * Daemon stuff
 */

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

typedef struct target {
    char watch_dir[LINE_WIDTH];
    uint32_t mask;
} target;

typedef struct config {
    char log_loc[LINE_WIDTH];
    char msg[LINE_WIDTH];
    int incident_limit;
    int num_targets;
    target targets[MAX_TARGETS];
} config;

void read_config_line(char* line, int length, FILE* config_file){
    if (fgets(line, length, config_file) == NULL){
        fprintf(stderr, "Could not read required config line\n");
        exit(CONFIG_FAILURE);
    }
}

void read_config_val(char* line, const char* fmt, void* config){
    if (sscanf(line, fmt, config) == EOF){
        fprintf(stderr, "Cound not parse config line: %s\n", line);
        exit(CONFIG_FAILURE);
    }
}

void scalar_config(FILE* config_file,
                   void* config,
                   const char* fmt,
                   int length){
    char line[length];
    read_config_line(line, length, config_file);
    read_config_val(line, fmt, config);
}

void target_config(FILE* config_file,
                   target* target_config,
                   int length){
    char line[length];
    read_config_line(line, length, config_file);
    target_config->mask = IN_CREATE;
    if (sscanf(line, TARGET_CFG_FMT, target_config->watch_dir) == EOF){
        fprintf(stderr, "Cound not parse config line: %s\n", line);
        exit(CONFIG_FAILURE);
    }
}

void read_config(const char* config_loc, config* c){
    FILE* config_f = fopen(config_loc, "r");

    scalar_config(config_f, c->log_loc, STR_CFG_FMT, sizeof(c->log_loc));
    scalar_config(config_f, c->msg, STR_CFG_FMT, sizeof(c->msg));

    scalar_config(config_f, &(c->incident_limit), INT_CFG_FMT, LINE_WIDTH);
    scalar_config(config_f, &(c->num_targets), INT_CFG_FMT, LINE_WIDTH);

    for(int target_idx = 0; target_idx < c->num_targets; target_idx++){
        target conf = c->targets[target_idx];
        target_config(config_f, &conf, LINE_WIDTH);
    }

    fclose(config_f);
}


/******************************************
 * Events
 */

void read_events(int fd){
    // determine number of bytes available
    unsigned int available_bytes;
    ioctl(fd, FIONREAD, &available_bytes);

    // if no data available, do nothing
    if (available_bytes <= 0){
        return;
    }

    char event_buffer[available_bytes];
    read(fd, event_buffer, available_bytes);

    int offset = 0;
    while (offset < available_bytes){

        char* start = event_buffer + offset;
        struct inotify_event* event = (struct inotify_event*) start;

        logger("Received event: %d\n", event->wd);
        logger("File: %s\n", event->name);

        int total_event_size = EVENT_SIZE + event->len;
        offset += total_event_size;
    }
}

// insert event processor here
// void process_event(struct inotify_event* event,
//                    target* targ)

// insert record_incident here
// void record_incident()

// insert backup_action here
// void backup_action()


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


/******************************************
 * Main
 */

int main(int argc, char* argv[]){

    const char* config_loc = config_location(argc, argv);

    config c;
    read_config(config_loc, &c);


    daemonize();
    start_log(c.log_loc);
    close_fds();

    // Initialization
    logger("Initializing backupd\n");

    // int inotify_fd = inotify_init();
    // int watch_fd = inotify_add_watch(inotify_fd,
    //                                  c.watch_dir,
    //                                  IN_CREATE);

    struct inotify_event event;

    // Main loop
    while (1){
        logger(c.msg);
        logger("\n");

        // read_events(inotify_fd);

        sleep(3);
    }

    exit(SUCCESS);
}
