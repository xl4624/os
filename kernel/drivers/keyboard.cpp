#include "keyboard.h"

#include <assert.h>
#include <sys/io.h>

#include "interrupt.h"
#include "tty.h"

static constexpr uint16_t kDataPort = 0x60;
static constexpr uint16_t kStatusPort = 0x64;
static constexpr uint8_t kStatusOutputFull = 0x01;
static constexpr uint8_t kStatusInputFull = 0x02;
static constexpr uint8_t kExtended = 0xE0;
static constexpr uint8_t kReleaseBit = 0x80;

static constexpr uint8_t kExtUp = 0x48;
static constexpr uint8_t kExtDown = 0x50;
static constexpr uint8_t kExtLeft = 0x4B;
static constexpr uint8_t kExtRight = 0x4D;

// PS/2 scan set 1 make-codes 0x00–0x44 -> Key::Value.
// The array is indexed directly by scancode byte.
// clang-format off
static constexpr size_t kScancodeTableSize = 69;
static constexpr Key::Value kScancodeTable[kScancodeTableSize] = {
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

static_assert(sizeof(kScancodeTable) / sizeof(Key::Value) == kScancodeTableSize,
              "kScancodeTable size must match kScancodeTableSize");

// ASCII lookup table indexed by Key::Value.
// Entries are {normal, shifted} characters; 0 means non-printable.
// clang-format off
struct AsciiPair { char normal; char shifted; };
static constexpr AsciiPair kAsciiTable[Key::COUNT] = {
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

// ====================================================================
// KeyboardDriver
// ====================================================================

KeyboardDriver kKeyboard;

static void keyboard_handler([[maybe_unused]] interrupt_frame *frame) {
    uint8_t scancode = inb(kDataPort);
    kKeyboard.process_scancode(scancode);
}

KeyboardDriver::KeyboardDriver() {
    Interrupt::register_handler(IRQ::Keyboard, keyboard_handler);
}

void KeyboardDriver::process_scancode(uint8_t scancode) {
    // First, try to process as a command response.
    if (process_response(scancode)) {
        return;
    }

    // If not a command response; process as key scancode.
    KeyEvent event = scancode_to_event(scancode);
    kTerminal.handle_key(event);

    // Buffer printable characters for sys_read.
    if (event.ascii != '\0') {
        buffer_char(event.ascii);
    }
}

void KeyboardDriver::buffer_char(char c) {
    if (input_count_ >= kInputBufferSize) {
        return;  // Buffer full, drop character.
    }
    input_buffer_[input_tail_] = c;
    input_tail_ = (input_tail_ + 1) % kInputBufferSize;
    ++input_count_;
}

size_t KeyboardDriver::read(char *buf, size_t count) {
    assert((count == 0 || buf != nullptr)
           && "KeyboardDriver::read(): buf is null with non-zero count");
    size_t n = 0;
    while (n < count && input_count_ > 0) {
        buf[n++] = input_buffer_[input_head_];
        input_head_ = (input_head_ + 1) % kInputBufferSize;
        --input_count_;
    }
    return n;
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

// Check if the output buffer has data to read.
bool KeyboardDriver::wait_for_output() const {
    // For interrupt-driven handling, we just check the status bit.
    // Return true if output buffer is full (data available).
    return (inb(kStatusPort) & kStatusOutputFull) != 0;
}

// Check if the input buffer is empty (ready to send).
bool KeyboardDriver::wait_for_input() const {
    // Return true if input buffer is empty (ready to send).
    return (inb(kStatusPort) & kStatusInputFull) == 0;
}

void KeyboardDriver::send_byte(uint8_t data) {
    while (!wait_for_input()) {
        // Spin-wait
        // TODO: could add timeout here
    }
    outb(kDataPort, data);
}

bool KeyboardDriver::send_command(PS2Command command) {
    assert(queue_count_ < kKeyboardCommandQueueSize
           && "KeyboardDriver::send_command(): command queue full");

    PS2CommandEntry entry{/*command=*/command, /*parameter=*/0,
                          /*has_parameter=*/false, /*retry_count=*/0};

    command_queue_[queue_tail_] = entry;
    queue_tail_ = (queue_tail_ + 1) % kKeyboardCommandQueueSize;
    ++queue_count_;

    // If idle, start processing immediately.
    if (state_ == PS2State::Idle) {
        process_command_queue();
    }

    return true;
}

bool KeyboardDriver::send_command_with_param(PS2Command command,
                                             uint8_t parameter) {
    assert(queue_count_ < kKeyboardCommandQueueSize
           && "KeyboardDriver::send_command_with_param(): command queue full");

    PS2CommandEntry entry{command, parameter, /*has_parameter=*/true,
                          /*retry_count=*/0};

    command_queue_[queue_tail_] = entry;
    queue_tail_ = (queue_tail_ + 1) % kKeyboardCommandQueueSize;
    ++queue_count_;

    // If idle, start processing immediately.
    if (state_ == PS2State::Idle) {
        process_command_queue();
    }

    return true;
}

bool KeyboardDriver::is_command_queue_empty() const {
    return queue_count_ == 0 && state_ == PS2State::Idle;
}

void KeyboardDriver::flush_command_queue() {
    queue_head_ = 0;
    queue_tail_ = 0;
    queue_count_ = 0;
    state_ = PS2State::Idle;
}

void KeyboardDriver::process_command_queue() {
    assert(queue_count_ > 0
           && "process_command_queue(): called with empty queue");

    PS2CommandEntry &entry = command_queue_[queue_head_];

    // Set state BEFORE sending so that if the IRQ fires immediately after
    // outb (before the next instruction), process_response() sees
    // WaitingForAck and handles the ACK correctly rather than discarding it.
    state_ = PS2State::WaitingForAck;
    send_byte(static_cast<uint8_t>(entry.command));
}

// Process a response byte from the keyboard.
// Returns true if the byte was consumed as a command response.
bool KeyboardDriver::process_response(uint8_t data) {
    switch (state_) {
        case PS2State::Idle:
            // Not expecting any response; this is a scancode.
            return false;

        case PS2State::WaitingForAck:
            if (data == kPS2Ack) {
                // Command acknowledged.
                PS2CommandEntry &entry = command_queue_[queue_head_];

                if (entry.has_parameter) {
                    // Clear the flag first: the next ACK (for the parameter)
                    // must complete the command, not re-send the parameter.
                    entry.has_parameter = false;
                    send_byte(entry.parameter);
                    return true;
                }

                // No parameter; command complete.
                queue_head_ = (queue_head_ + 1) % kKeyboardCommandQueueSize;
                --queue_count_;
                state_ = PS2State::Idle;

                // Process next command if any.
                if (queue_count_ > 0) {
                    process_command_queue();
                }
                return true;
            } else if (data == kPS2Resend) {
                // Resend requested.
                PS2CommandEntry &entry = command_queue_[queue_head_];
                ++entry.retry_count;

                if (entry.retry_count > kMaxRetries) {
                    // Max retries exceeded; drop the command.
                    queue_head_ = (queue_head_ + 1) % kKeyboardCommandQueueSize;
                    --queue_count_;
                    state_ = PS2State::Idle;

                    if (queue_count_ > 0) {
                        process_command_queue();
                    }
                } else {
                    // Retry the command.
                    process_command_queue();
                }
                return true;
            }
            // Received something that is neither ACK nor RESEND while expecting
            // a response. This should never happen with a well-behaved
            // keyboard and signals a state-machine or protocol error.
            assert(false
                   && "KeyboardDriver: unexpected byte while WaitingForAck");
            return false;

        case PS2State::WaitingForData:
            // For commands that expect data responses (like identify).
            // For now, just complete the command.
            queue_head_ = (queue_head_ + 1) % kKeyboardCommandQueueSize;
            --queue_count_;
            state_ = PS2State::Idle;

            if (queue_count_ > 0) {
                process_command_queue();
            }
            return true;
    }

    assert(false && "KeyboardDriver::process_response: unknown PS2State");
    return false;
}
