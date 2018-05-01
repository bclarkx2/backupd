# backupd
A Linux daemon that monitors the filesystem

# Files

* README.md: design document
* src/backupd.c: C source file containing code for the backupd daemon
* src/makefile: Makefile to build project

# Design

## Overview

`backupd` is a Linux daemon written in C. Its primary purpose is to monitor select portions of the user's filesystem for changes. If it notices a volume of changes exceeding a user-specified threshold, it will trigger a backup operation of the user's choice.

Notable characteristics:
* The daemon runs continuously without manual intervention
* The daemon's behavior is specified by use of a config file
* All daemon activities are logged to both a custom log file and to the system logs.

## Usage
The `backupd` daemon can be started manually via the command line with the following syntax:

`./backupd config_file`

To test, you will need to edit the .config file. The provided `sample.config` is an example config file. Just replace the filepaths with paths that you want on your system. (Note: the target directories must already exist).

Then, create files in the directories you set a target on. Each created file should show up as an event on the log. When the incident limit is reached, the backup command you specified in the config file will execute.

Note: this following section has not been fully implemented
If you want `backupd` to start automatically on system boot:
1. Copy the `backupd` executable to `/usr/local/bin`
2. Copy the `backupd_startup` script to `/etc/init.d/backupd_startup`
3. Ensure the script is executable (`chmod +x /etc/init.d/backupd_startup`)
4. Run the command `update-rc.d backupd_startup defaults`
5. The `backupd` will now start automatically on boot.
6. Control the daemon with `service backupd ____`

## Detailed Design

### Config file
The `backupd` daemon is controlled by the settings written to its configuration file. The config file contains whitespace separated data in a strict layout. The following information is contained:

* Log file location: filesystem path for custom log file
* Incident limit: the number of filesystem incidents `backupd` needs to see before invoking the backup action
* Backup action: a shell command to be invoked when the number of filesystem incidents reaches the predefined limit.
* Targets: list of directories to observe
 * Target location
 * Criteria for filesystem events to count as incidents in this target (file creation, file modification, etc...)

All of the above information is read into the `struct config` data type, using the `read_config` method and various `*_config` helper functions.

Each target is stored in the `struct target` data type. This struct contains all the information required to properly process an event detected in that specific target dir.

For an example config file, see `sample.config`

### Daemonization

The `daemonize` method transforms the `backupd` process into a full-fledged daemon process. 

### Logging

`backupd` uses the `start_log` function to initialize its logs, and the `logger` method to record messages to them. `end_log` will release any logging resources used.

The daemon uses a global file descriptor, `p_log`, to refer to its custom log file (the location of which is set by the config file.)

If syslogging is enabled, all messages are sent with the identifier "backupd", along with the process ID. All messages have priority LOG_INFO.

### Signal handling

Extreme conditions may require communication with the `backupd` daemon. Since it is not connected to a TTY, we must communicate with it via signals. The `signal_handler` function is essentially a big switch statement that allows `backupd` to respond to a variety of signals.

### Event handling

`backupd` starts a new thread for each of the target directories in its config file. That thread uses the `select` library function to block until it receives an `inotify` event indicating that a filesystem event has occured in the target directory. The thread can then hand off the event to the `process_event` function (along with the `target` struct).

`backupd` maintains an `incident_counter` global counter. This counter is incremented each time one of the threads detects an incident. It is kept threadsafe using a semaphore.

The `process_event` function is responsible for updating the `incident_counter` state, and potentially invoking the backup action.

### Backup action

The `backup_action` function is responsible for invoking whatever shell command is set in the config file. It is advised that this backup command write some sort of results elsewhere in the filesystem, as its progress is not recorded by the backupd daemon itself.

## Sample Output

The log file below is the results of running the daemon with the below config file. Note that this config file does not contain all the information the final version will contain.

### Sample Log File
```
Initializing backupd
Starting target thread: /home/bpc30/test
Received event in /home/bpc30/test
File: /home/bpc30/test/file1
Incident Count: 1
File: /home/bpc30/test/file2
Incident Count: 2
File: /home/bpc30/test/file3
Incident Count: 3
File: /home/bpc30/test/file4
Incident Count: 4
File: /home/bpc30/test/file5
Incident Count: 5
File: /home/bpc30/test/file6
Incident Count: 6
File: /home/bpc30/test/file7
Incident Count: 7
File: /home/bpc30/test/file8
Incident Count: 8
File: /home/bpc30/test/file9
Incident Count: 9
File: /home/bpc30/test/file10
Incident Count: 10
Reached incident limit! Invoking backup_action.
Calling backup command: echo "<external update cmd output>" >> /home/bpc30/backupd.log 2>&1
<external update cmd output>
SIGTERM received
Closing log.
```

## Failures

`backupd` has at least the following error codes:

Code | Name | Meaning
-----|------|------------
1 | FAILURE | Generic application failure
2 | FORK_FAILURE | Unable to fork child process during daemonization
3 | LOG_OPEN_FAILURE | Unable to open custom log file
4 | SID_FAILURE | Unable to set unique session id during daemonization
5 | CHDIR_FAILURE | Unable to change working directory during daemonization
6 | CLI_FAILURE | Incorrect command line usage
7 | CONFIG_FAILURE | Error reading config file
