Technical Specification
=======================

This document describes the technical details of the current implementation
and some possible improvements

Brief overview
--------------

__Idea:__ The idea of the project is quite simple: 
calculate the echo of a short pulse of sound with
a specified frequency,
as it would be recorded by a stereo receiver, 
if the speed of sound would be much slower.

__Goal:__ The goal is to investigate if such slowed-down echo
can convey enough information about the environment
to person with loss of sight to be usable.

The project uses the following hardware components:
- Intel Realsense (TM) 435 depth-vision camera
- Raspberry Pi v3 with a micro SD card (16GB micro SDHC) and a case
- Powerbank to power the RPi with the camera
- USB extension cable for the camera
- Selfie stick to hold the camera
- Headphones (headphones that do not isloate the environmental sounds)
- (optional) bone conduction headphones to keep the ears free
- (optional) cheap USB sound card (RPi's sound is kind of bad, but still usable)
- (optional) gadgets to attach the camera somewhere, 
	(i.e. chest mount, head mount or something else if you want to use the camera without your hands)

The depth-vision camera output is rendered into stereo sound
on a raspberry pi device. 
Initial experimentation determined that the virtual speed
of sound should be at approximately 5 m/s (~100x slowdown),
and that it is useful to increase the frequency of the generated
sound with the passing of time, to better convey the information
about the passing of time.

Inial experimentation involved emitting a short pulse to signal
the start time (when the virtual sound is sent to the environemtn)
and a longer pulse with the echo information. 
Current implementation skipps the initial short pulse 
and increases the frequency of the returning echo sound 
with the passing of time.

Technical details
-----------------

The software currently supports the following options:
```
Possible parameters:
[external communication]
--signal-pid=<pid> : 
	 pid of the process that is sent SIGUSR1,
	 after pointcloud is sved in the specified file
--signal-depth-filename=<filename> : 
	 filename to save depth to before signaling to an external process
[camera]
Caution: some parameter combinations are not supported by the camera,
and over an USB2 connection the selection of available parameters is even smaller
--camera-width=<width=1280> : 
	 depth camera width in pixels
--camera-height=<height=720> : 
	 depth camera height in pixels
--camera-fps=<fps=15> : 
	 depth camera frame rate
[data archiving]
--save-depth-to=<filename-template> : 
	 pointclouds and generated waveforms are saved to 
	 <filename-template>__<timestamp>.{dat|waveform}
[depth rendering]
--renderer-max-distance=<max distance=4.0> : 
	 max distance for depth renderer
--renderer-step-distance=<step distance=0.005> : 
	 step in distance for depth renderer (depth resolution)
--renderer-speed-of-sound=<speed of sound=1.0> : 
	 speed of sound for depth renderer [m/s]
--renderer-stereo-distance=<stereo distance=0.2> : 
	 distance between the depth renderer's "ears" [m]
--renderer-base-frequency=<frequency=1000.0> : 
	 base frequency for depth renderer [Hz=1/s]
--renderer-freq-doubling-length=<frequency doubling=infinity> : 
	 length at which the frequency doubles [m]
--renderer-base-amplitude=<amplitude=0.0> : 
	 base amplitude (added to signal) [%]
--renderer-start-frequency=<frequency=1000.0> : 
	 start signal frequency for depth renderer [Hz=1/s]
--renderer-start-duration=<duration=0.01> : 
	 start signal duration [s]
--renderer-start-amplitude=<amplitude=50.0> : 
	 start signal amplitude [% of max]
--renderer-interval-extra-time=<extre time=0.1> : 
	 extra time to wait between sound renders [s]
	 (time between renders is this time + <max distance> / <speed of sound>
--renderer-lower-distance=<lower distance=NaN> : 
	 the lower renderer is this far below the main renderer [m]
	 if left unset, NaN value is set to signal that the lower channel should not be used
--renderer-lower-frequency=<lower frequency=500> : 
	 base frequency of the lower renderer [Hz]
--renderer-lower-frequency-doubling-length=<lower freq. doubling=infinity> : 
	 length at which the lower signal's frequency doubles [m]
--renderer-lower-amplitude=<amplitude=0.0> : 
	 lower signal base amplitude (added to signal) [%]
```

In brief: 
- it is possible to configure the speed of sound,
the distance between the listener's ears and
the distance at which the frequency effectively doubles
(the frequency increases exponentially with time/distance).
- It is also possible to add anoter sound generated
from two "ears" that are the specified distance below
the first pari. This "lower" pair of ears was meant to allow
the user an additional alert for low obstcles 
or holes in the ground.
- It seems to be dificoult to understand the meaning
of two signals at two different frequencies,
so the user is advised not to use the lower pair of ears
(but let me know if you can make it useful).


Improvements
============
This section covers some ideas on how things could be improved,
modified, implemented in hardware, features that could be added...

It is here to **prevent patenting** of the mentioned things
(the world has gone insane with patents it seems),
as it is the author's oppinion that the here included
software and any hardware implementations of the same
ideas should remain free for all.

It is here because publishing something makes it unpatentable by others.

It is also here to provide ideas on how to improve things.

Not very well organized, basically a "braindump" of ideas.


Algorithm improvements / added functionality
--------------------------------------------
This section covers ideas on what could be added to
the algorithm to make the software more usable

* Automatic adaptation of maximal distance / speed of sound
Monitor the distances in the environment and possibly 
automatically adopt the parameters to the environment.
I.e. if the max observed distance is 4m, we can conclude that
the user is probably inside, so it makes sense to switch to
such configuration. Probably best to provide an option 
to supply the set of parameters for an "indoors" configuration
and a set of parameters for "outdoors", and possibly
one more set of parameters for working at a table "table".

* Sound marks at specific distance intervals
Make some additional beps at specific distance intervals to improve perception of the passing of time

* Filling of holes
Render the sounds as if comming from distances that a ball
with R=15cm can reach, if thrown from coordinate origin.
In this way the users can better perceive fences,
thin lamp posts, ...
One could render the points on the sourface of this virtual ball or the points
where its center lies when its motion has stopped.

* real sound scattering (set on/off with optional parameter)
implement real sound scattering: sound echos from surfaces
perpendicular to the viewer should be louder than echos from tilted sourfaces,
possibly cos(something) formula or some easy and fast to calculate approximation 

* Different emphasis for vertical surface changes than for horizontal surface changes
Fill holes only around vertical objects, so that the user is
still alerted on possible holes in the ground.
Possibly increase frequency/amplitude around detection of special objects, such as holes in the ground to alert the user on the objects.

* For persons that can hear only on one ear
Render one side with a different frequency and play both sounds on the ear that they can hear

* Special alerts on specific objects, and possibly objec-specific actions
such as signs (read the sign?), horizontal surfaces (artificially reduce the signal?), lamp post (increase the signal),
traffic lights (read which light is on) ...

* Guestures support
tilting the head by the user means something

* Hardware buttons ! (yes, it is a feature...)

* different "sound cone" radius depending on external parameters / buttons pressed by the user / bio signals from the user
so that the user can focus more on some objects than the others.
Possibly dynamically change some other parameters like max rendering distance based on this

* different rendering times based on external parameters / buttons pressed by the user / bio signals from the user

* use muscle movement through some sensors as input signals / buttons
for use for instance for changing the radius of the projected sound cone.
I.e. the user can tighten the muscles around his eyes to make 
the sound cone more sharp and focused, so that the user can better understand
the object at focus

* detection of a wider area of environment
Use more cameras and detect larger area of environment. Possibly render only a fraction of that environment.
More environment can be captured with different kinds of equipement

* render only a fraction of the environment, the fraction that we deduce the user finds most interesting
i.e. detect 360 deg of environment but render only the part at which the user's head is pointing

* detect the user's head position and direction
and possibly render only a focused part of the environment based on this information
(the part the user would see)

* using a mapping of time (rendering vs f(t) instead of vs t t=time)
As the area in spatial angle per distance shrinks, 
perhaps it would be more intuitive to render time non-linearly ?
Perhaps best with such mapping that the signal of the floor does not fall as 1/r, but is linear
or even constant ?
Perhaps also log or exp functions.

* render the floor differently
detect where the flor is and render it differently or in a different rendering pass,
possibly render obstacles on the floor differently (i.e. much higher frequency for the holes in the ground or changes in ground level)

* rendering as if everything would be at some specific distance
(make differences in times of arrival at both ears the same
as if all objects would be for example 1m away),
or make the distance between signal arrival times at both ears
a function of the angle from the camera

* rendering different heights of the image 
at different frequencies,
assigning special frequency(/-ies) to an area(/areas) of interest 
in the image, either assigned statically 
(i.e. bottom half of the image)
or dynamically (where the software recognizes is a hole in the road or some obsacle).

* rendering with different renderers in conscutive pulses
i.e. first render normally with one frequency set as base frequency,
then render just the botton malf of the image with another frequency
In this way the brain can get info about the surrounding space (where the walls are) on the first pulse
and the info about any obstacles on the floor (such as a low-lying obstacle, like a step to get on the pavement)

* rendering based on the total environment
Use a spatial awareness library that constructs the environment around you 
(i.e. arcore, arkit, slam\_gmapping or similar)
and render the whole environment, not just what the camera saw on the last sweep

* sweep the camera to increase the viewing angle
by physically moving the camera or using a mirror

* use a wider-angle camear or optics to increase the viewing angle

* rendering differences
render the difference in the perceived environment rather than the environment.
Probably most useful in combination with rendering with different renderes,
i.e. render differences in step 2. and render the environment on step 1.

* use a similar algorithm to render images to sound
(and possibly use this as a plugin / function provider for other software, i.e. web browser)
use similar renderers, possibly changing color to depth or something similar

* increasing the speed of time as an exponential function of time,
when rendering echos.
This way we can render a larger set of distances, but still keep good resolution
of the most important (=closest) distances. 
Some other function (i.e. linear, quadratic, splines, ... )
can be used instead of the exp.

* emit warning sounds when depth calculation is unreliable 
with D435 when there is too much light, depth perception is very poor
(or maybe I just haven't configured my D435 properly...)


Implementations in hardware
---------------------------
This section covers ideas on how to implement this in hardware.
Hopefully the resulting device could be much smaller than
the current device, possibly implemented in the 
form of glasses or a clip-on.

* using a vibrator instead of speakers/headphones to convey
the same information to people that have both
loss of hearing and loss of sight

* using an array of mechanical pressure applicators
to convey the same information to people 
that also have a loss of hearing. 
One could "plot" a waveform on the skin, 
plot some distances chart on the skin or something else.

* time-of-flight camera implementation
Make a pulse of the entire environment, and a time-delayed
window for detection, and scan the entire range of possible time delays, 
conveying each returnging amplitude of light into an amlitude of sound
at a corresponding delay.
Normally, time-of-flight cameras are used as a depth camera (some versions of MS Kinect),
where time of flight for each pixel is recorded separately.
We don't need this here for the most basic operation mode. 
We could use a simple distance meter and integrate the responses we get from all angles
(in principle).

We could have two of these in both corners of the glasses,
to get stereo sound.
Possibly we could increase the depth perception by interleaving
the frames and recording the light in the opposite corner than
we released it

* Speaker + stereo microphone implementation
Use high sampling rate sound card. Perhaps at a frequency
humans can't hear (possibly also pet friendly).

* use a vertical window equal to one column in a normal frame in combination with the time-of-flight
"pseudo camera" implementation
as this would enable to add additional renderers in hardware, 
i.e. renderer that renders all sound differences as if the object would be 1m away from the observer.










