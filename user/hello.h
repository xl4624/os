#pragma once

/*
 * Hand-encoded i686 machine code for user/hello.S.
 *
 * This is a temporary approach: the user program is compiled by hand into
 * raw bytes and embedded directly in the kernel image.
 *
 * TODO: Replace with a proper build step that assembles user/hello.S into a
 * flat binary and links it into the kernel via objcopy (i686-elf-objcopy -O
 * binary / --add-section).
 */

#include <stdint.h>

#include "paging.h"

// Virtual addresses for the user program and stack.
// TODO: These should come from an ELF loader or process setup code once proper
// process management is implemented.
static constexpr vaddr_t USER_CODE_VA = 0x00800000;
static constexpr vaddr_t USER_STACK_VA = 0x00BFC000;
static constexpr uint32_t USER_STACK_PAGES = 4;
static constexpr vaddr_t USER_STACK_TOP =
    USER_STACK_VA + USER_STACK_PAGES * PAGE_SIZE;

static const char kUserMessage[] = "Hello from userspace!\n";
static constexpr uint32_t kUserMessageLen = sizeof(kUserMessage) - 1;
static constexpr uint32_t kUserCodeLen =
    31;  // bytes of instructions before msg
static constexpr vaddr_t kUserMessageVA = USER_CODE_VA + kUserCodeLen;

// clang-format off
static const uint8_t kUserProgram[] = {
    // mov $SYS_WRITE(2), %eax
    0xB8, 0x02, 0x00, 0x00, 0x00,
    // mov $1, %ebx                (fd = stdout)
    0xBB, 0x01, 0x00, 0x00, 0x00,
    // mov $kUserMessageVA, %ecx   (buf)
    0xB9,
    static_cast<uint8_t>((kUserMessageVA >>  0) & 0xFF),
    static_cast<uint8_t>((kUserMessageVA >>  8) & 0xFF),
    static_cast<uint8_t>((kUserMessageVA >> 16) & 0xFF),
    static_cast<uint8_t>((kUserMessageVA >> 24) & 0xFF),
    // mov $kUserMessageLen, %edx  (count)
    0xBA,
    static_cast<uint8_t>((kUserMessageLen >>  0) & 0xFF),
    static_cast<uint8_t>((kUserMessageLen >>  8) & 0xFF),
    static_cast<uint8_t>((kUserMessageLen >> 16) & 0xFF),
    static_cast<uint8_t>((kUserMessageLen >> 24) & 0xFF),
    // int $0x80
    0xCD, 0x80,
    // mov $SYS_EXIT(0), %eax
    0xB8, 0x00, 0x00, 0x00, 0x00,
    // xor %ebx, %ebx              (exit code 0)
    0x31, 0xDB,
    // int $0x80
    0xCD, 0x80,
};
// clang-format on

static_assert(sizeof(kUserProgram) == kUserCodeLen,
              "kUserCodeLen must match sizeof(kUserProgram)");
static_assert(kUserCodeLen + kUserMessageLen <= PAGE_SIZE,
              "User code + message must fit within a single mapped page");
