# os

1. Setup the i868-elf Cross Compiler
    * Follow the [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler) guide
    * Should have `i686-elf-gcc`, `i686-elf-as`, etc. after
2. Install dependencies (follow OSDev guide, but here's the list I remember)
    * `xoriso`
    * `grub`
    * `qemu`
3. To setup the project you can:
    * Build kernel binary and bootable ISO:
    ```bash
    make # all
    ```

    * Run OS (with QEMU):
    ```bash
    make run # builds OS as well
    ```

    * Clean/delete object files, images, and binaries:
    ```bash
    make clean
    ```

    * Install libc/kernel headers (by default in `sysroot/`), gets run during builds
    ```bash
    make install
    ```

# Notes

For debugging purposes, `make run` automatically attaches the QEMU monitor to stdio.
This allows you to control the emulator (pause/resume VM, inspect registers, etc.).
Check out the docs at: https://qemu-project.gitlab.io/qemu/system/monitor.html.

To debug, you can run `make debug` to attach a debugger and a QEMU monitor to stdio,
which allows you to control the emulator (pause/resume VM, inspect registers, etc.).
You run `gdb` and type:

```gdb
(gdb) file myos.bin
(gdb) target remote localhost:1234
(gdb) continue
```
