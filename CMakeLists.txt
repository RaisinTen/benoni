cmake_minimum_required(VERSION 3.24)
project(benoni VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "Networking library")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# C++ standard
set(CMAKE_CXX_STANDARD 20)

if(APPLE)
  add_subdirectory(src/apple)
elseif(WIN32)
  add_subdirectory(src/win32)
elseif(UNIX)
  find_package(LibSoup REQUIRED)
  add_subdirectory(src/linux)
endif()

add_subdirectory(examples)

enable_testing()
add_subdirectory(test)
