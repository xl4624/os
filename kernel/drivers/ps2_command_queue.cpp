#include "ps2_command_queue.h"

#include <sys/io.h>

PS2CommandQueue::PS2CommandQueue() = default;

bool PS2CommandQueue::send_command(PS2Command command) {
  PS2CommandEntry entry{/*command=*/command, /*parameter=*/0,
                        /*has_parameter=*/false, /*retry_count=*/0};
  if (!queue_.push(entry)) {
    return false;
  }
  if (state_ == PS2State::Idle) {
    process_next();
  }
  return true;
}

bool PS2CommandQueue::send_command_with_param(PS2Command command, uint8_t parameter) {
  PS2CommandEntry entry{/*command=*/command, /*parameter=*/parameter,
                        /*has_parameter=*/true, /*retry_count=*/0};
  if (!queue_.push(entry)) {
    return false;
  }
  if (state_ == PS2State::Idle) {
    process_next();
  }
  return true;
}

bool PS2CommandQueue::is_idle() const { return queue_.is_empty() && state_ == PS2State::Idle; }

void PS2CommandQueue::flush() {
  queue_.clear();
  state_ = PS2State::Idle;
}

bool PS2CommandQueue::process_byte(uint8_t data) {
  switch (state_) {
    case PS2State::Idle: {
      return false;
    }

    case PS2State::WaitingForAck: {
      if (data == kPS2Ack) {
        PS2CommandEntry entry{};
        if (!queue_.peek(entry)) {
          state_ = PS2State::Idle;
          return true;
        }

        if (entry.has_parameter) {
          // Send parameter and wait for another ACK
          send_byte(entry.parameter);
          return true;
        }

        // Command complete
        static_cast<void>(queue_.pop(entry));
        state_ = PS2State::Idle;

        if (!queue_.is_empty()) {
          process_next();
        }
        return true;
      } else if (data == kPS2Resend) {
        PS2CommandEntry entry{};
        if (!queue_.peek(entry)) {
          state_ = PS2State::Idle;
          return true;
        }

        ++entry.retry_count;
        if (entry.retry_count > kMaxRetries) {
          static_cast<void>(queue_.pop(entry));
          state_ = PS2State::Idle;
          if (!queue_.is_empty()) {
            process_next();
          }
        } else {
          // Retry: update entry and resend
          static_cast<void>(queue_.pop(entry));
          static_cast<void>(queue_.push(entry));
          process_next();
        }
        return true;
      }
      return false;
    }

    case PS2State::WaitingForData: {
      // For commands that expect data responses
      PS2CommandEntry entry{};
      static_cast<void>(queue_.pop(entry));
      state_ = PS2State::Idle;
      if (!queue_.is_empty()) {
        process_next();
      }
      return true;
    }
  }

  return false;
}

void PS2CommandQueue::process_next() {
  if (queue_.is_empty()) {
    state_ = PS2State::Idle;
    return;
  }

  PS2CommandEntry entry{};
  if (!queue_.peek(entry)) {
    return;
  }

  send_byte(static_cast<uint8_t>(entry.command));
  state_ = PS2State::WaitingForAck;
}

void PS2CommandQueue::send_byte(uint8_t data) {
  while (!can_send()) {
    // Spin-wait for input buffer to be empty
  }
  outb(kPS2DataPort, data);
}

bool PS2CommandQueue::can_send() const { return (inb(kPS2StatusPort) & kPS2StatusInputFull) == 0; }
