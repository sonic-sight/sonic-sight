#!/usr/bin/env python3

import subprocess

import argparse

parser = argparse.ArgumentParser(
    description="Scritp to concatenate webm videos together with re-encoding",
    usage='use "%(prog)s --help" for more information',
)

parser.add_argument(
    "--output",
    type=str,
    required=True,
    help="Output file",
)

parser.add_argument(
    "inputs",
    type=str,
    nargs='+',
    help="List of input videos to concatenate",
)

if __name__ == "__main__":
    args = parser.parse_args()
    cmd="ffmpeg -i " + (" -i ".join( args.inputs )) +\
       " -filter_complex " + "".join( [ "[{n}:v:0][{n}:a:0]".format(n=i) for i in range(len(args.inputs)) ] ) +\
        "concat=n={n}:v=1:a=1[outv][outa]".format(n=len(args.inputs)) +\
        " -map [outv] -map [outa] -c:v libvpx-vp9 -crf 31 -b:v 0 -c:a libopus " + args.output
    print("Command to run:")
    print(cmd.split(" "))
    # print(cmd)
    # defaultcmd='''ffmpeg -i input1.webm -i input2.webm -i input3.webm -filter_complex "[0:v:0][0:a:0][1:v:0][1:a:0][2:v:0][2:a:0]concat=n=3:v=1:a=1[outv][outa]" -map "[outv]" -map "[outa]"  -s 1200x700 -c:v libvpx-vp9 -crf 31 -b:v 0 -c:a libopus testout.webm'''
    # print(defaultcmd)
    subprocess.run( cmd.split(" ") )