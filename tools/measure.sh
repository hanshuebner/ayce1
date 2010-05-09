#!/bin/bash

COUNT=$((1024 * 1024))

echo "* git revision" `git rev-parse HEAD`
echo
#for bs in 2 4 8 16 32 64; do
for bs in 64; do
		./dmx-speed.sh $bs $COUNT
		sleep 2
done
echo
echo
