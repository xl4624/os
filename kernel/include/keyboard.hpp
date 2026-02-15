#pragma once

#include <stdint.h>

// clang-format off
class Key {
public:
    enum Value : uint8_t {
        Unknown = 0,
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Space, Enter, Backspace, Tab, Esc,
        Minus, Equals, LeftBracket, RightBracket, Backslash,
        Semicolon, Apostrophe, Grave, Comma, Period, Slash,
        LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt, CapsLock,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        Insert, Delete, Home, End, PageUp, PageDown,
        Up, Down, Left, Right,
        COUNT,
    };
// clang-format on

    constexpr Key() : value_(Unknown) {}
    constexpr Key(Value v) : value_(v) {}

    constexpr Value value() const { return value_; }

    constexpr bool operator==(Key other) const { return value_ == other.value_; }
    constexpr bool operator!=(Key other) const { return value_ != other.value_; }
    // Allow comparisons directly with enum values: event.key == Key::Enter
    constexpr bool operator==(Value v) const { return value_ == v; }
    constexpr bool operator!=(Value v) const { return value_ != v; }

    // Construct from a PS/2 scan set 1 make-code (0x00–0x44).
    static Key from_scancode(uint8_t scancode);
    // Construct from a PS/2 extended (0xE0-prefix) make-code.
    static Key from_extended_scancode(uint8_t scancode);

    // Returns the ASCII character produced by this key, or 0 if non-printable.
    // Pass shift=true when a shift modifier is held.
    char ascii(bool shift) const;

    // Returns true for shift, ctrl, alt, and capslock keys.
    bool is_modifier() const;

private:
    Value value_;
};

struct KeyEvent {
    const Key key;
    const bool pressed;
    const char ascii;
    const bool shift;
    const bool ctrl;
    const bool alt;
};

class KeyboardDriver {
public:
    KeyboardDriver();
    ~KeyboardDriver() = default;
    KeyboardDriver(const KeyboardDriver &) = delete;
    KeyboardDriver &operator=(const KeyboardDriver &) = delete;

    void process_scancode(uint8_t scancode);
    KeyEvent scancode_to_event(uint8_t scancode);

private:
    bool shift_ = false;
    bool ctrl_ = false;
    bool alt_ = false;
    bool extended_scancode_ = false;
};

extern KeyboardDriver keyboard;
