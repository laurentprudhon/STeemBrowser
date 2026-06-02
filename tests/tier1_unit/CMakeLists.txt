# Tier 1: Unit tests (individual functions, no emulator init)
add_executable(tier1_unit_tests
  utility/test_infrastructure_smoke.cpp
)
target_link_libraries(tier1_unit_tests
  GTest::gtest_main
  emulator_mock
)
gtest_discover_tests(tier1_unit_tests)
