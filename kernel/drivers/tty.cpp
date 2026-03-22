#include "tty.h"

#include <sys/cdefs.h>
#include <sys/io.h>

#include "terminal.h"

static constexpr uint16_t kDebugconPort = 0xE9;

__BEGIN_DECLS

bool tty_debugcon_enabled = true;

void terminal_putchar(char c) {
  kTerminal.write_char(c);
  if (tty_debugcon_enabled) {
    outb(kDebugconPort, static_cast<uint8_t>(c));
  }
}

void terminal_clear(void) { kTerminal.clear(); }

void terminal_set_position(unsigned int row, unsigned int col) { kTerminal.set_position(row, col); }

void terminal_set_color(unsigned char color) { kTerminal.set_color(color); }

__END_DECLS

void terminal_write(const char* data) { kTerminal.write(data); }

void terminal_write(std::span<const char> data) { kTerminal.write(data); }
