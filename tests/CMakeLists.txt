cmake_minimum_required(VERSION 3.16)
project(SteemSSETests LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.11.0
)
set(gtest_force_shared_crl ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(CTest)
include(GoogleTest)
include(${CMAKE_SOURCE_DIR}/cmake/test_flags.cmake)

# Source directory for steemsse
set(STEEM_SOURCE_DIR ${CMAKE_SOURCE_DIR}/../steemsse-V4.2.0_R10/steem)
set(STEEM_HEADER_DIR ${STEEM_SOURCE_DIR}/headers)
set(STEEM_CODE_DIR ${STEEM_SOURCE_DIR}/code)
set(STEEM_3RDPARTY_DIR ${CMAKE_SOURCE_DIR}/../steemsse-V4.2.0_R10/3rdparty)

include_directories(
  ${STEEM_HEADER_DIR}
  ${STEEM_CODE_DIR}
  ${STEEM_CODE_DIR}/x
  ${STEEM_3RDPARTY_DIR}
  ${STEEM_3RDPARTY_DIR}/6301
  ${STEEM_3RDPARTY_DIR}/zlib
  ${STEEM_3RDPARTY_DIR}/zlib/contrib/minizip
  ${STEEM_3RDPARTY_DIR}/rtaudio
  ${CMAKE_SOURCE_DIR}/framework
)

add_definitions(
  -DTEST_BUILD -DUNIX -DLINUX -DSSE_DRAW_C -DSSE_RELEASE
  -DSSE_NO_OSD -DNO_PORTAUDIO -DNO_RTAUDIO -DSSE_NO_SCREENSAVER
  -DNDEBUG -DNO_DEBUG_BUILD
)

add_subdirectory(framework)
add_subdirectory(tier1_unit)
add_subdirectory(tier2_component)
add_subdirectory(tier3_integration)
