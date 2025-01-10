#include "keyboard.hpp"

#include <sys/io.h>

#include "interrupt.hpp"
#include "tty.hpp"

// TODO: https://wiki.osdev.org/PS/2_Keyboard#Command_Queue_and_State_Machine

KeyboardDriver keyboard;

void keyboard_handler(interrupt_frame *frame) {
    (void)frame;
    uint8_t scancode = inb(0x60);
    keyboard.process_scancode(scancode);
}

KeyboardDriver::KeyboardDriver() {
    interrupt_register_handler(IRQ::Keyboard, keyboard_handler);
}

void KeyboardDriver::process_scancode(uint8_t scancode) {
    if (scancode == 0xE0) {
        extended_scancode_ = true;
        return;
    }

    KeyEvent event;

    // Check if a key is pressed
    event.pressed = !(scancode & 0x80);
    scancode &= ~(0x80);

    // Get the Key enum from lookup table
    if (extended_scancode_) {
        event.key = lookup_extended(scancode);
        extended_scancode_ = false;
    } else if (scancode < SCANCODE_TABLE_SIZE) {
        event.key = scancode_to_key[scancode];
    } else {
        event.key = Key::Unknown;
    }

    switch (event.key) {
        case Key::LeftShift:
        case Key::RightShift: shift_ = event.pressed; break;
        case Key::LeftCtrl:
        case Key::RightCtrl: ctrl_ = event.pressed; break;
        case Key::LeftAlt:
        case Key::RightAlt: alt_ = event.pressed; break;
        default: break;
    }

    event.shift = shift_;
    event.ctrl = ctrl_;
    event.alt = alt_;

    if (event.pressed) {
        event.ascii = lookup_ascii(event.key);
    }

    terminal.handle_key(event);
}

Key KeyboardDriver::lookup_extended(uint8_t scancode) {
    switch (scancode) {
        case 0x48: return Key::Up;
        case 0x50: return Key::Down;
        case 0x4B: return Key::Left;
        case 0x4D: return Key::Right;
        default: return Key::Unknown;
    }
}

char KeyboardDriver::lookup_ascii(Key key) const {
    // ctrl/alt combinations don't produce ascii
    if (ctrl_ || alt_) {
        return 0;
    }

    switch (key) {
        case Key::A: return shift_ ? 'A' : 'a';
        case Key::B: return shift_ ? 'B' : 'b';
        case Key::C: return shift_ ? 'C' : 'c';
        case Key::D: return shift_ ? 'D' : 'd';
        case Key::E: return shift_ ? 'E' : 'e';
        case Key::F: return shift_ ? 'F' : 'f';
        case Key::G: return shift_ ? 'G' : 'g';
        case Key::H: return shift_ ? 'H' : 'h';
        case Key::I: return shift_ ? 'I' : 'i';
        case Key::J: return shift_ ? 'J' : 'j';
        case Key::K: return shift_ ? 'K' : 'k';
        case Key::L: return shift_ ? 'L' : 'l';
        case Key::M: return shift_ ? 'M' : 'm';
        case Key::N: return shift_ ? 'N' : 'n';
        case Key::O: return shift_ ? 'O' : 'o';
        case Key::P: return shift_ ? 'P' : 'p';
        case Key::Q: return shift_ ? 'Q' : 'q';
        case Key::R: return shift_ ? 'R' : 'r';
        case Key::S: return shift_ ? 'S' : 's';
        case Key::T: return shift_ ? 'T' : 't';
        case Key::U: return shift_ ? 'U' : 'u';
        case Key::V: return shift_ ? 'V' : 'v';
        case Key::W: return shift_ ? 'W' : 'w';
        case Key::X: return shift_ ? 'X' : 'x';
        case Key::Y: return shift_ ? 'Y' : 'y';
        case Key::Z: return shift_ ? 'Z' : 'z';

        // Numbers/symbols
        case Key::Num1: return shift_ ? '!' : '1';
        case Key::Num2: return shift_ ? '@' : '2';
        case Key::Num3: return shift_ ? '#' : '3';
        case Key::Num4: return shift_ ? '$' : '4';
        case Key::Num5: return shift_ ? '%' : '5';
        case Key::Num6: return shift_ ? '^' : '6';
        case Key::Num7: return shift_ ? '&' : '7';
        case Key::Num8: return shift_ ? '*' : '8';
        case Key::Num9: return shift_ ? '(' : '9';
        case Key::Num0: return shift_ ? ')' : '0';

        // Special characters
        case Key::Space: return ' ';
        case Key::Enter: return '\n';
        case Key::Tab: return '\t';
        case Key::Backspace: return '\b';

        case Key::Minus: return shift_ ? '_' : '-';
        case Key::Equals: return shift_ ? '+' : '=';
        case Key::LeftBracket: return shift_ ? '{' : '[';
        case Key::RightBracket: return shift_ ? '}' : ']';
        case Key::Backslash: return shift_ ? '|' : '\\';
        case Key::Semicolon: return shift_ ? ':' : ';';
        case Key::Quote: return shift_ ? '"' : '\'';
        case Key::Backtick: return shift_ ? '~' : '`';
        case Key::Comma: return shift_ ? '<' : ',';
        case Key::Period: return shift_ ? '>' : '.';
        case Key::Slash: return shift_ ? '?' : '/';

        default: return 0;  // Non-printable or unknown key
    }
}

// NOTE: Keep SCANCODE_TABLE_SIZE in sync with scancode_to_key array size.
const Key KeyboardDriver::scancode_to_key[] = {
    Key::Unknown,  // (0x00)
    Key::Esc,       Key::Num1,        Key::Num2,
    Key::Num3,      Key::Num4,        Key::Num5,
    Key::Num6,      Key::Num7,        Key::Num8,
    Key::Num9,      Key::Num0,        Key::Minus,
    Key::Equals,    Key::Backspace,   Key::Tab,
    Key::Q,         Key::W,           Key::E,
    Key::R,         Key::T,           Key::Y,
    Key::U,         Key::I,           Key::O,
    Key::P,         Key::LeftBracket, Key::RightBracket,
    Key::Enter,     Key::LeftCtrl,    Key::A,
    Key::S,         Key::D,           Key::F,
    Key::G,         Key::H,           Key::J,
    Key::K,         Key::L,           Key::Semicolon,
    Key::Quote,     Key::Backtick,    Key::LeftShift,
    Key::Backslash, Key::Z,           Key::X,
    Key::C,         Key::V,           Key::B,
    Key::N,         Key::M,           Key::Comma,
    Key::Period,    Key::Slash,       Key::RightShift,
    Key::Unknown,  // (keypad) *
    Key::LeftAlt,   Key::Space,       Key::CapsLock,
    Key::F1,        Key::F2,          Key::F3,
    Key::F4,        Key::F5,          Key::F6,
    Key::F7,        Key::F8,          Key::F9,
    Key::F10,
};
