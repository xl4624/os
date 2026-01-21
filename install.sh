#!/bin/sh

sysroot=$(make -s --no-print-directory -f Makefile print-sysroot)

echo "Installing headers..."
make -C kernel SYSROOT="$sysroot" install
make -C libc SYSROOT="$sysroot" install
