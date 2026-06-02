#include "emulator_mock.h"
#include "memory_mock.h"
#include "agenda_mock.h"

// Minimal mock globals needed to link against emulator code.
// These are the extern symbols declared in computer.h, emulator.h, cpu.h, etc.

// -- Memory pointers --
uint8_t* MockRom  = NULL;
uint8_t* MockRam  = NULL;

// -- Platform stubs --
void mock_platform_stub_init(void) {
  // Nothing needed on Linux
}

// -- Global init/cleanup --
void mock_init(void) {
  mock_memory_init();
  mock_agenda_init();
}

void mock_cleanup(void) {
  mock_agenda_cleanup();
  mock_memory_cleanup();
}
