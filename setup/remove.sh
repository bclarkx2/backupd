#/bin/bash
# uninstall all backupd systemd unit files

# stop daemon if running
sudo systemctl stop backupd

# remove files
sudo rm /usr/local/bin/backupd
sudo rm /usr/local/bin/backupd.sh
sudo rm /etc/systemd/system/backupd.service

# update systemd
sudo systemctl daemon-reload
