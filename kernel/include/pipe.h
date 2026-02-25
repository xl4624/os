#pragma once

#include <stdint.h>

#include "ring_buffer.h"

static constexpr uint32_t kPipeBufferSize = 4096;

struct Pipe {
  RingBuffer<char, kPipeBufferSize> buffer;
  uint32_t readers;  // number of open read-end FileDescriptions
  uint32_t writers;  // number of open write-end FileDescriptions

  Pipe() : readers(0), writers(0) {}
};

// Read up to count bytes from the pipe buffer into buf.
// Returns bytes read, 0 for EOF (no writers), or kSyscallRestart to block.
int32_t pipe_read(Pipe* pipe, uint8_t* buf, uint32_t count);

// Write up to count bytes from buf into the pipe buffer.
// Returns bytes written, -1 for broken pipe (no readers), or kSyscallRestart.
int32_t pipe_write(Pipe* pipe, const uint8_t* buf, uint32_t count);

// Called when a read-end FileDescription is freed. Decrements readers;
// frees the Pipe if both readers and writers are zero.
void pipe_close_read(Pipe* pipe);

// Called when a write-end FileDescription is freed. Decrements writers;
// frees the Pipe if both readers and writers are zero.
void pipe_close_write(Pipe* pipe);
