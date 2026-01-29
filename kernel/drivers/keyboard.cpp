#include "keyboard.hpp"

#include <assert.h>
#include <sys/io.h>

#include "interrupt.hpp"
#include "tty.hpp"

// TODO: https://wiki.osdev.org/PS/2_Keyboard#Command_Queue_and_State_Machine

namespace {

    constexpr uint16_t kDataPort = 0x60;
    constexpr uint8_t kExtended = 0xE0;
    constexpr uint8_t kReleaseBit = 0x80;

    constexpr uint8_t kExtUp = 0x48;
    constexpr uint8_t kExtDown = 0x50;
    constexpr uint8_t kExtLeft = 0x4B;
    constexpr uint8_t kExtRight = 0x4D;

    // PS/2 scan set 1 make-codes 0x00–0x44 → Key::Value.
    // The array is indexed directly by scancode byte.
    // clang-format off
    constexpr size_t kScancodeTableSize = 69;
    constexpr Key::Value kScancodeTable[kScancodeTableSize] = {
        Key::Unknown,                                                        // 0x00
        Key::Esc,                                                            // 0x01
        Key::Num1,      Key::Num2,      Key::Num3,      Key::Num4,           // 0x02–0x05
        Key::Num5,      Key::Num6,      Key::Num7,      Key::Num8,           // 0x06–0x09
        Key::Num9,      Key::Num0,      Key::Minus,     Key::Equals,         // 0x0A–0x0D
        Key::Backspace, Key::Tab,                                            // 0x0E–0x0F
        Key::Q,         Key::W,         Key::E,         Key::R,              // 0x10–0x13
        Key::T,         Key::Y,         Key::U,         Key::I,              // 0x14–0x17
        Key::O,         Key::P,         Key::LeftBracket, Key::RightBracket, // 0x18–0x1B
        Key::Enter,     Key::LeftCtrl,                                       // 0x1C–0x1D
        Key::A,         Key::S,         Key::D,         Key::F,              // 0x1E–0x21
        Key::G,         Key::H,         Key::J,         Key::K,              // 0x22–0x25
        Key::L,         Key::Semicolon, Key::Apostrophe, Key::Grave,         // 0x26–0x29
        Key::LeftShift, Key::Backslash,                                      // 0x2A–0x2B
        Key::Z,         Key::X,         Key::C,         Key::V,              // 0x2C–0x2F
        Key::B,         Key::N,         Key::M,         Key::Comma,          // 0x30–0x33
        Key::Period,    Key::Slash,     Key::RightShift,                     // 0x34–0x36
        Key::Unknown,                                                        // 0x37 (keypad *)
        Key::LeftAlt,   Key::Space,     Key::CapsLock,                       // 0x38–0x3A
        Key::F1,        Key::F2,        Key::F3,        Key::F4,             // 0x3B–0x3E
        Key::F5,        Key::F6,        Key::F7,        Key::F8,             // 0x3F–0x42
        Key::F9,        Key::F10,                                            // 0x43–0x44
    };
    // clang-format on

    static_assert(sizeof(kScancodeTable) / sizeof(Key::Value)
                      == kScancodeTableSize,
                  "kScancodeTable size must match kScancodeTableSize");

    // ASCII lookup table indexed by Key::Value.
    // Entries are {normal, shifted} characters; 0 means non-printable.
    // clang-format off
    struct AsciiPair { char normal; char shifted; };
    constexpr AsciiPair kAsciiTable[Key::COUNT] = {
        {0, 0},                                   // Unknown
        {'a','A'}, {'b','B'}, {'c','C'},          // A  B  C
        {'d','D'}, {'e','E'}, {'f','F'},          // D  E  F
        {'g','G'}, {'h','H'}, {'i','I'},          // G  H  I
        {'j','J'}, {'k','K'}, {'l','L'},          // J  K  L
        {'m','M'}, {'n','N'}, {'o','O'},          // M  N  O
        {'p','P'}, {'q','Q'}, {'r','R'},          // P  Q  R
        {'s','S'}, {'t','T'}, {'u','U'},          // S  T  U
        {'v','V'}, {'w','W'}, {'x','X'},          // V  W  X
        {'y','Y'}, {'z','Z'},                     // Y  Z
        {'0',')'}, {'1','!'}, {'2','@'},          // Num0  Num1  Num2
        {'3','#'}, {'4','$'}, {'5','%'},          // Num3  Num4  Num5
        {'6','^'}, {'7','&'}, {'8','*'},          // Num6  Num7  Num8
        {'9','('},                                // Num9
        {' ',' '}, {'\n','\n'},                  // Space  Enter
        {'\b','\b'}, {'\t','\t'},                // Backspace  Tab
        {0, 0},                                   // Esc
        {'-','_'}, {'=','+'},                    // Minus  Equals
        {'[','{'}, {']','}'},                    // LeftBracket  RightBracket
        {'\\','|'},                               // Backslash
        {';',':'}, {'\'','"'}, {'`','~'},        // Semicolon  Apostrophe  Grave
        {',','<'}, {'.','>'}, {'/','?'},         // Comma  Period  Slash
        // Modifiers (no ASCII)
        {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, // LeftShift..CapsLock
        // Function keys (no ASCII)
        {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},         // F1..F6
        {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},         // F7..F12
        // Navigation keys (no ASCII)
        {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},         // Insert..PageDown
        // Arrow keys (no ASCII)
        {0,0}, {0,0}, {0,0}, {0,0},                       // Up  Down  Left  Right
    };
    // clang-format on

    static_assert(sizeof(kAsciiTable) / sizeof(AsciiPair) == Key::COUNT,
                  "kAsciiTable size must match Key::COUNT");

}  // namespace

// === Key methods =============================================================

Key Key::from_scancode(uint8_t scancode) {
    if (scancode < kScancodeTableSize) {
        return Key(kScancodeTable[scancode]);
    }
    return Key(Unknown);
}

Key Key::from_extended_scancode(uint8_t scancode) {
    switch (scancode) {
        case kExtUp: return Key(Up);
        case kExtDown: return Key(Down);
        case kExtLeft: return Key(Left);
        case kExtRight: return Key(Right);
        default: return Key(Unknown);
    }
}

char Key::ascii(bool shift) const {
    assert(value_ < COUNT && "Key::ascii(): key value out of range");
    if (value_ < COUNT) {
        return shift ? kAsciiTable[value_].shifted : kAsciiTable[value_].normal;
    }
    return '\0';
}

bool Key::is_modifier() const {
    switch (value_) {
        case LeftShift:
        case RightShift:
        case LeftCtrl:
        case RightCtrl:
        case LeftAlt:
        case RightAlt:
        case CapsLock: return true;
        default: return false;
    }
}

// === KeyboardDriver ==========================================================

KeyboardDriver keyboard;

static void keyboard_handler([[maybe_unused]] interrupt_frame *frame) {
    uint8_t scancode = inb(kDataPort);
    keyboard.process_scancode(scancode);
}

KeyboardDriver::KeyboardDriver() {
    Interrupt::register_handler(IRQ::Keyboard, keyboard_handler);
}

void KeyboardDriver::process_scancode(uint8_t scancode) {
    KeyEvent event = scancode_to_event(scancode);
    kTerminal.handle_key(event);
}

KeyEvent KeyboardDriver::scancode_to_event(uint8_t scancode) {
    // The 0xE0 prefix byte is not a key event itself; set the flag and bail.
    if (scancode == kExtended) {
        extended_scancode_ = true;
        return KeyEvent{Key{}, false, '\0', shift_, ctrl_, alt_};
    }

    const bool pressed = !(scancode & kReleaseBit);
    scancode &= ~kReleaseBit;

    Key key = extended_scancode_ ? Key::from_extended_scancode(scancode)
                                 : Key::from_scancode(scancode);
    extended_scancode_ = false;

    if (key.is_modifier()) {
        switch (key.value()) {
            case Key::LeftShift:
            case Key::RightShift: shift_ = pressed; break;
            case Key::LeftCtrl:
            case Key::RightCtrl: ctrl_ = pressed; break;
            case Key::LeftAlt:
            case Key::RightAlt: alt_ = pressed; break;
            default: break;
        }
    }

    // ctrl/alt combinations suppress printable output.
    const char ascii = (pressed && !ctrl_ && !alt_) ? key.ascii(shift_) : '\0';
    return KeyEvent{key, pressed, ascii, shift_, ctrl_, alt_};
}
