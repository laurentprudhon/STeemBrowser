#include "memory_mock.h"
#include <string.h>

static uint8_t s_rom[64 * 1024];
static uint8_t s_ram[1024 * 1024];

void mock_memory_init(void) {
  // Zero out memory
  memset(s_rom, 0, sizeof(s_rom));
  memset(s_ram, 0, sizeof(s_ram));
}

void mock_memory_cleanup(void) {
  // Nothing to free; stack-allocated or static
}

uint8_t mock_peek(uint32_t addr) {
  if (addr < sizeof(s_rom)) return s_rom[addr];
  if (addr < sizeof(s_rom) + sizeof(s_ram)) return s_ram[addr - sizeof(s_rom)];
  return 0;
}

uint16_t mock_dpeek(uint32_t addr) {
  return (uint16_t)(mock_peek(addr) | (mock_peek(addr + 1) << 8));
}

uint32_t mock_lpeek(uint32_t addr) {
  return (uint32_t)(mock_peek(addr)
                  | (mock_peek(addr + 1) << 8)
                  | (mock_peek(addr + 2) << 16)
                  | (mock_peek(addr + 3) << 24));
}

void mock_poke(uint32_t addr, uint8_t val) {
  if (addr >= sizeof(s_rom) && addr < sizeof(s_rom) + sizeof(s_ram)) {
    s_ram[addr - sizeof(s_rom)] = val;
  }
}

void mock_dpoke(uint32_t addr, uint16_t val) {
  mock_poke(addr, (uint8_t)(val & 0xFF));
  mock_poke(addr + 1, (uint8_t)((val >> 8) & 0xFF));
}

void mock_lpoke(uint32_t addr, uint32_t val) {
  mock_poke(addr, (uint8_t)(val & 0xFF));
  mock_poke(addr + 1, (uint8_t)((val >> 8) & 0xFF));
  mock_poke(addr + 2, (uint8_t)((val >> 16) & 0xFF));
  mock_poke(addr + 3, (uint8_t)((val >> 24) & 0xFF));
}
