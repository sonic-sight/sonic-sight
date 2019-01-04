#!/usr/bin/env python3

import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BOARD)

if False:
    invalid_channels = [ ]
    valid_channels = []

    for i in range(40):
        if i in invalid_channels:
            print(i, " is invalid, continuing" )

        print("Testing pin ", i )
        try:
            GPIO.setup( i, GPIO.IN, pull_up_down=GPIO.PUD_DOWN )
            val = GPIO.input(i)
            print("Value is ", val)
            valid_channels.append(i)
        except ValueError as e:
            invalid_channels.append(i)

    print("All invalid channels are : ")
    print(invalid_channels)
    print("All valid channels are : ")
    print(valid_channels)
    import sys
    sys.exit(0)

# chan_list = [ 3, 5, 7, 11, 13, 15, 19, 21, 23 ]
chan_list = [3, 5, 7, 8, 10, 11, 12, 13, 15, 16, 18, 19, 21, 22, 23, 24, 26, 29, 31, 32, 33, 35, 36, 37, 38]

GPIO.setup( chan_list, GPIO.IN, pull_up_down=GPIO.PUD_DOWN )

def read_channels():
    return [ GPIO.input(ch) for ch in chan_list ]

state = read_channels()

print("State = ", state )

last_state = state

while True:
    state = read_channels()
    diff = [ x - y for x,y in zip(state,last_state) ]
    print(state)
    if diff != [0,]*len(diff):
        try:
            idx = diff.index(1)
            change = "DOWN"
        except ValueError as e:
            idx = diff.index(-1)
            change = "UP"
        print("Button ", chan_list[idx], " was ", change )
        last_state = state
    time.sleep(0.5)


