cmake_minimum_required(VERSION 3.18)
project(project VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "Project")

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(
  "${PROJECT_SOURCE_DIR}/../../.."
  "${CMAKE_CURRENT_BINARY_DIR}/benoni_build")

add_executable(project project.cc)

string(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_SYSTEM_NAME)
set(BENONI_TARGET benoni_${LOWER_SYSTEM_NAME})

target_link_libraries(project PRIVATE ${BENONI_TARGET})
