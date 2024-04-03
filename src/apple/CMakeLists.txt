add_library(benoni STATIC http.mm)

target_compile_options(benoni PUBLIC "-fobjc-arc")

target_include_directories(benoni PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>)

target_link_libraries(benoni PUBLIC "-framework Foundation")
