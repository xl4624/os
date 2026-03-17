# os

1. Setup the i868-elf Cross Compiler
    * Follow the [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler) guide
    * Should have `i686-elf-gcc`, `i686-elf-as`, etc. after
2. Install dependencies:
    * `i686-elf` cross-compiler toolchain (gcc, g++, as, ld, ar)
    * `xorriso` (for ISO creation)
    * `grub` (grub-file, grub-mkrescue)
    * `qemu-system-i386` (for running/debugging)
    * `lldb` (optional, for debugging with `make lldb`)
    * `gcc`/`g++` (host compilers for unit tests)
    * `make`
    * `dosfstools`
    * `mtools`
3. To setup the project you can:
    * Build kernel binary and bootable ISO:
    ```bash
    make # all
    ```

    * Run OS (with QEMU):
    ```bash
    make run # builds OS as well
    ```

    * Run host-compiled unit tests:
    ```bash
    make test
    ```

    * Run kernel tests in QEMU (output in `build/ktest/ktest.log`):
    ```bash
    make ktest
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

To debug, you can run `make lldb` to launch QEMU with a GDB stub and attach LLDB:

```gdb
(lldb) file myos.bin
(lldb) gdb-remote localhost:1234
(lldb) continue
```

