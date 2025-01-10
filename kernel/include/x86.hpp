#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

#include "multiboot.h"

/* Passed in to kernel_main() */
extern multiboot_info_t *multiboot_data;

__BEGIN_DECLS

/* Defined in arch/linker.ld */
extern uint32_t kernel_start;
extern uint32_t kernel_end;

__END_DECLS
