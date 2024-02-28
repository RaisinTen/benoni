add_executable(req_http_unit http.cc)

target_link_libraries(req_http_unit PRIVATE req)
target_link_libraries(req_http_unit PRIVATE GTest::gtest GTest::gtest_main)

gtest_discover_tests(req_http_unit)
