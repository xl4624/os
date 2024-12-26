# os

1. Setup the i868-elf Cross Compiler
    * Follow the [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler) guide
    * Should have `i686-elf-gcc`, `i686-elf-as`, etc. after
2. Install dependencies (follow OSDev guide, but here's the list I remember)
    * `xoriso`
    * `grub`
    * `qemu`
3. To setup the project you can:
    * Build OS:
    ```bash
    ./build.py
    ```

    * Run OS:
    ```bash
    ./run.py # Will build OS as well
    ```

    * Clean/delete object files, images, and binaries:
    ```bash
    ./clean.py
    ```
