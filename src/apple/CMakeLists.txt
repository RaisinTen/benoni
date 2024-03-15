add_library(benoni STATIC http.mm)

target_include_directories(benoni PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(benoni PUBLIC "-framework Foundation")
