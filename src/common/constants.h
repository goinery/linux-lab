#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Constants {
    constexpr quint16 DEFAULT_PORT = 8888;
    constexpr int HEARTBEAT_INTERVAL = 30000;
    constexpr int RECONNECT_INTERVAL = 3000;
    constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    const QString DEFAULT_HOST = "127.0.0.1";
    const QString APP_NAME = "ChatRoom";
#ifndef APP_VERSION_STR
#define APP_VERSION_STR "1.0.0"
#endif
    const QString APP_VERSION = APP_VERSION_STR;
    const QString USERS_FILE = "users.json";
}

#endif
