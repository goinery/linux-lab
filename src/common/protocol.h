#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QString>
#include <QDateTime>
#include <QDebug>

namespace Protocol {

enum class MessageType {
    Register,
    Login,
    LoginResponse,
    RegisterResponse,
    Chat,
    GroupChat,
    System,
    UserList,
    UserListRequest,
    Heartbeat,
    Logout,
    Unknown
};

inline MessageType typeFromString(const QString &str) {
    if (str == "register") return MessageType::Register;
    if (str == "login") return MessageType::Login;
    if (str == "login_response") return MessageType::LoginResponse;
    if (str == "register_response") return MessageType::RegisterResponse;
    if (str == "chat") return MessageType::Chat;
    if (str == "group_chat") return MessageType::GroupChat;
    if (str == "system") return MessageType::System;
    if (str == "user_list") return MessageType::UserList;
    if (str == "user_list_request") return MessageType::UserListRequest;
    if (str == "heartbeat") return MessageType::Heartbeat;
    if (str == "logout") return MessageType::Logout;
    return MessageType::Unknown;
}

inline QString typeToString(MessageType type) {
    switch (type) {
    case MessageType::Register: return "register";
    case MessageType::Login: return "login";
    case MessageType::LoginResponse: return "login_response";
    case MessageType::RegisterResponse: return "register_response";
    case MessageType::Chat: return "chat";
    case MessageType::GroupChat: return "group_chat";
    case MessageType::System: return "system";
    case MessageType::UserList: return "user_list";
    case MessageType::UserListRequest: return "user_list_request";
    case MessageType::Heartbeat: return "heartbeat";
    case MessageType::Logout: return "logout";
    default: return "unknown";
    }
}

inline QJsonObject createRegisterRequest(const QString &username, const QString &password) {
    return {{"type", "register"}, {"username", username}, {"password", password}};
}

inline QJsonObject createLoginRequest(const QString &username, const QString &password) {
    return {{"type", "login"}, {"username", username}, {"password", password}};
}

inline QJsonObject createLoginResponse(bool success, const QString &message) {
    return {{"type", "login_response"}, {"success", success}, {"message", message}};
}

inline QJsonObject createRegisterResponse(bool success, const QString &message) {
    return {{"type", "register_response"}, {"success", success}, {"message", message}};
}

inline QJsonObject createChatMessage(const QString &from, const QString &to, const QString &content) {
    return {{"type", "chat"}, {"from", from}, {"to", to},
            {"content", content}, {"timestamp", QDateTime::currentSecsSinceEpoch()}};
}

inline QJsonObject createGroupChatMessage(const QString &from, const QString &content) {
    return {{"type", "group_chat"}, {"from", from}, {"room", "general"},
            {"content", content}, {"timestamp", QDateTime::currentSecsSinceEpoch()}};
}

inline QJsonObject createSystemMessage(const QString &content) {
    return {{"type", "system"}, {"content", content}, {"timestamp", QDateTime::currentSecsSinceEpoch()}};
}

inline QJsonObject createSystemMessage(const QString &content, const QString &targetChat) {
    return {{"type", "system"}, {"content", content}, {"target_chat", targetChat}, {"timestamp", QDateTime::currentSecsSinceEpoch()}};
}

inline QJsonObject createUserListMessage(const QJsonArray &users) {
    return {{"type", "user_list"}, {"users", users}};
}

inline QJsonObject createUserListRequest() {
    return {{"type", "user_list_request"}};
}

inline QJsonObject createHeartbeat() {
    return {{"type", "heartbeat"}, {"timestamp", QDateTime::currentSecsSinceEpoch()}};
}

inline QJsonObject createLogoutMessage(const QString &username) {
    return {{"type", "logout"}, {"username", username}};
}

inline QByteArray serializeMessage(const QJsonObject &json) {
    QJsonDocument doc(json);
    QByteArray body = doc.toJson(QJsonDocument::Compact);
    QByteArray header;
    header.resize(4);
    quint32 size = quint32(body.size());
    header[0] = char((size >> 24) & 0xFF);
    header[1] = char((size >> 16) & 0xFF);
    header[2] = char((size >> 8) & 0xFF);
    header[3] = char(size & 0xFF);
    return header + body;
}

inline bool deserializeMessage(QByteArray &buffer, QJsonObject &outJson) {
    if (buffer.size() < 4) return false;
    quint32 size = (quint8(buffer[0]) << 24) | (quint8(buffer[1]) << 16) |
                   (quint8(buffer[2]) << 8) | quint8(buffer[3]);
    if (quint32(buffer.size()) < 4 + size) return false;
    QByteArray body = buffer.mid(4, size);
    buffer.remove(0, 4 + size);
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isNull()) return false;
    outJson = doc.object();
    return true;
}

}

#endif
