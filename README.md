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
The `backupd` daemon can be started via the command line with the following syntax:

`./backupd log_file`


## Detailed Design

