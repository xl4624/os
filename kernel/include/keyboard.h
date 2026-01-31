#pragma once

#include <stddef.h>
#include <stdint.h>

// Maximum number of pending keyboard commands in the queue.
static constexpr size_t kKeyboardCommandQueueSize = 8;

enum class PS2Command : uint8_t {
    SetLEDs = 0xED,
    Echo = 0xEE,
    GetScanCodeSet = 0xF0,
    Identify = 0xF2,
    SetTypematicRate = 0xF3,
    EnableScanning = 0xF4,
    DisableScanning = 0xF5,
    SetDefaults = 0xF6,
    Resend = 0xFE,
    Reset = 0xFF,
};

// PS/2 Keyboard response codes.
static constexpr uint8_t kPS2Ack = 0xFA;
static constexpr uint8_t kPS2Resend = 0xFE;
static constexpr uint8_t kPS2Echo = 0xEE;
static constexpr uint8_t kPS2TestPass = 0xAA;
static constexpr uint8_t kPS2Break = 0xF0;

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

    constexpr Value value() const {
        return value_;
    }

    constexpr bool operator==(Key other) const {
        return value_ == other.value_;
    }
    constexpr bool operator!=(Key other) const {
        return value_ != other.value_;
    }

    // Allow comparisons directly with enum values: event.key == Key::Enter
    constexpr bool operator==(Value v) const {
        return value_ == v;
    }
    constexpr bool operator!=(Value v) const {
        return value_ != v;
    }

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

// Command queue entry for sending commands to the PS/2 keyboard.
struct PS2CommandEntry {
    PS2Command command;
    uint8_t parameter;  // Valid if has_parameter is true.
    bool has_parameter;
    int retry_count;  // Number of resend attempts.
};

// State machine states for the keyboard command queue.
enum class PS2State : uint8_t {
    Idle,            // Not processing any command.
    WaitingForAck,   // Waiting for ACK (0xFA) response.
    WaitingForData,  // Waiting for data byte (e.g., after identify).
};

class KeyboardDriver {
   public:
    KeyboardDriver();
    ~KeyboardDriver() = default;
    KeyboardDriver(const KeyboardDriver &) = delete;
    KeyboardDriver &operator=(const KeyboardDriver &) = delete;

    void process_scancode(uint8_t scancode);
    KeyEvent scancode_to_event(uint8_t scancode);

    // =================================================================
    // Command queue API
    // =================================================================

    bool send_command(PS2Command command);
    bool send_command_with_param(PS2Command command, uint8_t parameter);
    bool is_command_queue_empty() const;
    void flush_command_queue();

   private:
    // Process a response byte from the keyboard (returns true if consumed).
    bool process_response(uint8_t data);
    // Start processing the next command in the queue
    void process_command_queue();
    // Send a byte to the keyboard controller data port.
    void send_byte(uint8_t data);
    // Wait for output buffer to have data.
    bool wait_for_output() const;
    // Wait for input buffer to be empty before sending.
    bool wait_for_input() const;

    bool shift_ = false;
    bool ctrl_ = false;
    bool alt_ = false;
    bool extended_scancode_ = false;

    // Command queue and state machine.
    PS2CommandEntry command_queue_[kKeyboardCommandQueueSize];
    size_t queue_head_ = 0;
    size_t queue_tail_ = 0;
    size_t queue_count_ = 0;
    PS2State state_ = PS2State::Idle;
    static constexpr int kMaxRetries = 3;
};

extern KeyboardDriver kKeyboard;
