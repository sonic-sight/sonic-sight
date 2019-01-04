#!/usr/bin/env python3

import subprocess
import re
import argparse

parser = argparse.ArgumentParser(
    description="Scritp to concatenate webm videos together with re-encoding",
    usage='use "%(prog)s --help" for more information',
)

parser.add_argument(
    "--input",
    type=str,
    required=True,
    help="Input video file",
)

parser.add_argument(
    "--time",
    type=float,
    required=True,
    help="Time offset where the frame is extracted [s]",
)

if __name__ == "__main__":
    args = parser.parse_args()
    output = re.sub( r'\.[^.]*', '', args.input ) + ".jpg"
    cmd="ffmpeg -i {input} -vf select=between(t\,{timestart}\,{timeend}) -vframes 1 {output}".format(
        input=args.input, timestart=args.time, timeend=args.time+1.0, output=output,
    )
    subprocess.run( cmd.split(" ") )

