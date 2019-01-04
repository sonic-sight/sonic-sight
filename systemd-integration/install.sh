#!/bin/bash 

set -x
set -o errexit
set -o nounset

mkdir -p ${HOME}/.local/share/systemd/user
# cp ./sonic-sight.service ${HOME}/.local/share/systemd/user
cp ./*.service ${HOME}/.local/share/systemd/user
for service in *.service
do
	systemctl --user enable "$service"
done
# systemctl --user enable sonic-sight.service
# systemctl --user enable host-network-delay-restart.service
systemctl --user daemon-reload

systemctl --user stop sonic-sight.service || true
systemctl --user start sonic-sight.service


