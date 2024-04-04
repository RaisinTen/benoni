cmake_minimum_required(VERSION 3.24)
project(benoni VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "Networking library")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# C++ standard
set(CMAKE_CXX_STANDARD 20)

option(BENONI_TESTS "Build the Benoni tests" OFF)
option(BENONI_EXAMPLES "Build the Benoni examples" OFF)
option(BENONI_INSTALL "Install Benoni" ON)

if(APPLE)
  add_subdirectory(src/apple)
elseif(WIN32)
  add_subdirectory(src/win32)
elseif(UNIX)
  find_package(LibSoup REQUIRED)
  add_subdirectory(src/linux)
endif()

set_target_properties(benoni PROPERTIES PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/benoni/http.h)

if(BENONI_INSTALL)
  include(GNUInstallDirs)

  install(TARGETS benoni
    EXPORT benoni
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
      COMPONENT benoni
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT benoni)
  install(EXPORT benoni
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmake"
      COMPONENT benoni)
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
