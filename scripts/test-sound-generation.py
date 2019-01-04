#!/usr/bin/env python3

# This script is used to test how different sounds sound like

import pyaudio
import numpy as np
import time
import sys

import pyaudio
import numpy as np

p = pyaudio.PyAudio()

volume = 0.5     # range [0.0, 1.0]
SAMPLE_RATE = 44100       # sampling rate, Hz, must be integer
DURATION = 3.0   # in seconds, may be float
N = SAMPLE_RATE * 4
t = np.arange(N) / SAMPLE_RATE

def make_signal(
        base_frequency,
        t0
):
    # return (np.sin(2 * np.pi * np.arange(SAMPLE_RATE * DURATION) * base_frequency / SAMPLE_RATE))
    phase = t * base_frequency
    # phase = t * base_frequency + base_frequency * (np.exp(t) - 1)/(np.log(2.)*k)
    # phase = base_frequency * t0 / np.log(2.) * ( np.exp(t) - 1. )
    phase = base_frequency * t0 / np.log(2.) * ( np.exp( np.log(2.) / t0 * t) - 1. )
    print("Final frequency would be ", base_frequency * 2**(max(t)/t0) )
    return np.sin(2 * np.pi * phase )

# generate samples, note conversion to float32 array

samples = np.zeros_like(t,dtype=np.float32)
k = np.sqrt(2.)
f = 1000.
t0 = 2.
t0_envelope = 1.5
envelope = np.abs( np.sin( 2 * np.pi * t / t0_envelope) )
samples += make_signal( f, t0 )
# samples += make_signal( k*f, t0 )
# samples += make_signal( 200.0, 2. )
# samples += make_signal( 440.0, 2. )

samples = samples * 0.25

from scipy.io.wavfile import write as write_wavefile
write_wavefile( "./test.wav", SAMPLE_RATE,
                samples )

import subprocess
# subprocess.run( "mplayer test.wav", shell=True )
subprocess.run( "vlc test.wav", shell=True )

# for paFloat32 sample values must be in range [-1.0, 1.0]
if False:
    # pyaudio seems to produce alsa buffer underruns ?
    stream = p.open(format=pyaudio.paFloat32,
                    channels=1,
                    rate=SAMPLE_RATE,
                    output=True)

    print("Play start")
    t1 = time.time()
    # play. May repeat with different volume values (if done interactively)
    stream.write(0.5*samples.astype(np.float32))
    t2 = time.time()
    print("Play stopped, duration = ", t2-t1)
    print( samples.shape, samples.shape[0]/SAMPLE_RATE )


    stream.stop_stream()
    stream.close()

    p.terminate()

sys.exit(0)

from matplotlib import pyplot as plt
ll = plt.plot( samples )
plt.show()
