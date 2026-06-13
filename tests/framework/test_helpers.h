#pragma once

#include <cstdint>

// Helper to declare that a byte was written at an address via MMIO
inline void mmio_write_byte(uint32_t addr, uint8_t val) {
    extern void io_write_b(uint32_t, uint8_t);
    io_write_b(addr, val);
}

inline uint16_t mmio_read_word(uint32_t addr)  {
    extern uint16_t io_read(uint32_t);
    return io_read(addr);
}

// Write a 16-bit big-endian instruction word to emulator RAM at given address.
inline void write_instruction_at(uint32_t addr, uint16_t opcode);
