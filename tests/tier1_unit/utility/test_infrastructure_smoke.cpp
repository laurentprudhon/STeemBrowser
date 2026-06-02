#include "gtest/gtest.h"
#include "emulator_mock.h"
#include "memory_mock.h"
#include "agenda_mock.h"
#include "test_helpers.h"

// Basic mock_init/mock_cleanup
TEST(SmokeTest, MockInitCleanup) {
  mock_init();
  mock_cleanup();
}

// Memory mock round-trip
TEST(SmokeTest, MemoryPeekPoke) {
  mock_memory_init();
  mock_poke(0x10000, 0xAB);
  EXPECT_EQ(0xAB, mock_peek(0x10000));
  mock_dpoke(0x10010, 0xCDAB);
  EXPECT_EQ(0xCDAB, mock_dpeek(0x10010));
  mock_lpoke(0x10020, 0x12345678);
  EXPECT_EQ(0x12345678, mock_lpeek(0x10020));
}

// Agenda mock add and query
TEST(SmokeTest, AgendaAddAndQuery) {
  mock_agenda_init();
  mock_agenda_add(NULL, (int)100, 42);
  EXPECT_EQ(1, g_agenda_length);
  EXPECT_EQ((uint32_t)100, g_agenda[0].time);
  EXPECT_EQ(42, g_agenda[0].param);
}

// CPU register helpers
TEST(SmokeTest, CpuRegisterHelpers) {
  test_cpu_set_dreg(0, 0x12345678);
  EXPECT_EQ(0x12345678, test_cpu_dreg(0));
  test_cpu_set_areg(7, 0x000FFFFF);
  EXPECT_EQ(0x000FFFFF, test_cpu_areg(7));
  test_cpu_set_pc(0x100);
  EXPECT_EQ(0x100, test_cpu_pc());
  test_cpu_set_sr(0x2000);
  EXPECT_EQ(0x2000, test_cpu_sr());
}

// Cycle counter
TEST(SmokeTest, CycleCounter) {
  test_cycles_reset();
  test_cycles_add(4);
  test_cycles_add(8);
  EXPECT_EQ(12, test_cycles());
}

TEST(SmokeTest, InfrastructureBuilds) {
  EXPECT_TRUE(true);
}
