#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ring_buffer.h"

// PS/2 Controller port addresses
static constexpr uint16_t kPS2DataPort = 0x60;
static constexpr uint16_t kPS2StatusPort = 0x64;

// PS/2 Status register bits
static constexpr uint8_t kPS2StatusOutputFull = 0x01;
static constexpr uint8_t kPS2StatusInputFull = 0x02;

// PS/2 Keyboard commands (sent to data port)
enum class PS2Command : uint8_t {
  SetLEDs = 0xED,
  Echo = 0xEE,
  GetScanCodeSet = 0xF0,
  Identify = 0xF2,
  SetTypematicRate = 0x3,
  EnableScanning = 0xF4,
  DisableScanning = 0xF5,
  SetDefaults = 0xF6,
  Resend = 0xFE,
  Reset = 0xFF,
};

// PS/2 Keyboard response codes (received from data port)
static constexpr uint8_t kPS2Ack = 0xFA;
static constexpr uint8_t kPS2Resend = 0xFE;
static constexpr uint8_t kPS2Echo = 0xEE;
static constexpr uint8_t kPS2TestPass = 0xAA;
static constexpr uint8_t kPS2Break = 0xF0;

// Maximum pending commands in queue
static constexpr size_t kPS2CommandQueueSize = 8;

/**
 * Entry in the PS/2 command queue.
 * Contains command byte, optional parameter, and retry tracking.
 */
struct PS2CommandEntry {
  PS2Command command;
  uint8_t parameter = 0;
  bool has_parameter = false;
  int retry_count = 0;
};

/**
 * State machine states for PS/2 command protocol.
 * Idle -> WaitingForAck (-> WaitingForData) -> Idle
 */
enum class PS2State : uint8_t {
  Idle,
  WaitingForAck,
  WaitingForData,
};

/**
 * Manages PS/2 keyboard command protocol.
 *
 * The PS/2 protocol requires sending commands and waiting for ACK (0xFA)
 * responses. Some commands require parameter bytes and may request resends
 * via 0xFE. This class handles the state machine and queuing.
 */
class PS2CommandQueue {
 public:
  PS2CommandQueue();
  ~PS2CommandQueue() = default;

  PS2CommandQueue(const PS2CommandQueue&) = delete;
  PS2CommandQueue& operator=(const PS2CommandQueue&) = delete;

  /**
   * Queues a command without parameters.
   * Returns true if queued, false if queue is full.
   */
  [[nodiscard]]
  bool send_command(PS2Command command);

  /**
   * Queues a command with a parameter byte.
   * Returns true if queued, false if queue is full.
   */
  [[nodiscard]]
  bool send_command_with_param(PS2Command command, uint8_t parameter);

  /**
   * Returns true when no commands are pending and state machine is idle.
   * Use this to wait for command completion before continuing.
   */
  [[nodiscard]]
  bool is_idle() const;

  /**
   * Clears all pending commands and resets state machine.
   */
  void flush();

  /**
   * Processes a byte from the keyboard hardware.
   *
   * Must be called for every byte received from the keyboard before
   * interpreting it as a scancode. Returns true if the byte was
   * consumed as a command response (ACK, Resend, etc.).
   *
   * Returns true if consumed as command response, false if it's a scancode.
   */
  [[nodiscard]]
  bool process_byte(uint8_t data);

 private:
  /**
   * Sends next command in queue. Assumes queue is non-empty.
   * Sets state to WaitingForAck before sending to avoid race with IRQ.
   */
  void process_next();

  /**
   * Sends byte to keyboard controller data port.
   * Spins until input buffer is ready.
   */
  void send_byte(uint8_t data);

  /**
   * Returns true when keyboard controller input buffer is empty.
   */
  [[nodiscard]]
  bool can_send() const;

  RingBuffer<PS2CommandEntry, kPS2CommandQueueSize> queue_;
  PS2State state_;
  static constexpr int kMaxRetries = 3;
};
