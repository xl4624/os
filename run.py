#!/usr/bin/env python3


from utils import run_command, ISO, logger


def main():
    run_command("./build.py")
    logger.info("Running OS...")
    run_command(f"qemu-system-i386 -cdrom {ISO}")


if __name__ == "__main__":
    main()
