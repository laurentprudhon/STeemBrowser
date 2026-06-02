# Tier 3: Integration tests (full emulator bootstrap, disk loading)
# No integration tests yet.
add_executable(tier3_integration_tests
  dummy_integration.cpp
)
target_link_libraries(tier3_integration_tests
  GTest::gtest_main
  emulator_mock
)
gtest_discover_tests(tier3_integration_tests)
