#!/usr/bin/env python3


from utils import run_command, logger


def main():
    sysroot = run_command(
        "make -s --no-print-directory -f Makefile print-sysroot"
    ).strip()
    logger.info("Installing headers...")
    run_command(f"make -C kernel SYSROOT={sysroot} install")
    run_command(f"make -C libc SYSROOT={sysroot} install")


if __name__ == "__main__":
    main()
