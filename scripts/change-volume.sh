#!/usr/bin/env bash

# This script changes volme and other things from command line,
# useful for connecting to rpi from a simple cli client (i.e. connect bot for android)
# To make it able to shut down the pi without password, add
# user_name ALL=(ALL) NOPASSWD: /sbin/poweroff, /sbin/reboot, /sbin/shutdown
# to /etc/sudoers.d/shutdown_nowp and chmod it to 440, owner root:root

function main_menu() {
echo "Main menu:"
echo "v : set volume"
echo "r : restart service"
echo "s : stop service"
echo "d : shutdown device"
echo "c : configure service (and restart it when done)"
}

while [[ 1 ]]
do
main_menu
echo "Main menu [vrsdc]: "
read -n1 menu
case "$menu" in
"v")
        while [[ 1 ]]
        do
        echo "Volume set: Up = u, Down = d, Quit = q"
        read -n1 vol

        case "$vol" in
            "d")
                amixer -d -c 1 sset Speaker 5%-
                ;;
            "u")
                amixer -d -c 1 sset Speaker 5%+
                ;;
            "q")
                break
                ;;
            *)
                "Pressed key : $vol, doing nothing"
                ;;
        esac

        done
    ;;
"r")
    echo "Restarting the service"
    systemctl --user restart sonic-sight.service
    ;;
"s")
    echo "Stopping the service"
    systemctl --user stop sonic-sight.service
    ;;
"d")
    sudo shutdown -P now
    ;;
"c")
    script="/home/sonic/sonic-sight/run-with-parameters.sh"
    read -p "Which distance ? i [inside] / s [street]" -n1 distance
    case "$distance" in
    "i")
        sed -i -e 's/^RANGE=.*/RANGE="INSIDE"/g' "${script}"
        ;;
    "s")
        sed -i -e 's/^RANGE=.*/RANGE="STREET"/g' "${script}"
        ;;
    *)
        echo "Wrong distance"
        continue
        ;;
    esac
    read -p "Double the frequency? d [doublie] / c [constant]" -n1 freq
    case "$freq" in
    "d")
        sed -i -e 's/^FREQUENCY=.*/FREQUENCY="DOUBLING"/g' "${script}"
        ;;
    "c")
        sed -i -e 's/^FREQUENCY=.*/FREQUENCY="CONSTANT"/g' "${script}"
        ;;
    *)
        echo "Wrong frequency setting"
        continue
        ;;
    esac
    systemctl --user restart sonic-sight.service
    ;;
*)
    echo "Wrong key in main menu"
    ;;
esac
done

set -x
while [[ 1 ]]
do
echo "Volume set: Up = u, Down = d, Quit = q"
read -n1 vol

case "$vol" in
    "d")
        amixer -d -c 1 sset Speaker 5%-
        ;;
    "u")
        amixer -d -c 1 sset Speaker 5%+
        ;;
    "q")
        break
        ;;
    *)
        "Pressed key : $vol, doing nothing"
        ;;
esac

done