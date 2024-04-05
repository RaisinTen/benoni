add_executable(postman_echo_get postman-echo-get.cc)

target_link_libraries(postman_echo_get PRIVATE ${BENONI_TARGET})

add_test(NAME postman_echo_get COMMAND $<TARGET_FILE:postman_echo_get>)
