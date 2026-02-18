#!/bin/sh

SYSROOT=$(make -s --no-print-directory -f Makefile print-sysroot)

echo "Installing headers..."
make -C kernel SYSROOT="$SYSROOT" install
make -C libc SYSROOT="$SYSROOT" install
make -C libcpp SYSROOT="$SYSROOT" install
