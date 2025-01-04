#include "keyboard.hpp"

#include <stdio.h>
#include <sys/io.h>

#include "interrupt.hpp"

#define LSHIFT 0x2A
#define RSHIFT 0x36

// TODO: https://wiki.osdev.org/PS/2_Keyboard#Command_Queue_and_State_Machine

KeyboardDriver kbd;

// scancode_to_ascii[...][1] is shifted key
// clang-format off
static const char scancode_to_ascii[58][2] = {
    {0, 0},       // (0x00)
    {0, 0},       // esc (0x01)
    {'1', '!'}, {'2', '@'}, {'3', '#'},
    {'4', '$'}, {'5', '%'}, {'6', '^'},
    {'7', '&'}, {'8', '*'}, {'9', '('},
    {'0', ')'}, {'-', '_'}, {'=', '+'},
    {'\b', '\b'}, // backspace (0x0E)
    {'\t', '\t'}, // tab (0x0F)
    {'q', 'Q'}, {'w', 'W'}, {'e', 'E'},
    {'r', 'R'}, {'t', 'T'}, {'y', 'Y'},
    {'u', 'U'}, {'i', 'I'}, {'o', 'O'},
    {'p', 'P'}, {'[', '{'}, {']', '}'},
    {'\n', '\n'}, // enter (0x1C)
    {0, 0},       // Lctrl (0x1D)
    {'a', 'A'}, {'s', 'S'}, {'d', 'D'},
    {'f', 'F'}, {'g', 'G'}, {'h', 'H'},
    {'j', 'J'}, {'k', 'K'}, {'l', 'L'},
    {';', ':'}, {'\'', '"'}, {'`', '~'},
    {0, 0},       // Lshift (0x2A)
    {'\\', '|'}, {'z', 'Z'}, {'x', 'X'},
    {'c', 'C'}, {'v', 'V'}, {'b', 'B'},
    {'n', 'N'}, {'m', 'M'}, {',', '<'},
    {'.', '>'}, {'/', '?'},
    {0, 0},       // Rshift (0x36)
    {'*', '*'},   // Keypad * (0x37)
    {0, 0},       // Lalt (0x39)
    {' ', ' '},   // space (0x3A)
};
// clang-format on

void keyboard_handler(interrupt_frame *frame) {
    (void)frame;
    uint8_t scancode = inb(0x60);
    kbd.handle_scancode(scancode);
}

KeyboardDriver::KeyboardDriver() : shift_(false) {
    interrupt_register_handler(IRQ::Keyboard, keyboard_handler);
}

void KeyboardDriver::handle_scancode(uint8_t scancode) {
    bool is_release = scancode & 0x80;
    scancode &= ~(0x80);
    if (is_release) {
        if (scancode == LSHIFT || scancode == RSHIFT) {
            shift_ = false;
        }
        return;
    }

    switch (scancode) {
        case LSHIFT:
        case RSHIFT: shift_ = true; break;

        default:
            if (scancode
                < sizeof(scancode_to_ascii) / sizeof(scancode_to_ascii[0])) {
                char c = scancode_to_ascii[scancode][shift_ ? 1 : 0];
                if (c != 0) {
                    putchar(c);
                }
            }
            break;
    }
}
