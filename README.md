# os

1. Setup the i868-elf Cross Compiler
    * Follow the [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler) guide
    * Should have `i686-elf-gcc`, `i686-elf-as`, etc. after
2. Install dependencies (follow OSDev guide, but here's the list I remember)
    * `xoriso`
    * `grub`
    * `qemu`
3. Run `chmod +x setup.py` and now you can:
    * Build OS:
    ```bash
    ./setup.py build
    ```
    * Run OS:
    ```bash
    ./setup.py run # Will build OS as well
    ```
