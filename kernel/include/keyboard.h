#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/kbd.h>

__BEGIN_DECLS

// Non-blocking read of one ASCII character from the keyboard ring buffer.
// Returns the character as unsigned char cast to int, or EOF if none available.
int keyboard_getchar(void);

__END_DECLS

#ifdef __cplusplus

#include "ps2_command_queue.h"
#include "ring_buffer.h"

// Size of keyboard input buffer for sys_read system call
static constexpr size_t kKeyboardInputBufferSize = 128;

// Size of raw key event buffer for /dev/kbd
static constexpr size_t kKeyEventBufferSize = 64;

/**
 * Key: Represents a physical keyboard key (not modified by shift/ctrl/alt).
 *
 * This class abstracts PS/2 scancode translation. It knows about scan set 1
 * make codes and extended (0xE0-prefixed) codes, converting them to a
 * hardware-independent key identifier.
 *
 * Usage:
 *   Key k = Key::from_scancode(0x1E);  // Returns Key::A
 *   char c = k.ascii(false);           // Returns 'a' (or 'A' if shifted)
 */
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
  /*
   * Allow comparisons directly with enum values: event.key == Key::Enter
   */
  constexpr bool operator==(Value v) const { return value_ == v; }
  constexpr bool operator!=(Value v) const { return value_ != v; }

  /**
   * Converts PS/2 scan set 1 make code to Key.
   * Returns Key::Unknown for unrecognized scancodes.
   */
  static Key from_scancode(uint8_t scancode);

  /**
   * Converts extended (0xE0-prefix) make code to Key.
   * Handles arrow keys and extended keypad.
   */
  static Key from_extended_scancode(uint8_t scancode);

  /**
   * Returns ASCII character for this key, or 0 if non-printable.
   *
   * shift: true for shifted character (e.g., 'A' vs 'a')
   * Note: Does not handle caps lock
   */
  char ascii(bool shift) const;

  /**
   * Returns true for modifier keys (shift, ctrl, alt, capslock).
   * Modifiers don't produce ASCII characters.
   */
  bool is_modifier() const;

 private:
  Value value_;
};

struct KeyEvent {
  const Key key;
  const bool pressed;  // true = press, false = release
  const char ascii;    // \0 if non-printable or modifier held
  const bool shift;
  const bool ctrl;
  const bool alt;
};

class KeyboardDriver {
 public:
  KeyboardDriver() = default;
  ~KeyboardDriver() = default;

  // Registers the keyboard IRQ handler.
  static void init();
  KeyboardDriver(const KeyboardDriver&) = delete;
  KeyboardDriver& operator=(const KeyboardDriver&) = delete;

  /**
   * Processes raw scancode byte from hardware.
   */
  void process_scancode(uint8_t scancode);

  /**
   * Converts scancode to KeyEvent with current modifier state.
   */
  KeyEvent scancode_to_event(uint8_t scancode);

  /**
   * Reads buffered keystrokes into user buffer.
   *
   * Reads up to count characters from the internal ring buffer.
   */
  size_t read(char* buf, size_t count);

  /**
   * Reads buffered key events into user buffer.
   *
   * Returns number of events read (0 if none pending).
   */
  size_t read_events(kbd_event* buf, size_t max_events);

  /**
   * Sends PS/2 command to keyboard controller.
   */
  bool send_command(PS2Command command) { return cmd_queue_.send_command(command); }

  /**
   * Sends PS/2 command with parameter byte.
   */
  bool send_command_with_param(PS2Command command, uint8_t parameter) {
    return cmd_queue_.send_command_with_param(command, parameter);
  }

  /**
   * Returns true when all pending commands have completed.
   */
  bool is_command_queue_empty() const { return cmd_queue_.is_idle(); }

  /**
   * Cancels all pending commands.
   */
  void flush_command_queue() { cmd_queue_.flush(); }

  /**
   * Discards all pending raw key events from the /dev/kbd buffer.
   * Called when a process opens /dev/kbd so it does not see keystrokes
   * that were typed before it started (e.g. the command name itself).
   */
  void flush_events() { event_buffer_.clear(); }

 private:
  /**
   * Buffers printable character for sys_read.
   * Silently drops character if input buffer is full.
   */
  void buffer_char(char c);

  // Modifier state tracked across scancodes
  bool shift_ = false;
  bool ctrl_ = false;
  bool alt_ = false;
  bool extended_scancode_ = false;  // Received 0xE0 prefix

  // Input buffer for sys_read (ASCII characters)
  RingBuffer<char, kKeyboardInputBufferSize> input_buffer_;

  // Raw key event buffer for /dev/kbd
  RingBuffer<kbd_event, kKeyEventBufferSize> event_buffer_;

  // PS/2 command protocol handler
  PS2CommandQueue cmd_queue_;
};

extern KeyboardDriver kKeyboard;

#endif  // __cplusplus
