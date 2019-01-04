#!/bin/bash

set -o errexit
set -o nounset

# pointclouds=( "./test-data/test1/*.dat" )
# readarray pointclouds < <(ls ./test-data/tree1/*.dat)
# readarray pointclouds < <(ls ./test-data/stairs-descending/*.dat)
# readarray pointclouds < <(ls ./test-data/stairs-ascending/*.dat ./test-data/gate-barrier/*.dat ./test-data/parked-bicycles/*.dat ./test-data/flower-pots/*.dat )
readarray pointclouds < <(ls ./test-data/flower-pots/*.dat)

cmd="scripts/create-rendeing-animation.py"
# cmd="ipython3 -i scripts/create-rendeing-animation.py -- "

echo "UNUSED PARAMETERS:" \
--temporary-image-storage=/mnt/ramdisk \
--images-exist \


for pc in "${pointclouds[@]}"
do
echo "Parsing pointcloud : $pc"

${cmd} \
--step-distance=0.10 \
--min-distance=0.00 \
--max-distance=4.00 \
--distance-marker-width=0.10 \
--stereo-distance=0.35 \
--animation-fps=12 \
--pointcloud="$pc"


done

# mediainfo "$(ls ${testpath}/*.wav)"
# vlc animation.wav
# eog ./test-data/test1/animation*.png

