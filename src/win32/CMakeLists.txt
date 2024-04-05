add_library(${BENONI_TARGET} STATIC http.cc)

target_include_directories(${BENONI_TARGET} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>)

target_link_libraries(${BENONI_TARGET} PUBLIC "Winhttp.lib")
