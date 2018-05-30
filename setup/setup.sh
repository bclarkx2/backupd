#/bin/bash
# install backupd daemon as systemd unit

# if anything fails, we have a problem
set -e

# grab location of config file, save as env var
if [[ -z "$1" ]] ; then
    echo "Usage: ./setup.sh config_file"
    exit 1
elif [[ ! -f "$1" ]] ; then
    echo "config_file must exist! See sample.config"
    exit 1
else
    BACKUPD_CONFIG="$1"
    sed -i "s,BACKUPD_CONFIG=.*,BACKUPD_CONFIG=\"$BACKUPD_CONFIG\",g" "backupd.sh"
    echo "config file location set: $BACKUPD_CONFIG"
fi

# generate executable
make -f ../src/makefile

# copy files to appropriate places
sudo cp backupd.sh /usr/local/bin/
sudo cp backupd.service /etc/systemd/system/
sudo cp ../out/backupd /usr/local/bin/

# reload systemd units
sudo systemctl daemon-reload

# enable backupd service
sudo systemctl enable backupd
sudo systemctl start backupd
