#include "ata.h"

#include <sys/io.h>

// Primary ATA channel I/O ports.
static constexpr uint16_t kRegData = 0x1F0;
static constexpr uint16_t kRegSecCnt = 0x1F2;
static constexpr uint16_t kRegLbaLo = 0x1F3;
static constexpr uint16_t kRegLbaMid = 0x1F4;
static constexpr uint16_t kRegLbaHi = 0x1F5;
static constexpr uint16_t kRegDrvHead = 0x1F6;
static constexpr uint16_t kRegCmd = 0x1F7;      // write: command; read: status
static constexpr uint16_t kRegControl = 0x3F6;  // write: device control; read: alt status

// Status register bits.
static constexpr uint8_t kBSY = 0x80;
static constexpr uint8_t kDRQ = 0x08;
static constexpr uint8_t kERR = 0x01;

// ATA commands.
static constexpr uint8_t kCmdReadSectors = 0x20;
static constexpr uint8_t kCmdWriteSectors = 0x30;
static constexpr uint8_t kCmdFlushCache = 0xE7;
static constexpr uint8_t kCmdIdentify = 0xEC;

namespace {

bool s_present = false;
uint32_t s_sectors = 0;

// Spin until BSY clears. Returns false if the error bit is set.
bool wait_bsy() {
  uint8_t status;
  do {
    status = inb(kRegCmd);
  } while ((status & kBSY) != 0);
  return (status & kERR) == 0;
}

// Spin until DRQ is set. Returns false if ERR is set.
[[nodiscard]] bool wait_drq() {
  for (;;) {
    const uint8_t status = inb(kRegCmd);
    if ((status & kERR) != 0) {
      return false;
    }
    if ((status & kDRQ) != 0) {
      return true;
    }
  }
}

// Program the LBA28 registers and issue a single-sector command to the master drive.
void issue_lba28(uint32_t lba, uint8_t cmd) {
  outb(kRegControl, 0x02);  // nIEN: disable interrupt delivery
  outb(kRegDrvHead, static_cast<uint8_t>(0xE0 | ((lba >> 24) & 0x0F)));
  outb(kRegSecCnt, 1);
  outb(kRegLbaLo, static_cast<uint8_t>(lba));
  outb(kRegLbaMid, static_cast<uint8_t>(lba >> 8));
  outb(kRegLbaHi, static_cast<uint8_t>(lba >> 16));
  outb(kRegCmd, cmd);
}

}  // namespace

namespace Ata {

void init() {
  // Select master drive and disable interrupts.
  outb(kRegControl, 0x02);
  outb(kRegDrvHead, 0xA0);
  if (!wait_bsy()) {
    return;
  }

  // Issue IDENTIFY. A status of 0x00 or 0xFF means no drive.
  outb(kRegCmd, kCmdIdentify);
  const uint8_t status = inb(kRegCmd);
  if (status == 0x00 || status == 0xFF) {
    return;
  }

  if (!wait_drq()) {
    return;
  }

  // Read 256 16-bit words of IDENTIFY data.
  uint16_t id[256];
  for (unsigned short& i : id) {
    i = inw(kRegData);
  }

  // Words 60-61 hold the 28-bit LBA sector count (little-endian on x86).
  s_sectors = (static_cast<uint32_t>(id[61]) << 16) | id[60];
  s_present = true;
}

bool is_present() { return s_present; }

uint32_t sector_count() { return s_sectors; }

bool read_sector(uint32_t lba, uint8_t buf[512]) {
  if (!s_present) {
    return false;
  }
  wait_bsy();
  issue_lba28(lba, kCmdReadSectors);
  if (!wait_drq()) {
    return false;
  }

  auto* words = reinterpret_cast<uint16_t*>(buf);
  for (int i = 0; i < 256; ++i) {
    words[i] = inw(kRegData);
  }
  return true;
}

bool write_sector(uint32_t lba, const uint8_t buf[512]) {
  if (!s_present) {
    return false;
  }
  wait_bsy();
  issue_lba28(lba, kCmdWriteSectors);
  if (!wait_drq()) {
    return false;
  }

  const auto* words = reinterpret_cast<const uint16_t*>(buf);
  for (int i = 0; i < 256; ++i) {
    outw(kRegData, words[i]);
  }

  // Flush write cache to ensure data is committed to the medium.
  outb(kRegCmd, kCmdFlushCache);
  wait_bsy();
  return true;
}

}  // namespace Ata
