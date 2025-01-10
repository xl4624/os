import logging
import subprocess
import sys


def run_command(cmd, capture_output=False):
    """Utility to run a shell command in a subprocess."""
    result = subprocess.run(
        cmd, shell=True, capture_output=capture_output, text=capture_output
    )
    if result.returncode != 0:
        sys.exit(result.returncode)
    return result.stdout if capture_output else result


logging.basicConfig(
    level=logging.INFO,
    format="build: %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
    force=True,
)
logger = logging.getLogger(__name__)


def main():
    sysroot = run_command(
        "make -s --no-print-directory -f Makefile print-sysroot",
        capture_output=True,
    ).strip()
    logger.info("Installing headers...")
    run_command(f"make -C kernel SYSROOT={sysroot} install")
    run_command(f"make -C libc SYSROOT={sysroot} install")


if __name__ == "__main__":
    main()
