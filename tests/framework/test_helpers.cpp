#include "test_helpers.h"

// CPU register mock state
static int32_t   s_dreg[8]   = {0};
static uint32_t  s_areg[8]   = {0};
static uint32_t  s_pc        = 0;
static uint16_t  s_sr        = 0;
static int       s_cycles    = 0;

void  test_cpu_set_dreg(int idx, int32_t val) {
  if (idx >= 0 && idx < 8) s_dreg[idx] = val;
}
int32_t test_cpu_dreg(int idx) {
  if (idx >= 0 && idx < 8) return s_dreg[idx];
  return 0;
}

void  test_cpu_set_areg(int idx, uint32_t val) {
  if (idx >= 0 && idx < 8) s_areg[idx] = val;
}
uint32_t test_cpu_areg(int idx) {
  if (idx >= 0 && idx < 8) return s_areg[idx];
  return 0;
}

void  test_cpu_set_pc(uint32_t pc) { s_pc = pc; }
uint32_t test_cpu_pc(void)         { return s_pc; }

void  test_cpu_set_sr(uint16_t sr) { s_sr = sr; }
uint16_t test_cpu_sr(void)         { return s_sr; }

void   test_cycles_add(int cycles)   { s_cycles += cycles; }
int    test_cycles(void)             { return s_cycles; }
void   test_cycles_reset(void)       { s_cycles = 0; }
