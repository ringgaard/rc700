#!/bin/bash

FLUX=$1
IMD=$2

rm /tmp/disk*

# Convert KryoFlux stream file to RC format:
# -l15 (output level: device, read, cell, format
# -f/tmp/disk.raw (output file)
# -s0 -e70 (output track 0-70)
# -g2 (both sides)
# -z0 (128 bytes per sector)
# -n+16 (exactly 16 sectors per track)
# -k2 (40 tracks)
# -i3 (image type 3: FM sector image, 40/80+ tracks, SS/DS, SD/DD, 300, FM)
# -m1 -f$FLUX/track (read from image file)
# -i0 -e83 (read track all tracks 0-83)

dtc -l15 -f/tmp/disk.raw -s0 -e70 -g2 -z0 -n+16 -k2 -i3 -m1 -f$FLUX/track -i0 -e83

# Convert raw images of side 0 and 1 to IMD format.
./raw2imd rc $IMD /tmp/disk_s0.raw /tmp/disk_s1.raw

