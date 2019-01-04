#!/bin/bash

LOG_FILE=/home/sonic/network.log


# Close STDOUT file descriptor
exec 1<&-
# Close STDERR FD
exec 2<&-

# Open STDOUT as \$LOG_FILE file for read and write.
exec 1<>$LOG_FILE

# Redirect STDERR to STDOUT
exec 2>&1

set -x

date 
id
env 

/bin/sleep 10
/usr/bin/nmcli con down Wlan0Host
/bin/sleep 3
/usr/bin/nmcli con up Wlan0Host
