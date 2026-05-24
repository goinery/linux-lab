set(COMMON_SOURCES
    common/protocol.h
    common/constants.h
)

set(SERVER_SOURCES
    server/chat_server.h
    server/chat_server.cpp
    server/client_handler.h
    server/client_handler.cpp
    server/thread_pool.h
    server/user_manager.h
    server/user_manager.cpp
)

set(CLIENT_SOURCES
    client/chat_client.h
    client/chat_client.cpp
    client/mainwindow.h
    client/mainwindow.cpp
    client/mainwindow_userlist.cpp
    client/login_dialog.h
    client/login_dialog.cpp
    client/message_widget.h
    client/message_widget.cpp
    client/resources.qrc
)

file(GLOB CHATROOM_EMBED_FONT_FILES
    "${CMAKE_SOURCE_DIR}/resources/fonts/*.ttf"
    "${CMAKE_SOURCE_DIR}/resources/fonts/*.ttc"
    "${CMAKE_SOURCE_DIR}/resources/fonts/*.otf"
)

if(CHATROOM_EMBED_FONT_FILES)
    set(CHATROOM_FONT_QRC "${CMAKE_CURRENT_BINARY_DIR}/embedded_fonts.qrc")
    file(WRITE "${CHATROOM_FONT_QRC}" "<RCC>\n    <qresource prefix=\"/fonts\">\n")

    foreach(FONT_FILE IN LISTS CHATROOM_EMBED_FONT_FILES)
        get_filename_component(FONT_NAME "${FONT_FILE}" NAME)
        file(APPEND "${CHATROOM_FONT_QRC}" "        <file alias=\"${FONT_NAME}\">${FONT_FILE}</file>\n")
    endforeach()

    file(APPEND "${CHATROOM_FONT_QRC}" "    </qresource>\n</RCC>\n")
    list(APPEND CLIENT_SOURCES "${CHATROOM_FONT_QRC}")
    message(STATUS "Embedding chatroom fonts: ${CHATROOM_EMBED_FONT_FILES}")
else()
    message(STATUS "No font files found under resources/fonts/. Using system fonts fallback.")
endif()

add_executable(${CHATROOM_PROGRAM_NAME}
    main.cpp
    app_setup.h
    app_setup.cpp
    unified_flow_window.h
    unified_flow_window.cpp
    unified_flow_window_builder.cpp
    theme_manager.h
    theme_manager.cpp
    cursor_manager.h
    cursor_manager.cpp
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
