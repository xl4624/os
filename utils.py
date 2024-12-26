import subprocess
import sys
import logging
import os

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


def run_command(cmd):
    """Utility to run a shell command in a subprocess."""
    result = subprocess.run(cmd, shell=True)
    if result.returncode != 0:
        sys.exit(result.returncode)


def setup_logging():
    logging.basicConfig(
        level=logging.INFO,
        format="[%(asctime)s] [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
        force=True,
    )
    return logging.getLogger(__name__)


logger = setup_logging()
