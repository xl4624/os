#pragma once

#include <stddef.h>
#include <stdint.h>

class KeyboardDriver {
   public:
    KeyboardDriver();
    void handle_scancode(uint8_t scancode);

   private:
    uint8_t buffer_[256];
    bool shift_;
};

extern KeyboardDriver keyboard;
