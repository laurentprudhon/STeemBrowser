# Tier 2: Component tests (sub-systems like FDC, MFP, Blitter)
# No component tests yet.
add_executable(tier2_component_tests
  dummy_component.cpp
)
target_link_libraries(tier2_component_tests
  GTest::gtest_main
  emulator_mock
)
gtest_discover_tests(tier2_component_tests)
