#include "keyboard.h"
#include "ktest.h"

// ---------------------------------------------------------------------------
// Key::from_scancode
// ---------------------------------------------------------------------------

TEST(keyboard, from_scancode_unknown_at_zero) {
    Key k = Key::from_scancode(0x00);
    ASSERT_EQ(k, Key::Unknown);
}

TEST(keyboard, from_scancode_esc) {
    Key k = Key::from_scancode(0x01);
    ASSERT_EQ(k, Key::Esc);
}

TEST(keyboard, from_scancode_digit_row) {
    ASSERT_EQ(Key::from_scancode(0x02), Key::Num1);
    ASSERT_EQ(Key::from_scancode(0x03), Key::Num2);
    ASSERT_EQ(Key::from_scancode(0x04), Key::Num3);
    ASSERT_EQ(Key::from_scancode(0x05), Key::Num4);
    ASSERT_EQ(Key::from_scancode(0x06), Key::Num5);
    ASSERT_EQ(Key::from_scancode(0x07), Key::Num6);
    ASSERT_EQ(Key::from_scancode(0x08), Key::Num7);
    ASSERT_EQ(Key::from_scancode(0x09), Key::Num8);
    ASSERT_EQ(Key::from_scancode(0x0A), Key::Num9);
    ASSERT_EQ(Key::from_scancode(0x0B), Key::Num0);
}

TEST(keyboard, from_scancode_qwerty_row) {
    ASSERT_EQ(Key::from_scancode(0x10), Key::Q);
    ASSERT_EQ(Key::from_scancode(0x11), Key::W);
    ASSERT_EQ(Key::from_scancode(0x12), Key::E);
    ASSERT_EQ(Key::from_scancode(0x13), Key::R);
    ASSERT_EQ(Key::from_scancode(0x14), Key::T);
    ASSERT_EQ(Key::from_scancode(0x15), Key::Y);
    ASSERT_EQ(Key::from_scancode(0x16), Key::U);
    ASSERT_EQ(Key::from_scancode(0x17), Key::I);
    ASSERT_EQ(Key::from_scancode(0x18), Key::O);
    ASSERT_EQ(Key::from_scancode(0x19), Key::P);
}

TEST(keyboard, from_scancode_home_row) {
    ASSERT_EQ(Key::from_scancode(0x1E), Key::A);
    ASSERT_EQ(Key::from_scancode(0x1F), Key::S);
    ASSERT_EQ(Key::from_scancode(0x20), Key::D);
    ASSERT_EQ(Key::from_scancode(0x21), Key::F);
    ASSERT_EQ(Key::from_scancode(0x22), Key::G);
    ASSERT_EQ(Key::from_scancode(0x23), Key::H);
    ASSERT_EQ(Key::from_scancode(0x24), Key::J);
    ASSERT_EQ(Key::from_scancode(0x25), Key::K);
    ASSERT_EQ(Key::from_scancode(0x26), Key::L);
}

TEST(keyboard, from_scancode_bottom_row) {
    ASSERT_EQ(Key::from_scancode(0x2C), Key::Z);
    ASSERT_EQ(Key::from_scancode(0x2D), Key::X);
    ASSERT_EQ(Key::from_scancode(0x2E), Key::C);
    ASSERT_EQ(Key::from_scancode(0x2F), Key::V);
    ASSERT_EQ(Key::from_scancode(0x30), Key::B);
    ASSERT_EQ(Key::from_scancode(0x31), Key::N);
    ASSERT_EQ(Key::from_scancode(0x32), Key::M);
}

TEST(keyboard, from_scancode_control_keys) {
    ASSERT_EQ(Key::from_scancode(0x0E), Key::Backspace);
    ASSERT_EQ(Key::from_scancode(0x0F), Key::Tab);
    ASSERT_EQ(Key::from_scancode(0x1C), Key::Enter);
    ASSERT_EQ(Key::from_scancode(0x39), Key::Space);
}

TEST(keyboard, from_scancode_modifiers) {
    ASSERT_EQ(Key::from_scancode(0x2A), Key::LeftShift);
    ASSERT_EQ(Key::from_scancode(0x36), Key::RightShift);
    ASSERT_EQ(Key::from_scancode(0x1D), Key::LeftCtrl);
    ASSERT_EQ(Key::from_scancode(0x38), Key::LeftAlt);
    ASSERT_EQ(Key::from_scancode(0x3A), Key::CapsLock);
}

TEST(keyboard, from_scancode_function_keys) {
    ASSERT_EQ(Key::from_scancode(0x3B), Key::F1);
    ASSERT_EQ(Key::from_scancode(0x3C), Key::F2);
    ASSERT_EQ(Key::from_scancode(0x3D), Key::F3);
    ASSERT_EQ(Key::from_scancode(0x3E), Key::F4);
    ASSERT_EQ(Key::from_scancode(0x3F), Key::F5);
    ASSERT_EQ(Key::from_scancode(0x40), Key::F6);
    ASSERT_EQ(Key::from_scancode(0x41), Key::F7);
    ASSERT_EQ(Key::from_scancode(0x42), Key::F8);
    ASSERT_EQ(Key::from_scancode(0x43), Key::F9);
    ASSERT_EQ(Key::from_scancode(0x44), Key::F10);
}

TEST(keyboard, from_scancode_punctuation) {
    ASSERT_EQ(Key::from_scancode(0x0C), Key::Minus);
    ASSERT_EQ(Key::from_scancode(0x0D), Key::Equals);
    ASSERT_EQ(Key::from_scancode(0x1A), Key::LeftBracket);
    ASSERT_EQ(Key::from_scancode(0x1B), Key::RightBracket);
    ASSERT_EQ(Key::from_scancode(0x27), Key::Semicolon);
    ASSERT_EQ(Key::from_scancode(0x28), Key::Apostrophe);
    ASSERT_EQ(Key::from_scancode(0x29), Key::Grave);
    ASSERT_EQ(Key::from_scancode(0x2B), Key::Backslash);
    ASSERT_EQ(Key::from_scancode(0x33), Key::Comma);
    ASSERT_EQ(Key::from_scancode(0x34), Key::Period);
    ASSERT_EQ(Key::from_scancode(0x35), Key::Slash);
}

TEST(keyboard, from_scancode_out_of_range_returns_unknown) {
    // Anything >= kScancodeTableSize (69 = 0x45) should return Unknown.
    ASSERT_EQ(Key::from_scancode(0x45), Key::Unknown);
    ASSERT_EQ(Key::from_scancode(0x60), Key::Unknown);
    ASSERT_EQ(Key::from_scancode(0xFF), Key::Unknown);
}

// ---------------------------------------------------------------------------
// Key::from_extended_scancode
// ---------------------------------------------------------------------------

TEST(keyboard, from_extended_scancode_arrows) {
    ASSERT_EQ(Key::from_extended_scancode(0x48), Key::Up);
    ASSERT_EQ(Key::from_extended_scancode(0x50), Key::Down);
    ASSERT_EQ(Key::from_extended_scancode(0x4B), Key::Left);
    ASSERT_EQ(Key::from_extended_scancode(0x4D), Key::Right);
}

TEST(keyboard, from_extended_scancode_unknown) {
    ASSERT_EQ(Key::from_extended_scancode(0x00), Key::Unknown);
    ASSERT_EQ(Key::from_extended_scancode(0x1C), Key::Unknown);  // numpad Enter
    ASSERT_EQ(Key::from_extended_scancode(0xFF), Key::Unknown);
}

// ---------------------------------------------------------------------------
// Key::ascii
// ---------------------------------------------------------------------------

TEST(keyboard, ascii_lowercase_letters) {
    for (uint8_t sc = 0x10; sc <= 0x19; sc++) {  // Q-P row
        Key k = Key::from_scancode(sc);
        char ch = k.ascii(false);
        ASSERT(ch >= 'a' && ch <= 'z');
    }
}

TEST(keyboard, ascii_uppercase_with_shift) {
    for (uint8_t sc = 0x10; sc <= 0x19; sc++) {  // Q-P row
        Key k = Key::from_scancode(sc);
        char lo = k.ascii(false);
        char hi = k.ascii(true);
        ASSERT_EQ(hi, lo - 32);  // uppercase = lowercase - 32 in ASCII
    }
}

TEST(keyboard, ascii_specific_letters) {
    ASSERT_EQ(Key::from_scancode(0x1E).ascii(false), 'a');
    ASSERT_EQ(Key::from_scancode(0x1E).ascii(true), 'A');
    ASSERT_EQ(Key::from_scancode(0x30).ascii(false), 'b');
    ASSERT_EQ(Key::from_scancode(0x2E).ascii(false), 'c');
}

TEST(keyboard, ascii_digits_unshifted) {
    ASSERT_EQ(Key::from_scancode(0x02).ascii(false), '1');
    ASSERT_EQ(Key::from_scancode(0x03).ascii(false), '2');
    ASSERT_EQ(Key::from_scancode(0x04).ascii(false), '3');
    ASSERT_EQ(Key::from_scancode(0x05).ascii(false), '4');
    ASSERT_EQ(Key::from_scancode(0x06).ascii(false), '5');
    ASSERT_EQ(Key::from_scancode(0x07).ascii(false), '6');
    ASSERT_EQ(Key::from_scancode(0x08).ascii(false), '7');
    ASSERT_EQ(Key::from_scancode(0x09).ascii(false), '8');
    ASSERT_EQ(Key::from_scancode(0x0A).ascii(false), '9');
    ASSERT_EQ(Key::from_scancode(0x0B).ascii(false), '0');
}

TEST(keyboard, ascii_digits_shifted_symbols) {
    ASSERT_EQ(Key::from_scancode(0x02).ascii(true), '!');
    ASSERT_EQ(Key::from_scancode(0x03).ascii(true), '@');
    ASSERT_EQ(Key::from_scancode(0x04).ascii(true), '#');
    ASSERT_EQ(Key::from_scancode(0x05).ascii(true), '$');
    ASSERT_EQ(Key::from_scancode(0x06).ascii(true), '%');
    ASSERT_EQ(Key::from_scancode(0x07).ascii(true), '^');
    ASSERT_EQ(Key::from_scancode(0x08).ascii(true), '&');
    ASSERT_EQ(Key::from_scancode(0x09).ascii(true), '*');
    ASSERT_EQ(Key::from_scancode(0x0A).ascii(true), '(');
    ASSERT_EQ(Key::from_scancode(0x0B).ascii(true), ')');
}

TEST(keyboard, ascii_punctuation) {
    ASSERT_EQ(Key::from_scancode(0x0C).ascii(false), '-');
    ASSERT_EQ(Key::from_scancode(0x0C).ascii(true), '_');
    ASSERT_EQ(Key::from_scancode(0x0D).ascii(false), '=');
    ASSERT_EQ(Key::from_scancode(0x0D).ascii(true), '+');
    ASSERT_EQ(Key::from_scancode(0x1A).ascii(false), '[');
    ASSERT_EQ(Key::from_scancode(0x1A).ascii(true), '{');
    ASSERT_EQ(Key::from_scancode(0x1B).ascii(false), ']');
    ASSERT_EQ(Key::from_scancode(0x1B).ascii(true), '}');
    ASSERT_EQ(Key::from_scancode(0x2B).ascii(false), '\\');
    ASSERT_EQ(Key::from_scancode(0x2B).ascii(true), '|');
    ASSERT_EQ(Key::from_scancode(0x27).ascii(false), ';');
    ASSERT_EQ(Key::from_scancode(0x27).ascii(true), ':');
    ASSERT_EQ(Key::from_scancode(0x28).ascii(false), '\'');
    ASSERT_EQ(Key::from_scancode(0x28).ascii(true), '"');
    ASSERT_EQ(Key::from_scancode(0x29).ascii(false), '`');
    ASSERT_EQ(Key::from_scancode(0x29).ascii(true), '~');
    ASSERT_EQ(Key::from_scancode(0x33).ascii(false), ',');
    ASSERT_EQ(Key::from_scancode(0x33).ascii(true), '<');
    ASSERT_EQ(Key::from_scancode(0x34).ascii(false), '.');
    ASSERT_EQ(Key::from_scancode(0x34).ascii(true), '>');
    ASSERT_EQ(Key::from_scancode(0x35).ascii(false), '/');
    ASSERT_EQ(Key::from_scancode(0x35).ascii(true), '?');
}

TEST(keyboard, ascii_whitespace_keys) {
    ASSERT_EQ(Key::from_scancode(0x39).ascii(false), ' ');
    ASSERT_EQ(Key::from_scancode(0x39).ascii(true), ' ');
    ASSERT_EQ(Key(Key::Enter).ascii(false), '\n');
    ASSERT_EQ(Key(Key::Backspace).ascii(false), '\b');
    ASSERT_EQ(Key(Key::Tab).ascii(false), '\t');
}

TEST(keyboard, ascii_non_printable_keys_return_zero) {
    // Esc, function keys, and arrows should return 0.
    ASSERT_EQ(Key(Key::Esc).ascii(false), '\0');
    ASSERT_EQ(Key(Key::F1).ascii(false), '\0');
    ASSERT_EQ(Key(Key::F12).ascii(false), '\0');
    ASSERT_EQ(Key(Key::Up).ascii(false), '\0');
    ASSERT_EQ(Key(Key::Down).ascii(false), '\0');
    ASSERT_EQ(Key(Key::Left).ascii(false), '\0');
    ASSERT_EQ(Key(Key::Right).ascii(false), '\0');
    ASSERT_EQ(Key(Key::Insert).ascii(false), '\0');
    ASSERT_EQ(Key(Key::Delete).ascii(false), '\0');
    ASSERT_EQ(Key(Key::Home).ascii(false), '\0');
    ASSERT_EQ(Key(Key::End).ascii(false), '\0');
    ASSERT_EQ(Key(Key::PageUp).ascii(false), '\0');
    ASSERT_EQ(Key(Key::PageDown).ascii(false), '\0');
}

TEST(keyboard, ascii_modifiers_return_zero) {
    ASSERT_EQ(Key(Key::LeftShift).ascii(false), '\0');
    ASSERT_EQ(Key(Key::RightShift).ascii(false), '\0');
    ASSERT_EQ(Key(Key::LeftCtrl).ascii(false), '\0');
    ASSERT_EQ(Key(Key::RightCtrl).ascii(false), '\0');
    ASSERT_EQ(Key(Key::LeftAlt).ascii(false), '\0');
    ASSERT_EQ(Key(Key::RightAlt).ascii(false), '\0');
    ASSERT_EQ(Key(Key::CapsLock).ascii(false), '\0');
}

TEST(keyboard, ascii_unknown_returns_zero) {
    ASSERT_EQ(Key(Key::Unknown).ascii(false), '\0');
    ASSERT_EQ(Key(Key::Unknown).ascii(true), '\0');
}

// ---------------------------------------------------------------------------
// Key::is_modifier
// ---------------------------------------------------------------------------

TEST(keyboard, is_modifier_true_for_modifiers) {
    ASSERT_TRUE(Key(Key::LeftShift).is_modifier());
    ASSERT_TRUE(Key(Key::RightShift).is_modifier());
    ASSERT_TRUE(Key(Key::LeftCtrl).is_modifier());
    ASSERT_TRUE(Key(Key::RightCtrl).is_modifier());
    ASSERT_TRUE(Key(Key::LeftAlt).is_modifier());
    ASSERT_TRUE(Key(Key::RightAlt).is_modifier());
    ASSERT_TRUE(Key(Key::CapsLock).is_modifier());
}

TEST(keyboard, is_modifier_false_for_printable_keys) {
    ASSERT_FALSE(Key(Key::A).is_modifier());
    ASSERT_FALSE(Key(Key::Z).is_modifier());
    ASSERT_FALSE(Key(Key::Num0).is_modifier());
    ASSERT_FALSE(Key(Key::Space).is_modifier());
    ASSERT_FALSE(Key(Key::Enter).is_modifier());
}

TEST(keyboard, is_modifier_false_for_function_and_nav_keys) {
    ASSERT_FALSE(Key(Key::F1).is_modifier());
    ASSERT_FALSE(Key(Key::F12).is_modifier());
    ASSERT_FALSE(Key(Key::Up).is_modifier());
    ASSERT_FALSE(Key(Key::Esc).is_modifier());
    ASSERT_FALSE(Key(Key::Unknown).is_modifier());
}

// ---------------------------------------------------------------------------
// KeyboardDriver::scancode_to_event
// ---------------------------------------------------------------------------

TEST(keyboard, scancode_to_event_key_press) {
    // 0x1E = A make-code (press).
    KeyEvent ev = kKeyboard.scancode_to_event(0x1E);
    ASSERT_EQ(ev.key, Key::A);
    ASSERT_TRUE(ev.pressed);
}

TEST(keyboard, scancode_to_event_key_release) {
    // 0x9E = 0x1E | 0x80 = A break-code (release).
    KeyEvent ev = kKeyboard.scancode_to_event(0x9E);
    ASSERT_EQ(ev.key, Key::A);
    ASSERT_FALSE(ev.pressed);
}

TEST(keyboard, scancode_to_event_ascii_on_press) {
    KeyEvent ev = kKeyboard.scancode_to_event(0x1E);  // A
    ASSERT_EQ(ev.ascii, 'a');
}

TEST(keyboard, scancode_to_event_no_ascii_on_release) {
    KeyEvent ev = kKeyboard.scancode_to_event(0x9E);  // A release
    ASSERT_EQ(ev.ascii, '\0');
}

TEST(keyboard, scancode_to_event_extended_prefix_returns_no_key) {
    // 0xE0 is the extended prefix byte; it is not a key itself.
    KeyEvent ev = kKeyboard.scancode_to_event(0xE0);
    ASSERT_EQ(ev.key, Key::Unknown);
    ASSERT_FALSE(ev.pressed);
}

TEST(keyboard, scancode_to_event_extended_arrow_sequence) {
    // Two-byte sequence: 0xE0, 0x48 → Up arrow press.
    kKeyboard.scancode_to_event(0xE0);  // consume prefix
    KeyEvent ev = kKeyboard.scancode_to_event(0x48);
    ASSERT_EQ(ev.key, Key::Up);
    ASSERT_TRUE(ev.pressed);
    ASSERT_EQ(ev.ascii, '\0');
}

TEST(keyboard, scancode_to_event_extended_arrow_release) {
    // 0xE0, 0xC8 → Up arrow release (0x48 | 0x80 = 0xC8).
    kKeyboard.scancode_to_event(0xE0);
    KeyEvent ev = kKeyboard.scancode_to_event(0xC8);
    ASSERT_EQ(ev.key, Key::Up);
    ASSERT_FALSE(ev.pressed);
}

TEST(keyboard, shift_state_tracked) {
    // Press LeftShift (0x2A), then 'a' (0x1E), then release shift (0xAA).
    kKeyboard.scancode_to_event(0x2A);                        // LeftShift press
    KeyEvent with_shift = kKeyboard.scancode_to_event(0x1E);  // A
    ASSERT_TRUE(with_shift.shift);
    ASSERT_EQ(with_shift.ascii, 'A');

    kKeyboard.scancode_to_event(0xAA);  // LeftShift release
    KeyEvent without_shift = kKeyboard.scancode_to_event(0x1E);  // A
    ASSERT_FALSE(without_shift.shift);
    ASSERT_EQ(without_shift.ascii, 'a');
}

TEST(keyboard, right_shift_state_tracked) {
    kKeyboard.scancode_to_event(0x36);                // RightShift press
    KeyEvent ev = kKeyboard.scancode_to_event(0x1E);  // A
    ASSERT_TRUE(ev.shift);
    ASSERT_EQ(ev.ascii, 'A');
    kKeyboard.scancode_to_event(0xB6);  // RightShift release
}

TEST(keyboard, ctrl_state_tracked) {
    kKeyboard.scancode_to_event(0x1D);                // LeftCtrl press
    KeyEvent ev = kKeyboard.scancode_to_event(0x1E);  // A
    ASSERT_TRUE(ev.ctrl);
    // Ctrl suppresses ASCII output.
    ASSERT_EQ(ev.ascii, '\0');
    kKeyboard.scancode_to_event(0x9D);  // LeftCtrl release
}

TEST(keyboard, alt_state_tracked) {
    kKeyboard.scancode_to_event(0x38);                // LeftAlt press
    KeyEvent ev = kKeyboard.scancode_to_event(0x1E);  // A
    ASSERT_TRUE(ev.alt);
    // Alt suppresses ASCII output.
    ASSERT_EQ(ev.ascii, '\0');
    kKeyboard.scancode_to_event(0xB8);  // LeftAlt release
}

TEST(keyboard, modifier_key_press_has_no_ascii) {
    KeyEvent ev = kKeyboard.scancode_to_event(0x2A);  // LeftShift press
    ASSERT_EQ(ev.ascii, '\0');
    kKeyboard.scancode_to_event(0xAA);  // release
}

// ---------------------------------------------------------------------------
// KeyboardDriver::flush_command_queue
// ---------------------------------------------------------------------------

TEST(keyboard, queue_empty_after_flush) {
    kKeyboard.flush_command_queue();
    ASSERT_TRUE(kKeyboard.is_command_queue_empty());
}

TEST(keyboard, flush_is_idempotent) {
    kKeyboard.flush_command_queue();
    kKeyboard.flush_command_queue();
    ASSERT_TRUE(kKeyboard.is_command_queue_empty());
}
