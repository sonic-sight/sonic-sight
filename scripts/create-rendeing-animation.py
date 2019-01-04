#!/usr/bin/env python3

# Requirement: have ffmpeg with vpx support. Verify with:
# ffmpeg -codecs | grep vpx
# ...
# DEV.L. vp8                  On2 VP8 (decoders: vp8 vp8_v4l2m2m libvpx ) (encoders: libvpx vp8_v4l2m2m vp8_vaapi )
# DEV.L. vp9                  Google VP9 (decoders: vp9 libvpx-vp9 ) (encoders: libvpx-vp9 vp9_vaapi )

import matplotlib.pyplot as plt
from matplotlib import image as mpimg
from matplotlib.collections import PatchCollection
from matplotlib.patches import Rectangle
import matplotlib.patches as mpatches
import argparse
import numpy as np
from ipdb import set_trace
import subprocess
import os
from scipy.io.wavfile import write as write_wavefile
import sys

parser = argparse.ArgumentParser(
    description="Scritp to create an animation of the rending of the depth to sound",
    usage='use "%(prog)s --help" for more information',
)

parser.add_argument(
    "--stereo-distance",
    type=float,
    default=0.35,
    help="Distance between the virtual ears",
)

parser.add_argument(
    "--speed-of-sound",
    type=float,
    default=1.,
    help="Speed of sound",
)

parser.add_argument(
    "--min-distance",
    type=float,
    default=0.2,
    help="Minimum distance",
)

parser.add_argument(
    "--max-distance",
    type=float,
    default=1.5,
    help="Maximum distance",
)

parser.add_argument(
    "--step-distance",
    type=float,
    default=0.025,
    help="Rendering step for distance",
)

parser.add_argument(
    "--distance-marker-width",
    type=float,
    default=0.025,
    help="Width of the marker region in distance",
)

parser.add_argument(
    "--animation-fps",
    type=int,
    default=15,
    help="Frames per second for the animation",
)

parser.add_argument(
    "--base-frequency",
    type=float,
    default=1000.,
    help="Base amplitudes-rendering frequency",
)

parser.add_argument(
    "--start-frequency",
    type=float,
    default=500.,
    help="Start sound frequency",
)

parser.add_argument(
    "--start-duration",
    type=float,
    default=0.05,
    help="Start sound duration [s]",
)

parser.add_argument(
    "--start-amplitude",
    type=float,
    default=0.05,
    help="Start sound amplitude [0-1.)",
)

parser.add_argument(
    "--sound-sample-rate",
    type=int,
    default=44100,
    help="Sample rate for the generated sound",
)

parser.add_argument(
    "--show-depth",
    type=float,
    default=None,
    help="If set, only image at this depth is shown interactively",
)

parser.add_argument(
    "--max-relative-amplitude",
    type=float,
    default=0.3,
    help="Maximal relative amplitude on the graph of amplitudes, if set to negative value range is auto-adjusted",
)

parser.add_argument(
    "--video-file-type",
    type=str,
    default="webm",
    choices=["webm","mp4",],
    help="Choose the video file type",
)

parser.add_argument(
    "--temporary-image-storage",
    type=str,
    default="",
    help="Temporay image storage folder, can be set to ramdisk." \
         "If unset, images are in the same folder as the pointcloud, and are not removed after finishing." \
         "If set, images are in specified folder and are removed after animation is created."
)

parser.add_argument(
    "--pointcloud",
    type=str,
    required=True,
    help="Pointcloud data file",
)

parser.add_argument(
    "--loudness",
    type=str,
    default=None,
    help="Loudness data file, if unset derived from pointcloud file name",
)

parser.add_argument(
    "--amplitudes",
    type=str,
    default=None,
    help="Amplitudes data file, if unset derived from pointcloud file name",
)

parser.add_argument(
    "--color-image",
    type=str,
    default=None,
    help="Color image data file (png), if unset derived from pointcloud file name",
)

parser.add_argument(
    "--info-file",
    type=str,
    default=None,
    help="File with parameters of the rendering app, if unset derived from pointcloud file name",
)

parser.add_argument(
    "--images-exist",
    default=False,
    action="store_true",
    help="If set, images are presumed to already exist",
)

def read_parameters_file( filename ):
    d = {}
    with open( filename, "rt" ) as f:
        for line in f.readlines():
            values = line.strip().split("=")
            if len(values) == 2:
                d[values[0].strip()] = values[1].strip()
    return d

def make_sound_file(
        filename,
        amplitudes, max_distance_amplitudes,
        sample_rate,
        amplitudes_time_step,
        min_distance, max_distance,
        base_frequency,
        start_frequency,
        start_duration,
        start_amplitude
):
    sample = np.zeros( (int(amplitudes.shape[0]*amplitudes_time_step*sample_rate),2), dtype=np.float32 )
    t = np.arange(0,sample.shape[0])/sample_rate
    for i in range(amplitudes.shape[0]):
        time_start = amplitudes_time_step * i
        time_end = amplitudes_time_step*(i+1)
        i_start = int(time_start*sample_rate)
        i_end = int(time_end*sample_rate)
        for j in [0,1]:
            sample[i_start:i_end,j] = amplitudes[i,j] * \
                  np.sin( (2*np.pi)*base_frequency * \
                  t[i_start:i_end] )
    # Add the start sound
    n_start = int(start_duration * sample_rate)
    for i in [0,1]:
        sample[0:n_start,i] = start_amplitude * \
            np.sin( (2*np.pi)*start_frequency * \
            np.arange(0,n_start)/sample_rate )
    # Trim the sound to the sound matching the animated frames
    i_start = int( amplitudes.shape[0] / max_distance_amplitudes * min_distance *\
        amplitudes_time_step * sample_rate )
    i_end   = int( amplitudes.shape[0] / max_distance_amplitudes * max_distance *\
        amplitudes_time_step * sample_rate ) + 1
    i_end = min( i_end, int(amplitudes.shape[0]*amplitudes_time_step*sample_rate) )
    sample = sample[i_start:i_end,:]
    # now save the waveform to .wave file in fname
    write_wavefile( filename, sample_rate, sample )

if __name__ == "__main__":
    import matplotlib
    # matplotlib.use("Agg")
    # matplotlib.rc('image', cmap="gnuplot2")
    matplotlib.rc('image', cmap="cubehelix")

    args = parser.parse_args()

    args.pointcloud = args.pointcloud.strip()

    def mdrfn( set_fn, extension ):
        if set_fn is not None:
            return set_fn
        else:
            return args.pointcloud.replace(".dat",extension)

    color_image = mpimg.imread( mdrfn( args.color_image, "_aligned.png" ) )
    loudness = np.fromfile( mdrfn( args.loudness, ".loudness" ),
        dtype=np.float32 ).reshape( ( -1, 2 ) )
    amplitudes = np.fromfile( mdrfn( args.amplitudes, ".amp" ),
        dtype=np.float32 ).reshape( ( -1, 2 ) )
    points = np.fromfile( args.pointcloud,
        dtype=np.float32 ).reshape( color_image.shape[0:2] + (3,) )
    parameters = read_parameters_file( mdrfn( args.info_file, ".inf" ) )
    del mdrfn

    if args.max_distance > float(parameters["max_distance"]):
        args.max_distance = float(parameters["max_distance"])
        print("Max distance exceed the distance in parameters file, setting it from parameters file")

    # RGBA tuples of marker colors
    left_color = np.array( [ 1., 0., 0., 1., ] )
    # right_color = np.array( [ 0., 1., 0., 1., ] )
    right_color = np.array( [ 0., 0., 1., 1., ] )


    too_close_marker = 100000
    # too_close_marker = float("nan")
    points[:,:,2] = np.where( points[:,:,2] < 0.01, too_close_marker, points[:,:,2] )

    depth = points[:,:,2]
    marker = np.ones_like(depth)


    eye_l = np.array( [ -args.stereo_distance/2, 0, 0 ] )
    eye_r = np.array( [ +args.stereo_distance/2, 0, 0 ] )
    cam_loc = np.array( [ 0., 0., 0. ] )
    locations = [ eye_l, cam_loc, eye_r ]
    print("Eyes: ", eye_l, eye_r)

    differences = [ points - loc for loc in locations ]

    lengths = [ np.sqrt( np.einsum("...k,...k->...", ddd, ddd ) )
                for ddd in differences ]

    colors_l = np.empty( depth.shape + (4,), dtype=np.float32 )
    colors_r = np.empty_like(colors_l)
    colors_combined = np.empty_like(colors_l)
    colors_l[:,:,0:3] = left_color[0:3]
    colors_r[:,:,0:3] = right_color[0:3]

    delta = 0.05
    distance = 0.85

    left_mask = np.zeros_like(depth)
    left_mask[1::2,::2] = 1.
    left_mask[::2,1::2] = 1

    right_mask = np.zeros_like(depth)
    right_mask[1::2,1::2] = 1.
    right_mask[::2,::2] = 1

    def show_depth_frame( distance, delta, title=None ):
        global mask_l, mask_r, fig, ax
        # left_region = np.where( np.abs(depth - distance) < delta, 1.0, 0.0 )
        dl = np.abs(lengths[0] - distance)
        dr = np.abs(lengths[2] - distance)
        left_region = np.where( dl < delta, np.exp(-2*(dl**2)/delta**2), 0.0 )
        right_region = np.where( dr < delta, np.exp(-2*(dl**2)/delta**2), 0.0 )
        # left_region = np.where( np.abs(lengths[0] - distance) < delta, 1.0, 0.0 )
        # right_region = np.where( np.abs(lengths[2] - distance) < delta, 1.0, 0.0 )

        # TODO: verify the masks are actually better
        colors_l[:,:,3] = left_region * left_mask
        colors_r[:,:,3] = right_region * right_mask

        # set_trace()
        # sm = left_region.shape + (1,)
        # sc = (1,1,3)
        # colors_combined[:,:,0:3] = \
        #     left_region.reshape(sm) * left_color[0:3].reshape(sc) \
        #     + right_region.reshape(sm) * right_color[0:3].reshape(sc)
        # colors_combined[:,:,3] = np.clip( left_region + right_region, a_min=0., a_max=1. )

        # fig, ax = plt.subplots( figsize=(6.5,5), dpi=150  )

        fig, axesarray = plt.subplots( nrows=2, ncols=1,
                figsize=(6.5,5.),
                gridspec_kw = {
                    'wspace':0.025,
                    'hspace':0.05,
                    'height_ratios':[4.,1.],
                }
        )
        ax = axesarray[0]
        ax.set_clip_on(False)
        ax.xaxis.set_ticks([])
        ax.yaxis.set_ticks([])
        ax.set_xticklabels([])
        ax.set_yticklabels([])
        # im_depth = ax.imshow( depth, vmin=0., vmax=args.max_distance*1.1 )
        # fig.colorbar(im_depth)
        ax.imshow( color_image )
        # im_depth = ax.imshow( lengths[2], vmin=0., vmax=args.max_distance*1.1 )
        # im_depth = ax.imshow( dr, vmin=0., vmax=args.max_distance*1.1 )
        mask_l = ax.imshow( colors_l )
        mask_r = ax.imshow( colors_r )
        # mask_combined = ax.imshow( colors_combined )

        # Add color markers to the sides of the image
        l = ax.get_xlim()
        d = l[1]-l[0]
        avg_h = np.average( ax.get_ylim() )
        ax.set_xlim( [ l[0]-0.1*d, l[1]+0.1*d ] )
        ax.set_axis_bgcolor( [ 0., 0., 0., 0., ] )
        ax.set_axis_off()

        # rect_h, rect_w = 0.1*color_image.shape[0], 0.1*color_image.shape[1]
        rect_h, rect_w = 0.1*color_image.shape[0], 0.1*color_image.shape[1]
        ax.add_collection( PatchCollection( [
            mpatches.Rectangle( xy=( -0.1*d, avg_h-rect_h/2 ), width=rect_w/2, height=rect_h, clip_on=False ),
            # mpatches.FancyBboxPatch( xy=( -0.1*d, avg_h-rect_h/2 ), width=rect_w/2, height=rect_h, clip_on=False,
            # boxstyle=mpatches.BoxStyle("Round", pad=0.3) ),
            ], facecolor=left_color, alpha=1.0 ) )
        ax.text( -0.075*d, avg_h, "L",
            horizontalalignment='center',
            verticalalignment='center',
            )
        ax.add_collection( PatchCollection( [
            mpatches.Rectangle( xy=( l[1]+0.05*d, avg_h-rect_h/2 ), width=rect_w/2, height=rect_h, clip_on=False ),
        ], facecolor=right_color, alpha=1.0 ) )
        ax.text( l[1]+0.075*d, avg_h, "R",
                 horizontalalignment='center',
                 verticalalignment='center',
                 )

        # Add loudness to plot
        ax = axesarray[1]
        xx = (np.arange(0.,loudness.shape[0])+0.5) * float(parameters["step_distance"])
        ax.plot( xx, loudness[:,0], label="L", color=tuple(left_color) )
        ax.plot( xx, loudness[:,1], label="R", color=tuple(right_color) )
        ax.plot( [distance,distance,], [0.,np.max(loudness)*1.1], color="black" )
        ax.set_xlim( [ 0., float(parameters["max_distance"]) ])
        if args.max_relative_amplitude > 0.:
            ax.set_ylim( [ 0., args.max_relative_amplitude ] )
            ax.yaxis.set_ticks(np.arange(0.0,args.max_relative_amplitude-0.001,0.10))
        ax.set_xlabel("distance [m]")
        ax.set_ylabel("relative amplitude")

        if title is None:
            title="distance = {: 6.2f}".format(distance)
        plt.suptitle(title)
        # plt.tight_layout()

    if args.show_depth is not None:
        show_depth_frame( distance=args.show_depth, delta=args.distance_marker_width, title="Test" )
        plt.show()
        plt.close()
    else:
        pointcloud_folder, pointcloud_file = os.path.split( args.pointcloud )
        f = pointcloud_file.replace(".dat","") + "_anim_"
        if len(args.temporary_image_storage) == 0:
            working_folder = pointcloud_folder
        else:
            working_folder = args.temporary_image_storage
        fname_sound         = os.path.join( pointcloud_folder, f + "sound.wav" )
        fname_video         = os.path.join( pointcloud_folder, f + "vid." + args.video_file_type )
        fname_glob          = os.path.join(    working_folder, f + "img_*.png" )
        fname_image_pattern = os.path.join(    working_folder, f + "img_{:09.4f}.png" )

        make_sound_file(
            filename=fname_sound,
            amplitudes=amplitudes, max_distance_amplitudes=float(parameters["max_distance"]),
            sample_rate=args.sound_sample_rate,
            amplitudes_time_step=(float(parameters["max_distance"])/amplitudes.shape[0]) / (args.step_distance*args.animation_fps),# * (args.max_distance-args.min_distance) / args.step_distance,
            min_distance=args.min_distance, max_distance=args.max_distance,
            base_frequency=args.base_frequency,
            start_frequency=args.start_frequency,
            start_duration=args.start_duration,
            start_amplitude=args.start_amplitude
            )

        image_files = []
        if args.images_exist == False:
            for distance in np.arange(args.min_distance, args.max_distance+args.step_distance/2., args.step_distance):
                title="distance = {: 6.2f}".format(distance)
                print("Plotting image ", title )
                fname=fname_image_pattern.format(distance)
                show_depth_frame( distance=distance, delta=args.distance_marker_width, title=title )
                plt.savefig(fname)
                plt.close()
                image_files.append(fname)

        if args.video_file_type == "mp4":
            cmd="ffmpeg -y -pattern_type glob -framerate {fps}/1 -i {glob} -i {sound} -c:v libx264 -r 30 -pix_fmt yuv420p -c:a libvorbis {video}".format(
               glob=fname_glob, fps=args.animation_fps, video=fname_video, sound=fname_sound,
            ).split(" ")
            subprocess.run(cmd)
        else:
            cmd="ffmpeg -y -pattern_type glob -framerate {fps}/1 -i {glob} -i {sound} -c:v libvpx-vp9 -crf 31 -b:v 0 -c:a libopus {video}".format(
                glob=fname_glob, fps=args.animation_fps, video=fname_video, sound=fname_sound,
            ).split(" ")
            subprocess.run(cmd)
        if len(args.temporary_image_storage) > 0:
            for fname in image_files:
                os.remove(fname)
        # lossless webm conversion from existing video
        # cmd="ffmpeg -y -i {combined} -c:v libvpx-vp9 -lossless 1 {webvideo}".format(
        #     combined=fname_combined, webvideo=fname_webm ).split(" ")
        # subprocess.run(cmd)
        # Combining videos using ffmpeg:
        # https://trac.ffmpeg.org/wiki/Concatenate#samecodec
        # ffmpeg -f concat -safe 0 -i <(for f in ./*anim_vid.webm; do echo "file '$PWD/$f'"; done) -c copy allffmpeg.webm
        # or using mkvmerge:
        # mkvmerge -o allmkvmerge.webm -w $( ls *_vid.webm | sed -e '2,${s/^/+/g}' )
        # Note: actually the videos that are concatenated as above
        # produce an error in firefox, therefore I am re-encoding the
        # videos with ffmpeg:
        # ffmpeg -i input1.webm -i input2.webm -i input3.webm -filter_complex "[0:v:0][0:a:0][1:v:0][1:a:0][2:v:0][2:a:0]concat=n=3:v=1:a=1[outv][outa]" -map "[outv]" -map "[outa]"  -s 1200x700 -c:v libvpx-vp9 -crf 31 -b:v 0 -c:a libopus  output.webm


