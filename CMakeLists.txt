cmake_minimum_required(VERSION 3.24)
project(req VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "Networking library")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# C++ standard
set(CMAKE_CXX_STANDARD 20)

add_library(req STATIC src/http.mm)

target_include_directories(req PUBLIC include)

if(APPLE)
  target_link_libraries(req PUBLIC "-framework Foundation")
endif()

find_package(GoogleTest REQUIRED)
enable_testing()
add_subdirectory(test)
