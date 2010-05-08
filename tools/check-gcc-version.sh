#!/bin/sh

# Check that the toolchain version is equal to a fixed version 
# (avoid changes of the toolchain between versions and different developers)

TOOLCHAIN_VERSION=`avr-gcc -v 2>&1 | grep version`;
FIXED_TOOLCHAIN_VERSION="gcc version 4.3.2 (GCC) "

# ensure we always use the same version
if [ "$TOOLCHAIN_VERSION" != "$FIXED_TOOLCHAIN_VERSION" ]; then
		echo "$TOOLCHAIN_VERSION should be $FIXED_TOOLCHAIN_VERSION"
		exit 1
else
		exit 0
fi