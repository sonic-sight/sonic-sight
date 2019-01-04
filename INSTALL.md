Overview
========
This document is split in two parts: software and hardware.
Software chapter describes how to get all things running on a RPi3,
hardware part describes my recommendations for building a hardware interface to the software.

You will need a RPi3 (with at least 8GB memory card) and Intel® RealSense™ D435 depth vision camera.
Optional parts that make life easier:
* powerbank to make it portable
* body harness camera holder / any other camera holder
* enclosure for RPi with some hardware buttons, wired to some input pins
You can also use it on a laptop running linux with the camera directly attached to the laptop.
Probably it will work with other microcomputers as well, and most probably with some other
depth vision cameras supported by Intel's librealsense.

In current state the instructions presume you will be able to solve build issues on your own.
Tested on ubuntu-mate release for RPi (16.04).
User 'sonic' was used in the testing, and the username might be hardcoded into some scripts.


Software
========

Install librealsense on rpi (follow instructions from librealsense repo, you will need to add swap before compilation), 
and then run 
```
make release
```
on rpi where you copied this repo.

To make the app standalone and automatically start on RPi boot, 
configure RPi to automatically login the selected user 
and install systemd services 
```
cd systemd-integration
./install.sh
```
which will install systemd services in your user account.

Start the program by running a wrapper script
```
./run-with-parameters.sh
```
which starts the program with some pre-selected parameters.

TODO: specify build dependencies here, give more detailed instructions.

Hardware
========
It is sometimes useful to be able to control the application through hardware buttons,
and that is what `scripts/button-menu.py` is for. It controlls some aspects of the program through
hardware buttons connected to RPi's AI pins.

Wire some buttons to 



