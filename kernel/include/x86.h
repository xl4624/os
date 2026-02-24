#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

#include "multiboot.h"

__BEGIN_DECLS

extern uint32_t mboot_magic;
extern multiboot_info_t* mboot_info;

/* Defined in arch/linker.ld */
extern uint32_t kernel_start;
extern uint32_t kernel_end;

__END_DECLS
