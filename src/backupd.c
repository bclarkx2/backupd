
/***************************************************
 * backupd.c
 * Author: Brian Clark
 * Date: 4/12/18
 * Description: Linux filesystem monitoring daemon
 */

#include <stdio.h>

int main(int arc, char* argv[]){


    // Steps

    // Fork parent process

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
