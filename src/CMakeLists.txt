set(COMMON_SOURCES
    common/protocol.h
    common/constants.h
)

set(SERVER_SOURCES
    server/chat_server.h
    server/chat_server.cpp
    server/client_handler.h
    server/client_handler.cpp
    server/user_manager.h
    server/user_manager.cpp
)

set(CLIENT_SOURCES
    client/chat_client.h
    client/chat_client.cpp
    client/mainwindow.h
    client/mainwindow.cpp
    client/mainwindow_userlist.cpp
    client/message_widget.h
    client/message_widget.cpp
    client/resources.qrc
)

set(CHATROOM_FONT_FILE "${CMAKE_SOURCE_DIR}/resources/fonts/font.ttf")
if(NOT EXISTS "${CHATROOM_FONT_FILE}")
    message(FATAL_ERROR
        "Required bundled font is missing: ${CHATROOM_FONT_FILE}. "
        "Restore resources/fonts/font.ttf before building.")
endif()
message(STATUS "Embedding chatroom font: ${CHATROOM_FONT_FILE}")

add_executable(${CHATROOM_PROGRAM_NAME}
    main.cpp
    app_setup.h
    app_setup.cpp
    unified_flow_window.h
    unified_flow_window.cpp
    unified_flow_window_builder.cpp
    theme_manager.h
    theme_manager.cpp
    ${COMMON_SOURCES}
    ${SERVER_SOURCES}
    ${CLIENT_SOURCES}
)

target_include_directories(${CHATROOM_PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/client
)

target_link_libraries(${CHATROOM_PROGRAM_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

target_compile_definitions(${CHATROOM_PROGRAM_NAME} PRIVATE
    APP_VERSION_STR="${PROJECT_VERSION}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CHATROOM_PROGRAM_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
