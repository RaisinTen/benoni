if(NOT GoogleTest_FOUND)
  # Run GoogleTest discover after the actual build
  set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
  include(GoogleTest)
  set(BUILD_GMOCK ON CACHE BOOL "enable googlemock")
  set(INSTALL_GTEST OFF CACHE BOOL "disable installation")
  add_subdirectory("${PROJECT_SOURCE_DIR}/deps/googletest")
  set(GoogleTest_FOUND ON)
endif()
