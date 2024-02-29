add_library(req STATIC http.cc)

target_include_directories(req PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(req PUBLIC "Winhttp.lib")
