add_library(${BENONI_TARGET} STATIC http.mm)

target_compile_options(${BENONI_TARGET} PUBLIC "-fobjc-arc")

target_include_directories(${BENONI_TARGET} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>)

target_link_libraries(${BENONI_TARGET} PUBLIC "-framework Foundation")
