add_test(NAME packaging.project_configure COMMAND
  "${CMAKE_COMMAND}"
  -S "${CMAKE_CURRENT_SOURCE_DIR}/project"
  -B "${CMAKE_CURRENT_BINARY_DIR}/project"
  "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}")

add_test(NAME packaging.project_build COMMAND
  "${CMAKE_COMMAND}"
  --build "${CMAKE_CURRENT_BINARY_DIR}/project")

set_tests_properties(packaging.project_build
  PROPERTIES DEPENDS packaging.project_configure)
