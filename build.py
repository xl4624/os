#!/usr/bin/env python3

import os
from utils import run_command, logger

SYSROOT = "sysroot/"

ISO = "myos.iso"
BIN = "myos.bin"
ISODIR = "isodir"
GRUBCFGPATH = os.path.join(ISODIR, "boot", "grub", "grub.cfg")
GRUBCFG = """menuentry "myos" {
    multiboot /boot/myos.bin
    boot
}
"""


def main():
    run_command("./install.py")
    logger.info(" Building...")
    run_command("make")
    create_iso()


def create_iso():
    logger.info("Creating ISO image...")
    run_command(f"mkdir -p {ISODIR}/boot/grub")
    run_command(f"cp {BIN} {ISODIR}/boot/{BIN}")
    with open(GRUBCFGPATH, "w") as file:
        file.write(GRUBCFG)
    run_command(f"grub-mkrescue -o {ISO} {ISODIR}")


if __name__ == "__main__":
    main()
