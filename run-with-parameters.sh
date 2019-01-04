#!/bin/bash 

set -x
# while [ 1 ]
# do 
# 	sleep 1
# 	date
# done
# 
# exit 0


if [[ "$(id -u -n )" == "sonic" ]]
then
	cmd="${HOME}/sonic-sight/release_build/render-to-sound"
	# cmd="${cmd} --save-depth-to=./data/frame "
	export DISPLAY=:0
	pulseaudio --check || pulseaudio -D

	# USB sound card
	pacmd set-default-sink 'alsa_output.usb-C-Media_Electronics_Inc._USB_PnP_Sound_Device-00.analog-stereo'
	# built-in sound card:
	# pacmd set-default-sink 'alsa_output.0.analog-stereo'
else
	cmd="./release_build/render-to-sound --save-depth-to=./data/frame "
	# cmd="./release_build/render-to-sound "
	# cmd="./debug_build/render-to-sound --save-depth-to=./data/frame "
	# cmd="gdb --args ./debug_build/render-to-sound --save-depth-to=./data/frame "
fi

# Range is either "INSIDE" or "STREET"
RANGE="STREET"
# Frequency is either "CONSTANT" or "DOUBLING"
FREQUENCY="CONSTANT"

parameters=(
"--camera-width=640" \
"--camera-height=480" \
"--camera-fps=6" \
"--renderer-start-frequency=1414.2" \
"--renderer-lower-frequency=1414.2" \
"--renderer-lower-amplitude=0.0" \
"--renderer-interval-extra-time=0.01" \
)
# if lower max distance is unset, lower rendering is disabled
# "--renderer-lower-frequency-doubling-length=20." \
#

case "$RANGE" in
	"INSIDE")
		parameters=("${parameters[@]}" \
"--renderer-max-distance=3.0" \
"--renderer-step-distance=0.01" \
"--renderer-speed-of-sound=2.0" \
"--renderer-stereo-distance=0.35" \
"--renderer-base-frequency=2000.0" \
)
		if [[ "$FREQUENCY" == "DOUBLING" ]]
		then
		parameters=("${parameters[@]}" \
"--renderer-freq-doubling-length=6." \
"--renderer-base-amplitude=1.5" \
)
		else
		parameters=("${parameters[@]}" \
"--renderer-start-amplitude=25." \
"--renderer-start-duration=0.02" \
)
		fi
		;;
	"STREET")
		parameters=("${parameters[@]}" \
"--renderer-max-distance=6.0" \
"--renderer-step-distance=0.05" \
"--renderer-speed-of-sound=6.0" \
"--renderer-stereo-distance=0.5" \
"--renderer-base-frequency=2000.0" \
)
		if [[ "$FREQUENCY" == "DOUBLING" ]]
		then
		parameters=("${parameters[@]}" \
"--renderer-freq-doubling-length=12." \
"--renderer-base-amplitude=1.5" \
)
		else
		parameters=("${parameters[@]}" \
"--renderer-start-amplitude=25." \
"--renderer-start-duration=0.02" \
)
		fi
		;;
	*)
		echo "Unknown RANGE = $RANGE"
		exit 1
		;;
esac

${cmd} \
"${parameters[@]}"


