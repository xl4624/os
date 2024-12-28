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


def run_command(cmd, capture_output=False):
    """Utility to run a shell command in a subprocess."""
    result = subprocess.run(cmd, shell=True, capture_output=capture_output, text=capture_output)
    if result.returncode != 0:
        sys.exit(result.returncode)
    return result.stdout if capture_output else result


def setup_logging():
    logging.basicConfig(
        level=logging.INFO,
        format="build: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
        force=True,
    )
    return logging.getLogger(__name__)


logger = setup_logging()
