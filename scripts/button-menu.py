#!/usr/bin/env python3

import subprocess
import time
import re
import logging
import sys
import os
import threading

logger = logging.getLogger()
log_level = logging.INFO
log_level = logging.DEBUG

logger.setLevel(log_level)
ch = logging.StreamHandler(sys.stdout)
ch.setLevel(log_level)
formatter = logging.Formatter('%(asctime)s %(levelname)6s|%(process)5d| %(message)s')
ch.setFormatter(formatter)
logger.addHandler(ch)

import RPi.GPIO as GPIO

# Configuration
script="/home/sonic/sonic-sight/run-with-parameters.sh"
sounds="/home/sonic/sonic-sight/scripts/sounds"
bounce_time = 500
shutdown_hold_time = 3
PIN = {
    "volume_up":21,
    "volume_down":19,
    "shutdown_or_stop":7,
    "switch_distance":13,
    "switch_frequency_doubling":11
}
range_groups = [
    "INSIDE", "STREET", ]
range_variable = "RANGE"
frequency_doubling_groups = [
    "CONSTANT", "DOUBLING", ]
frequency_doubling_variable = "FREQUENCY"


def get_variable( var_name, var_groups ):
    with open(script, 'r' ) as f:
        content = f.read()
    var_m=re.search(r'^' + var_name + r'="(?P<var>.*)"',content,flags=re.MULTILINE)
    if var_m.group("var") not in var_groups:
        logger.error("Error: variable " + var_name + " set not to one of the possible settings "
                     + str(var_groups) )
        sys.exit(1)
    return var_m.group("var")


def set_variable( var_name, var_value ):
    cmd='''sed -i -e 's/^''' + var_name + '''=.*/'''\
        + var_name + '''="''' \
        + var_value + '''"/g' "''' + script + '''"'''
    subprocess.run(cmd, shell=True)

play_process = None

def play_message( message ):
    global play_process
    if play_process is not None:
        logger.debug("Killing play process")
        play_process.kill()
    message = message.lower()
    msg = message.replace(" ","_")
    logger.debug("Playing message : " + message )
    msg_sound = sounds + "/" + msg + ".wav"
    if not os.path.isfile( msg_sound ):
        logger.error("Error: message has no sound file")
        logger.error("Run :" + "pico2wave -w \"" +
                     msg_sound + "\" \"" + message + "\"" )
        cmd="""pico2wave -w \"""" +\
            msg_sound + """\" \"""" + message + """\""""
        print("RUNNING: " + cmd)
        subprocess.run(cmd, shell=True)

        # sys.exit(1)
    # cmd="aplay \"" + msg_sound + "\""
    logger.debug("Starting play plrocess")
    cmd="cvlc --play-and-exit --gain=0.5 \"" + msg_sound + "\""
    print("PLAYING MESSAGE: ", message )
    play_process = subprocess.Popen( cmd, shell=True )
    logger.debug("Play process is running in background")
    # subprocess.run(cmd, shell=True)


# Initial parameters
current_range = get_variable( range_variable, range_groups )
current_frequency_doubling = get_variable(
    frequency_doubling_variable,
    frequency_doubling_groups
)


def set_volume( up_down="UP" ):
    play_message("volume " + up_down)
    cmd=r"amixer -d -c 1 sset Speaker 5%" + (r"+" if up_down == "UP" else r"-")
    subprocess.run( cmd, shell=True )


shutdown_hold_on = threading.Semaphore()
shutdown_last_button_time = time.time() - 100

def shutdown_or_stop():
    global shutdown_hold_on
    global shutdown_last_button_time
    if not shutdown_hold_on.acquire(blocking=False):
        logger.debug("Could not acquire shutdown-or-stop semaphore")
        return
    # if time.time() - shutdown_last_button_time < bounce_time:
    #     logger.debug("Shutdown button pressed within bounce time")
    #     shutdown_hold_on.release()
    #     return
    logger.debug("Acquired shutdown-or-stop semaphore")
    t1 = time.time()
    t2 = time.time()
    hold_on = 1
    while t2 - t1 < shutdown_hold_time and hold_on:
        time.sleep(0.2)
        hold_on = GPIO.input(PIN["shutdown_or_stop"])
        t2 = time.time()
    if hold_on:
        play_message("Shutting down the device, wait 30s before unplugging the cable")
        try:
            play_process.wait(10)
        except subprocess.TimeoutExpired:
            logger.error("Timout expired in waiting for play process")
        subprocess.call( "sudo shutdown -P now", shell=True )
    else:
        ret = subprocess.call("systemctl --quiet --user is-active sonic-sight.service", shell=True)
        if ret == 0: # return code 0 means that the service is alive
            play_message("Toggling sound rendering service to stopped")
            subprocess.call("systemctl --user stop sonic-sight.service", shell=True)
        else:
            play_message("Toggling sound rendering service to started")
            subprocess.call("systemctl --user start sonic-sight.service", shell=True)
    logger.debug("Releasing shutdown-or-stop semaphore")
    shutdown_last_button_time = time.time()
    shutdown_hold_on.release()


def switch_distance():
    global current_range
    current_range=range_groups[(range_groups.index(current_range) + 1) % len(range_groups)]
    play_message("Changing distance to " + current_range )
    set_variable( range_variable, current_range )
    subprocess.call("systemctl --user restart sonic-sight.service", shell=True)


def switch_frequency_doubling():
    global current_frequency_doubling
    current_frequency_doubling=frequency_doubling_groups[
        (frequency_doubling_groups.index(current_frequency_doubling) + 1) % len(frequency_doubling_groups)]
    play_message("Changing frequency doubling to " + current_frequency_doubling )
    set_variable( frequency_doubling_variable, current_frequency_doubling )
    subprocess.call("systemctl --user restart sonic-sight.service", shell=True)


GPIO.setmode(GPIO.BOARD)
print(sorted(PIN.values()))
GPIO.setup( list( PIN.values() ), GPIO.IN, pull_up_down=GPIO.PUD_DOWN )

GPIO.add_event_detect(
    PIN["shutdown_or_stop"], GPIO.RISING,
    callback=lambda ch : shutdown_or_stop(), bouncetime=bounce_time)

GPIO.add_event_detect(
    PIN["volume_up"], GPIO.RISING,
    callback=lambda ch : set_volume(up_down="UP"), bouncetime=bounce_time)

GPIO.add_event_detect(
    PIN["volume_down"], GPIO.RISING,
    callback=lambda ch : set_volume(up_down="DOWN"), bouncetime=bounce_time)

GPIO.add_event_detect(
    PIN["switch_distance"], GPIO.RISING,
    callback=lambda ch : switch_distance(), bouncetime=bounce_time)

GPIO.add_event_detect(
    PIN["switch_frequency_doubling"], GPIO.RISING,
    callback=lambda ch : switch_frequency_doubling(), bouncetime=bounce_time)

try:
    while True:
        time.sleep(10)
except KeyboardInterrupt:
    GPIO.cleanup()

GPIO.cleanup()
