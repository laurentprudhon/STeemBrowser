#pragma once
#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "gtest/gtest.h"
#include <stdint.h>

// Helper macros for CPU register assertions
#define EXPECT_DREG(idx, val) \
  EXPECT_EQ((val), test_cpu_dreg(idx))

#define EXPECT_AREG(idx, val) \
  EXPECT_EQ((val), test_cpu_areg(idx))

#define EXPECT_PC(val) \
  EXPECT_EQ((val), test_cpu_pc())

#define EXPECT_SR(val) \
  EXPECT_EQ((val), test_cpu_sr())

#ifdef __cplusplus
extern "C" {
#endif

// CPU state access helpers (set by mock before running a code fragment)
void  test_cpu_set_dreg(int idx, int32_t val);
int32_t test_cpu_dreg(int idx);
void  test_cpu_set_areg(int idx, uint32_t val);
uint32_t test_cpu_areg(int idx);
void  test_cpu_set_pc(uint32_t pc);
uint32_t test_cpu_pc(void);
void  test_cpu_set_sr(uint16_t sr);
uint16_t test_cpu_sr(void);

// Cycle counter
void   test_cycles_add(int cycles);
int    test_cycles(void);
void   test_cycles_reset(void);
#define ASSERT_CYCLES(expected) ASSERT_EQ((expected), test_cycles())
#define EXPECT_CYCLE_LEQ(expected) EXPECT_LE(test_cycles(), (expected))

#ifdef __cplusplus
}
#endif

#endif // TEST_HELPERS_H
