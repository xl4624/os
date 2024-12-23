#!/usr/bin/python3

import argparse
import os
import sys
import subprocess

SYSROOT = "sysroot/"

ISO = "myos.iso"
BIN = "myos.bin"
ISODIR = "isodir"
GRUBCFGPATH = os.path.join(ISODIR, "boot", "grub", "grub.cfg")
GRUBCFG = \
"""menuentry "myos" {
    multiboot /boot/myos.bin
    boot
}
"""


def run_command(cmd):
    """Utility to run a shell command in a subprocess."""
    result = subprocess.run(cmd, shell=True)
    if result.returncode != 0:
        sys.exit(result.returncode)


def build():
    install()
    run_command("make")
    create_iso()


def install():
    run_command("make -C kernel install")
    run_command("make -C libc install")


def run():
    build()
    run_command(f"qemu-system-i386 -cdrom {ISO}")


def clean():
    run_command("make clean")
    run_command(f"rm -f {ISO}")
    run_command(f"rm -rf {ISODIR} {SYSROOT}")


def check():
    run_command("make check")


def create_iso():
    run_command(f"mkdir -p {ISODIR}/boot/grub")
    run_command(f"cp {BIN} {ISODIR}/boot/{BIN}")
    with open(GRUBCFGPATH, "w") as file:
        file.write(GRUBCFG)
    run_command(f"grub-mkrescue -o {ISO} {ISODIR}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "command",
        nargs="?",
        default="build",
        choices=["build", "run", "clean", "check", "install"],
        help="OPTIONS: build, run, clean, check, install",
    )
    args = parser.parse_args()

    match args.command.lower():
        case "build":
            build()
        case "run":
            run()
        case "clean":
            clean()
        case "check":
            check()
        case "install":
            install()


if __name__ == "__main__":
    main()
