#pragma once

#include <stdint.h>

namespace Usermode {

// Transition from ring 0 to ring 3 using the iret trick.
// `entry` is the virtual address of the user-mode entry point.
// `user_stack_top` is the top of the user-mode stack.
// This function does not return.
[[noreturn]]
void jump(uint32_t entry, uint32_t user_stack_top);

}  // namespace Usermode
