#include "test_fixture.h"
#include "test_helpers.h"
#include <cstring>

// cpuinit.h declares m68ki_initialized and related state; we need the
// CPU register alias pointers that cpu.cpp sets up.
extern void SetTimingFunctions();
extern uint32_t cpureg[16];
extern uint32_t pc;
extern uint16_t SR;

// The bus-access function pointers are set by SetTimingFunctions().
extern void (*pBusReadB)();
extern void (*pBusWriteB)();

// Memory access functions from ior.cpp/iow.cpp (declared in device_map.h)
extern void io_write_b(uint32_t addr, uint8_t val);
extern uint16_t io_read(uint32_t addr);

// ---- TestEmulatorEnvironment implementation ----

void TestEmulatorEnvironment::SetUp() {
    // Not much to do here: global chips are statically constructed in
    // steem_test_core.a. Each test that wants clean state should call
    // reset_cpu_only() or power_on().
}

void TestEmulatorEnvironment::TearDown() {
    // Nothing special — globals will be reused by next test, so each
    // test that cares about state must reset in its own SetUp() body.
}

void TestEmulatorEnvironment::set_register(int n, uint32_t val) {
    if (n >= 0 && n < 16)
        cpureg[n] = val;
}

uint32_t TestEmulatorEnvironment::reg(int n) const {
    if (n >= 0 && n < 16)
        return cpureg[n];
    return 0;
}

uint16_t TestEmulatorEnvironment::sr() const {
    return SR;
}

void TestEmulatorEnvironment::execute_one() {
    m68kProcess();
}

void TestEmulatorEnvironment::execute_n(uint32_t /*cycles*/) {
    // For now just one iteration — the cycle budget logic requires
    // a working clock which needs timing setup. We'll add it later.
    m68kProcess();
}

void TestEmulatorEnvironment::write_byte(uint32_t addr, uint8_t val) {
    io_write_b(addr, val);
}

uint8_t TestEmulatorEnvironment::read_byte(uint32_t addr) const {
    // io_read returns WORD, we take low byte.
    return static_cast<uint8_t>(io_read(addr));
}

void TestEmulatorEnvironment::write_instruction_at(uint32_t addr, uint16_t opcode) {
    // Write big-endian: high byte first, then low byte.
    io_write_b(addr, static_cast<uint8_t>((opcode >> 8) & 0xFF));
    io_write_b(addr + 1, static_cast<uint8_t>(opcode & 0xFF));
}

uint16_t TestEmulatorEnvironment::read_instruction_at(uint32_t addr) const {
    uint8_t hi = static_cast<uint8_t>(io_read(addr));
    uint8_t lo = static_cast<uint8_t>(io_read(addr + 1));
    return (hi << 8) | lo;
}

void TestEmulatorEnvironment::reset_cpu_only() {
    // We delegate to the real reset path.  reset_peripherals(false) == cold.
    if (pBusReadB)
        SetTimingFunctions();
    reset_peripherals(true);
}

// ---- free helper for test_helpers.h ----

void write_instruction_at(uint32_t addr, uint16_t opcode) {
    io_write_b(addr, static_cast<uint8_t>((opcode >> 8) & 0xFF));
    io_write_b(addr + 1, static_cast<uint8_t>(opcode & 0xFF));
}
