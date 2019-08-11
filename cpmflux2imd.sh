#!/bin/bash

FLUX=$1
IMD=$2

rm /tmp/disk*

# Convert KryoFlux stream file to CPM format:
# -l15 (output level: device, read, cell, format
# -f/tmp/disk.raw (output file)
# -s0 -e70 (output track 0-70)
# -g2 (both sides)
# -z2 (512 bytes per sector)
# -n+9 (exactly 9 sectors per track)
# -k2 (40 tracks)
# -i4 (image type 4: MFM sector image, 40/80+ tracks, SS/DS, DD/HD, 300, MFM)
# -m1 -f$FLUX/track (read from image file)
# -i0 -e83 (read track all tracks 0-83)

dtc -l15 -f/tmp/disk.raw -s0 -e70 -g2 -z2 -n+9 -k2 -i4 -m1 -f$FLUX/track -i0 -e83

# Read track 0 side 0:
# -s0 -e0 (only read track 0)
# -g0 (only read side 0)
# -z0 (128 bytes per sector)
# -n+16 (exactly 16 sectors per track)
# -i3 (image type 3: FM sector image, 40/80+ tracks, SS/DS, SD/DD, 300, FM)

dtc -l15 -f/tmp/disk_t0.raw -s0 -e0 -g0 -z0 -n+16 -k2 -i3 -m1 -f$FLUX/track -i0 -e83

# Read track 0 side 1:
# -s0 -e0 (only read track 0)
# -g0 (only read side 0)
# -z1 (256 bytes per sector)
# -n+16 (exactly 16 sectors per track)
# -i4 (image type 4: MFM sector image, 40/80+ tracks, SS/DS, DD/HD, 300, MFM)

dtc -l15 -f/tmp/disk_t0.raw -s0 -e0 -g1 -z1 -n+16 -k2 -i4 -m1 -f$FLUX/track -i0 -e83

# Convert raw images of side 0 and 1 to IMD format.
./raw2imd cpm $IMD /tmp/disk_s0.raw /tmp/disk_s1.raw /tmp/disk_t0_s0.raw /tmp/disk_t0_s1.raw

