add_executable(benoni_http_unit http.cc)

target_link_libraries(benoni_http_unit PRIVATE benoni)
target_link_libraries(benoni_http_unit PRIVATE GTest::gtest GTest::gtest_main)

gtest_discover_tests(benoni_http_unit)
