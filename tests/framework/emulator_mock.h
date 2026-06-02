#pragma once
#ifndef TEST_EMULATOR_MOCK_H
#define TEST_EMULATOR_MOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Emulator mock: Provides stub global variables and functions that the
// Steem SSE source code expects, allowing compilation and linking.
// Call mock_init() before running tests, mock_cleanup() after.

// --- Memory mocks ---
// Minimal mock memory for CPU tests (1MB ST RAM + 64KB ROM)
#define MOCK_ROM_SIZE (64 * 1024)
#define MOCK_RAM_SIZE (1 * 1024 * 1024)
#define MOCK_MEM_SIZE (MOCK_ROM_SIZE + MOCK_RAM_SIZE)

extern uint8_t* MockRom;
extern uint8_t* MockRam;

void mock_memory_init(void);
void mock_memory_cleanup(void);

// --- Platform stubs ---
void mock_platform_stub_init(void);

// --- Global init/cleanup ---
void mock_init(void);
void mock_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_EMULATOR_MOCK_H
