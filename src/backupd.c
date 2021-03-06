
/***************************************************
 * backupd.c
 * Author: Brian Clark
 * Date: 4/12/18
 * Description: Linux filesystem monitoring daemon
 */

#include <pthread.h>            // pthreads
#include <semaphore.h>          // sem_t
#include <signal.h>             // signal
#include <stdarg.h>             // varargs
#include <stdbool.h>            // bool
#include <stdio.h>              // I/O
#include <stdlib.h>             // exit
#include <syslog.h>             // syslog
#include <sys/types.h>          // pid_t
#include <sys/inotify.h>        // inotify
#include <sys/ioctl.h>          // ioctl
#include <sys/select.h>         // select
#include <sys/stat.h>           // umask
#include <unistd.h>             // fork

// BSD source
// #define __USE_BSD
#define _BSD_SOURCE

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
#define LINE_WIDTH 120
#define EVENT_SIZE sizeof(struct inotify_event)
#define MAX_TARGETS 10
const char* STR_CFG_FMT = "%*s %2000[^\n]%*c";
const char* INT_CFG_FMT = "%*s %d";
const char* TARGET_CFG_FMT = "%*s %s";
const char* BOOL_CFG_FMT = "%*s %d";

const char* SYSLOG_IDENT = "backupd";
const int SYSLOG_OPTIONS = LOG_CONS | LOG_PID;
const int SYSLOG_FACILITY = LOG_USER;

// Structures
typedef struct target {
    char watch_dir[LINE_WIDTH];
    uint32_t mask;
} target;

typedef struct config {
    char log_loc[LINE_WIDTH];
    char backup[LINE_WIDTH];
    bool syslogging;
    int incident_limit;
    int num_targets;
    target targets[MAX_TARGETS];
} config;


//Globals

config c;                         // Global config struct
FILE* p_log;                      // Global log file ptr
int incident_counter = 0;         // record num instances
sem_t incident_sem;               // keep incident_counter safe


/*****************************
 * Logging
 */

void start_custom_log(char* custom_fp){
    printf("Creating log file: %s\n", custom_fp);
    p_log = fopen(custom_fp, "a+");
    if (p_log == NULL){
        fprintf(stderr, "Log file: cannot open\n");
        exit(LOG_OPEN_FAILURE);
    }
}

void start_syslog(){
    printf("Starting syslog entry with ident: %s\n", SYSLOG_IDENT);
    openlog(SYSLOG_IDENT, SYSLOG_OPTIONS, SYSLOG_FACILITY);
}

void start_log(char* custom_fp){
    start_custom_log(custom_fp);
    if (c.syslogging){
        start_syslog();
        syslog(LOG_INFO, "STARTING BACKUPD\n");
    }
}

void logger(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    vfprintf(p_log, fmt, args);
    va_end(args);
    fflush(p_log);
    if(c.syslogging){
        va_start(args, fmt);
        vsyslog(LOG_INFO, fmt, args);
        va_end(args);
    }
}

void end_log(){
    logger("Closing log.\n");
    fclose(p_log);
    if(c.syslogging)
        closelog();
}

/******************************
 * Incident counter
 */

int get_incidents(){
    return incident_counter;
}

void reset_incidents(){
    incident_counter = 0;
}

void increment_incidents(){
    incident_counter++;
}

void init_incident_counter(){
    sem_init(&incident_sem, 0, 1);
    reset_incidents();
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
            end_log();
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

void write_pid(const char* pidfile){
    FILE* f = fopen(pidfile, "w");
    fprintf(f, "%d", getpid());
    fclose(f);
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
    scalar_config(config_f, c->backup, STR_CFG_FMT, sizeof(c->backup));

    scalar_config(config_f, &(c->syslogging), BOOL_CFG_FMT, LINE_WIDTH);

    scalar_config(config_f, &(c->incident_limit), INT_CFG_FMT, LINE_WIDTH);
    scalar_config(config_f, &(c->num_targets), INT_CFG_FMT, LINE_WIDTH);

    for(int target_idx = 0; target_idx < c->num_targets; target_idx++){
        target* conf = &(c->targets[target_idx]);
        target_config(config_f, conf, LINE_WIDTH);
    }

    fclose(config_f);
}


/******************************************
 * Events
 */

void backup_action(){
    logger("Calling backup command: %s\n", c.backup);
    system(c.backup);
}

void process_event(struct inotify_event* event,
                   target* targ){

    logger("File: %s/%s\n", targ->watch_dir, event->name);

    sem_wait(&incident_sem);

    increment_incidents();
    logger("Incident Count: %d\n", get_incidents());

    if(get_incidents() >= c.incident_limit){
        logger("Reached incident limit! Invoking backup_action.\n");
        backup_action();
        reset_incidents();
    }

    sem_post(&incident_sem);
}

void read_events(int fd, target* targ){
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

        process_event(event, targ);

        int total_event_size = EVENT_SIZE + event->len;
        offset += total_event_size;
    }
}

void* target_thread(void* param){
    target targ = *((target*) param);
    logger("Starting target thread: %s\n", targ.watch_dir);
 
    int inotify_fd = inotify_init();
    int watch_fd = inotify_add_watch(inotify_fd,
                                     targ.watch_dir,
                                     targ.mask);


    fd_set readfds, writefds, exceptfds;
    struct timeval tv;
    int retval;

    while (1){

        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        FD_ZERO(&readfds);
        FD_SET(inotify_fd, &readfds);

        tv.tv_sec = 1000;
        tv.tv_usec = 0;

        retval = select(MAX_TARGETS,
                        &readfds,
                        &writefds,
                        &exceptfds,
                        &tv);
        
        if (retval < 0){
            logger("Error reading inotify_fd: %d\n", inotify_fd);
        }
        else{
            logger("Received event in %s\n", targ.watch_dir);
            read_events(inotify_fd, &targ);
        }

    }
}

pthread_t start_target_thread(target* targ){
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_create(&tid, &attr, target_thread, targ);

    return tid;
}

void init_target_threads(target* targets, int num_targets){
    for(int targ_idx = 0; targ_idx < num_targets; targ_idx++){
        target* targ = &(targets[targ_idx]);
        pthread_t tid = start_target_thread(targ);
    }
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


/******************************************
 * Main
 */

int main(int argc, char* argv[]){

    const char* config_loc = config_location(argc, argv);

    read_config(config_loc, &c);

    daemonize();
    write_pid("/tmp/backupd.pid");
    start_log(c.log_loc);
    close_fds();

    // Initialization
    logger("Initializing backupd\n");

    init_incident_counter();
    init_target_threads(c.targets, c.num_targets);

    // Parent is done
    pause();

    exit(SUCCESS);
}
