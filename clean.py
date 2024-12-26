#!/usr/bin/env python3

from utils import run_command, SYSROOT, ISODIR, ISO, logger


def main():
    logger.info("Cleaning up files and directories...")
    run_command("make clean")
    run_command(f"rm -f {ISO}")
    run_command(f"rm -rf {ISODIR} {SYSROOT}")


if __name__ == "__main__":
    main()
