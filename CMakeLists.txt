cmake_minimum_required(VERSION 3.24)
project(benoni VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "Networking library")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# C++ standard
set(CMAKE_CXX_STANDARD 20)

option(BENONI_TESTS "Build the Benoni tests" OFF)
option(BENONI_EXAMPLES "Build the Benoni examples" OFF)

if(APPLE)
  add_subdirectory(src/apple)
elseif(WIN32)
  add_subdirectory(src/win32)
elseif(UNIX)
  find_package(LibSoup REQUIRED)
  add_subdirectory(src/linux)
endif()

if(BENONI_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BENONI_TESTS)
  enable_testing()
  add_subdirectory(test/unit)

  if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test/packaging)
  endif()
endif()
