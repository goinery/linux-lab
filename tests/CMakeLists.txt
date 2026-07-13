set(CHATROOM_SRC "${CMAKE_CURRENT_LIST_DIR}/../src")

add_executable(protocol_test
    protocol_test.cpp
    ${CHATROOM_SRC}/common/protocol.h
)
target_include_directories(protocol_test PRIVATE ${CHATROOM_SRC}/common)
target_link_libraries(protocol_test PRIVATE Qt5::Core)

add_executable(user_manager_test
    user_manager_test.cpp
    ${CHATROOM_SRC}/server/user_manager.h
    ${CHATROOM_SRC}/server/user_manager.cpp
    ${CHATROOM_SRC}/common/constants.h
)
target_include_directories(user_manager_test PRIVATE
    ${CHATROOM_SRC}/common
    ${CHATROOM_SRC}/server
)
target_link_libraries(user_manager_test PRIVATE Qt5::Core)

add_executable(server_integration_test
    server_integration_test.cpp
    ${CHATROOM_SRC}/server/chat_server.h
    ${CHATROOM_SRC}/server/chat_server.cpp
    ${CHATROOM_SRC}/server/client_handler.h
    ${CHATROOM_SRC}/server/client_handler.cpp
    ${CHATROOM_SRC}/server/user_manager.h
    ${CHATROOM_SRC}/server/user_manager.cpp
    ${CHATROOM_SRC}/common/protocol.h
    ${CHATROOM_SRC}/common/constants.h
)
target_include_directories(server_integration_test PRIVATE
    ${CHATROOM_SRC}/common
    ${CHATROOM_SRC}/server
)
target_link_libraries(server_integration_test PRIVATE Qt5::Core Qt5::Network)

add_executable(chat_client_test
    chat_client_test.cpp
    ${CHATROOM_SRC}/client/chat_client.h
    ${CHATROOM_SRC}/client/chat_client.cpp
    ${CHATROOM_SRC}/common/protocol.h
    ${CHATROOM_SRC}/common/constants.h
)
target_include_directories(chat_client_test PRIVATE
    ${CHATROOM_SRC}/client
    ${CHATROOM_SRC}/common
)
target_link_libraries(chat_client_test PRIVATE Qt5::Core Qt5::Network)

foreach(test_target IN ITEMS
        protocol_test
        user_manager_test
        server_integration_test
        chat_client_test)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

add_test(NAME protocol_test COMMAND protocol_test)
add_test(NAME user_manager_test COMMAND user_manager_test)
add_test(NAME server_integration_test COMMAND server_integration_test)
add_test(NAME chat_client_test COMMAND chat_client_test)
set_tests_properties(server_integration_test PROPERTIES TIMEOUT 20)
set_tests_properties(chat_client_test PROPERTIES TIMEOUT 15)

