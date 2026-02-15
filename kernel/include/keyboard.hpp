#pragma once

#include <stddef.h>
#include <stdint.h>

enum class Key;
struct KeyEvent;

class KeyboardDriver {
   public:
    KeyboardDriver();
    ~KeyboardDriver() = default;
    KeyboardDriver(const KeyboardDriver &) = delete;
    KeyboardDriver &operator=(const KeyboardDriver &) = delete;

    void process_scancode(uint8_t scancode);

    char lookup_ascii(Key key) const;
    static Key scancode_to_key(uint8_t scancode);
    KeyEvent scancode_to_event(uint8_t scancode);

   private:
    Key lookup_extended(uint8_t scancode);

    bool shift_ = false;
    bool ctrl_ = false;
    bool alt_ = false;
    bool extended_scancode_ = false;
};

extern KeyboardDriver keyboard;

// clang-format off
enum class Key {
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,

    // Special Keys
    Enter, Backspace, Tab, Space,
    Minus, Plus, Equals, LeftBracket,
    RightBracket, Backslash, Semicolon,
    Apostrophe, Grave, Comma, Period,
    Slash, Esc, Quote, Backtick,

    // Modifiers
    LeftShift, RightShift,
    LeftCtrl, RightCtrl,
    LeftAlt, RightAlt,
    CapsLock,

    // Function Keys
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,

    // Navigation
    Insert, Delete,
    Home, End,
    PageUp, PageDown,

    // Arrow Keys
    Up, Down, Left, Right,

    Unknown,
};
// clang-format on

struct KeyEvent {
    Key key;
    bool pressed;  // true if pressed, false if released
    char ascii;
    bool shift;
    bool ctrl;
    bool alt;
};
