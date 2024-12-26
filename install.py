#!/usr/bin/env python3


from utils import run_command, logger


def main():
    logger.info("Installing headers...")
    run_command("make -C kernel install")
    run_command("make -C libc install")


if __name__ == "__main__":
    main()
