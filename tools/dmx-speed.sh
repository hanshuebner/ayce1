#!/bin/bash

# Measure the speed of the DMX interface by sending a specified amount of data in
# blocks over the serial interface

if [ $BASH_ARGC -ne 2 ]; then
		echo "Usage: ./dmx-speed.sh blocksize count"
		exit 1
fi

DMX_IF=/dev/cu.usbmodem3d11

# index 0 ?? i'll never get shell programming...
TOTAL=${BASH_ARGV[0]}
BLOCKSIZE=${BASH_ARGV[1]}
COUNT=$(( $TOTAL / $BLOCKSIZE ))
echo "Writing $TOTAL bytes in $COUNT packets of $BLOCKSIZE bytes to DMX interface $DMX_IF:"
echo -n "      - "
dd if=/dev/zero of=$DMX_IF bs=$BLOCKSIZE count=$COUNT 2>&1 | tail -1