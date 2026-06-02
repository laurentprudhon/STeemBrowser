add_library(emulator_mock STATIC
  emulator_mock.cpp
  memory_mock.cpp
  agenda_mock.cpp
  test_helpers.cpp
)

target_link_libraries(emulator_mock PRIVATE GTest::gtest)
