cmake_minimum_required(VERSION 3.16)
project(steem_tests LANGUAGES CXX C)

include(CTest)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Paths
set(STEEM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../steemsse-V4.2.0_R10")
set(STEEM_STEEM_DIR  "${STEEM_SOURCE_DIR}/steem")
set(STEEM_INCLUDE_DIR "${STEEM_SOURCE_DIR}/include")
set(STEEM_3RDPARTY   "${STEEM_SOURCE_DIR}/3rdparty")

# ---- steem_test_core: ENTIRE emulator as one static library ----
add_library(steem_test_core STATIC

  # CPU
  "${STEEM_STEEM_DIR}/cpu.cpp"
  "${STEEM_STEEM_DIR}/cpu_ea.cpp"
  "${STEEM_STEEM_DIR}/cpu_op.cpp"
  "${STEEM_STEEM_DIR}/cpuinit.cpp"

  # Memory / MMU
  "${STEEM_STEEM_DIR}/mmu.cpp"
  "${STEEM_STEEM_DIR}/ior.cpp"
  "${STEEM_STEEM_DIR}/iow.cpp"

  # Chips
  "${STEEM_STEEM_DIR}/glue.cpp"
  "${STEEM_STEEM_DIR}/shifter.cpp"
  "${STEEM_STEEM_DIR}/dma.cpp"
  "${STEEM_STEEM_DIR}/blitter.cpp"
  "${STEEM_STEEM_DIR}/mfp.cpp"
  "${STEEM_STEEM_DIR}/psg.cpp"
  "${STEEM_STEEM_DIR}/ikbd.cpp"

  # Disk / Floppy
  "${STEEM_STEEM_DIR}/fdc.cpp"
  "${STEEM_STEEM_DIR}/floppy_drive.cpp"
  "${STEEM_STEEM_DIR}/floppy_disk.cpp"
  "${STEEM_STEEM_DIR}/disk_stw.cpp"
  "${STEEM_STEEM_DIR}/disk_scp.cpp"
  "${STEEM_STEEM_DIR}/disk_hfe.cpp"
  "${STEEM_STEEM_DIR}/disk_ghost.cpp"

  # I/O peripherals
  "${STEEM_STEEM_DIR}/acia.cpp"
  "${STEEM_STEEM_DIR}/rs232.cpp"
  "${STEEM_STEEM_DIR}/acsi.cpp"

  # Core / Runtime
  "${STEEM_STEEM_DIR}/emulator.cpp"
  "${STEEM_STEEM_DIR}/reset.cpp"
  "${STEEM_STEEM_DIR}/run.cpp"
  "${STEEM_STEEM_DIR}/computer.cpp"
  "${STEEM_STEEM_DIR}/tos.cpp"
  "${STEEM_STEEM_DIR}/loadsave.cpp"
  "${STEEM_STEEM_DIR}/loadsave_emu.cpp"

  # Utility
  "${STEEM_STEEM_DIR}/acc.cpp"
  "${STEEM_STEEM_DIR}/macros.cpp"
  "${STEEM_STEEM_DIR}/historylist.cpp"
  "${STEEM_STEEM_DIR}/dataloadsave.cpp"
  "${STEEM_STEEM_DIR}/palette.cpp"
  "${STEEM_STEEM_DIR}/archive.cpp"
  "${STEEM_STEEM_DIR}/stports.cpp"
  "${STEEM_STEEM_DIR}/midi.cpp"

  # Options / OSD / Debug infrastructure
  "${STEEM_STEEM_DIR}/options.cpp"
  "${STEEM_STEEM_DIR}/debug.cpp"
  "${STEEM_STEEM_DIR}/osd.cpp"
  "${STEEM_STEEM_DIR}/d2.cpp"
  "${STEEM_STEEM_DIR}/iolist.cpp"
  "${STEEM_STEEM_DIR}/key_table.cpp"
  "${STEEM_INCLUDE_DIR}/wordwrapper.cpp"

  # Draw / video output (C draw code, no X11)
  "${STEEM_STEEM_DIR}/draw.cpp"
  "${STEEM_STEEM_DIR}/code/draw_c/draw_c.cpp"

  # Disk management / Hard disk
  "${STEEM_STEEM_DIR}/hd_gemdos.cpp"
  "${STEEM_STEEM_DIR}/harddiskman.cpp"
  "${STEEM_STEEM_DIR}/diskman.cpp"
  "${STEEM_STEEM_DIR}/diskman_diags.cpp"

  # GUI dialogs -- defines SSEOptions, OptionBox, InfoBox etc.
  "${STEEM_STEEM_DIR}/gui_controls.cpp"
  "${STEEM_STEEM_DIR}/gui.cpp"
  "${STEEM_STEEM_DIR}/stemwin.cpp"
  "${STEEM_STEEM_DIR}/infobox.cpp"
  "${STEEM_STEEM_DIR}/shortcutbox.cpp"
  "${STEEM_STEEM_DIR}/patchesbox.cpp"
  "${STEEM_STEEM_DIR}/mem_browser.cpp"
  "${STEEM_STEEM_DIR}/mr_static.cpp"
  "${STEEM_STEEM_DIR}/stemdialogs.cpp"
  "${STEEM_STEEM_DIR}/dwin_edit.cpp"

  # Joystick / notification / misc GUI
  "${STEEM_STEEM_DIR}/stjoy.cpp"
  "${STEEM_STEEM_DIR}/notifyinit.cpp"
  "${STEEM_STEEM_DIR}/options_create.cpp"
  "${STEEM_STEEM_DIR}/printer.cpp"
  "${STEEM_STEEM_DIR}/translate.cpp"

  # Include/ utility files
  "${STEEM_INCLUDE_DIR}/easystr.cpp"
  "${STEEM_INCLUDE_DIR}/easystringlist.cpp"
  "${STEEM_INCLUDE_DIR}/easycompress.cpp"
  "${STEEM_INCLUDE_DIR}/circularbuffer.cpp"
  "${STEEM_INCLUDE_DIR}/mymisc.cpp"
  "${STEEM_INCLUDE_DIR}/notwin_mymisc.cpp"
  "${STEEM_INCLUDE_DIR}/portio.cpp"
  "${STEEM_INCLUDE_DIR}/di_get_contents.cpp"
  "${STEEM_INCLUDE_DIR}/dirsearch.cpp"
  "${STEEM_INCLUDE_DIR}/configstorefile.cpp"

  # X-platform UI widgets -- hxc.cpp includes all sub-widgets via internal
  # #include directives, so we only list hxc.cpp directly.
  "${STEEM_INCLUDE_DIR}/x/hxc.cpp"
  "${STEEM_INCLUDE_DIR}/x/hxc_dir_lv.cpp"
  "${STEEM_INCLUDE_DIR}/x/x_mymisc.cpp"

  # X-platform port IO (defines TPortIO methods)
  "${STEEM_INCLUDE_DIR}/x/x_portio.cpp"

  # 3rdparty C / C++ sources
  "${STEEM_3RDPARTY}/pasti/div68kCycleAccurate.c"
  "${STEEM_3RDPARTY}/6301/6301.c"
  "${STEEM_3RDPARTY}/dsp/dsp.cpp"
  "${STEEM_3RDPARTY}/dsp/FIR-filter-class/filt.cpp"

  # zlib (compiled into our lib)
  "${STEEM_3RDPARTY}/zlib/crc32.c"
  "${STEEM_3RDPARTY}/zlib/inflate.c"
  "${STEEM_3RDPARTY}/zlib/adler32.c"
  "${STEEM_3RDPARTY}/zlib/trees.c"
  "${STEEM_3RDPARTY}/zlib/inffast.c"
  "${STEEM_3RDPARTY}/zlib/inftrees.c"
  "${STEEM_3RDPARTY}/zlib/deflate.c"
  "${STEEM_3RDPARTY}/zlib/zutil.c"
  "${STEEM_3RDPARTY}/zlib/compress.c"

  # minizip
  "${STEEM_3RDPARTY}/zlib/contrib/minizip/ioapi.c"
  "${STEEM_3RDPARTY}/zlib/contrib/minizip/unzip.c"

  # CapsPlug (wrapper around libcapsimage library)
  "${STEEM_3RDPARTY}/caps/CapsPlug.cpp"
)

# Include directories: mirror Makefile.txt exactly
target_include_directories(steem_test_core PRIVATE
    ${STEEM_INCLUDE_DIR}
    ${STEEM_STEEM_DIR}/headers
    ${STEEM_3RDPARTY}
    ${STEEM_3RDPARTY}/zlib
    ${STEEM_3RDPARTY}/zlib/contrib/minizip
    ${STEEM_3RDPARTY}/rtaudio
    ${STEEM_3RDPARTY}/6301
    ${STEEM_3RDPARTY}/dsp
    ${STEEM_3RDPARTY}/dsp/FIR-filter-class
    ${STEEM_STEEM_DIR}/code
    ${STEEM_STEEM_DIR}/code/x
)

# Mirror Makefile.txt flags, minus -Wfatal-errors which causes false failures.
target_compile_options(steem_test_core PRIVATE
    -w
    -fpermissive
    -O2
    -std=gnu++11
)
set_target_properties(steem_test_core PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 11
)

# Definitions: mirror production, plus test-friendly guards
target_compile_definitions(steem_test_core PRIVATE
    UNIX
    LINUX
    SSE_DRAW_C
    SSE_RELEASE
    NO_DEBUG_BUILD
    NDEBUG
    # x86_64: required for easystr.cpp Empty_Text array size
    SSE_X64
    # Disable optional features to reduce deps
    SSE_NO_SCREENSAVER
    SSE_NO_UPDATE
    SSE_LEAN_AND_MEAN
    SSE_UNIX_STATIC_VAR_INIT
    SSE_NO_JOYSTICK_MM
    SSE_NO_INTERNAL_SPEAKER
    # Audio backends fully disabled for test build
    NO_PORTAUDIO
    NO_RTAUDIO
    SSE_NO_AUDIO
    # Disable X11 extensions we don't need
    NO_SHM
    NO_XVIDMODE
    TEST_BUILD
)

target_link_libraries(steem_test_core PUBLIC pthread X11 Xext Xxf86vm pulse-simple pulse-mainloop-glib)

# ---- platform_stub: thin stub layer for platform APIs ----
add_library(platform_stub STATIC
    framework/platform_stub.cpp
)

target_include_directories(platform_stub PRIVATE
    ${STEEM_INCLUDE_DIR}
    ${STEEM_STEEM_DIR}/headers
    ${STEEM_3RDPARTY}
    ${STEEM_STEEM_DIR}/code
    ${STEEM_STEEM_DIR}/code/x
)

target_compile_definitions(platform_stub PRIVATE
    UNIX
    LINUX
    TEST_BUILD
    NO_PORTAUDIO
    NO_RTAUDIO
    SSE_NO_AUDIO
)

target_compile_options(platform_stub PRIVATE
    -w
    -fpermissive
    -O2
    -std=gnu++11
)

# ---- Test helper library ----

add_library(test_helpers STATIC
    framework/test_helpers.cpp
)

target_include_directories(test_helpers PUBLIC framework/)

target_link_libraries(test_helpers
    PUBLIC GTest::gtest steem_test_core platform_stub
)

target_compile_options(test_helpers PRIVATE
    -w
    -fpermissive
    -O2
    -std=gnu++11
)

# ---- Subdirectories with actual test targets ----

add_subdirectory(utility)
