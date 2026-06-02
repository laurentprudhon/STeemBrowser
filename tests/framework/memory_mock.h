#pragma once
#ifndef TEST_MEMORY_MOCK_H
#define TEST_MEMORY_MOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Mock 64 KiB ROM + configurable RAM
void mock_memory_init(void);
void mock_memory_cleanup(void);

// Access helpers (mirrors PEEK/Poke semantics)
uint8_t  mock_peek(uint32_t addr);
uint16_t mock_dpeek(uint32_t addr);
uint32_t mock_lpeek(uint32_t addr);
void     mock_poke(uint32_t addr, uint8_t val);
void     mock_dpoke(uint32_t addr, uint16_t val);
void     mock_lpoke(uint32_t addr, uint32_t val);

#ifdef __cplusplus
}
#endif

#endif // TEST_MEMORY_MOCK_H
