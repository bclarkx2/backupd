#!/bin/bash 
# /usr/local/bin/backupd.sh 
# control script for backupd daemon

BACKUPD_CONFIG=""

function d_start ( ) 
{ 
    echo "backupd: starting service" 
    /usr/local/bin/backupd $BACKUPD_CONFIG
    sleep 3
    echo "backupd: done waiting for initialization"
}
 
function d_stop ( ) 
{ 
    echo  "backupd: stopping" 
    kill $(cat /tmp/backupd.pid)
    rm  /tmp/backupd.pid
 }
 
function d_status ( ) 
{ 
    ps -ef | grep backupd | grep -v grep
    echo "PID indicate indication file $(cat /tmp/backdup.pid 2&>/dev/null)"
}
 
# Some Things That run always
# nothing atm
 
# Management instructions of the service 
case "$1" in
    start)
        d_start
        ;;
    stop)
        d_stop
        ;;
    reload)
        d_stop
        sleep 1
        d_start
        ;;
    status)
        d_status
        ;;
    *)
    echo "Usage: $ 0 {start | stop | reload | status}"
    exit 1
    ;;
esac
 
exit 0
