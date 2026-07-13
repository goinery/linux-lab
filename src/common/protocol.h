#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QByteArray>
#include <QString>
#include <QDateTime>
#include <QDebug>

namespace Protocol {

constexpr quint32 MAX_PAYLOAD_SIZE = 1024U * 1024U;

enum class DecodeStatus {
    Complete,
    Incomplete,
    Invalid
};

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

inline QByteArray serializeMessage(const QJsonObject &json) {
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);
    if (body.isEmpty() || quint32(body.size()) > MAX_PAYLOAD_SIZE) {
        qWarning() << "Refusing to serialize invalid or oversized message:"
                   << body.size() << "bytes";
        return {};
    }

    QByteArray header;
    header.resize(4);
    const quint32 size = quint32(body.size());
    header[0] = char((size >> 24) & 0xFF);
    header[1] = char((size >> 16) & 0xFF);
    header[2] = char((size >> 8) & 0xFF);
    header[3] = char(size & 0xFF);
    return header + body;
}

inline DecodeStatus deserializeMessage(QByteArray &buffer, QJsonObject &outJson,
                                       QString *errorMessage = nullptr) {
    if (buffer.size() < 4) {
        return DecodeStatus::Incomplete;
    }

    const quint32 size = (quint32(quint8(buffer[0])) << 24)
                       | (quint32(quint8(buffer[1])) << 16)
                       | (quint32(quint8(buffer[2])) << 8)
                       | quint32(quint8(buffer[3]));
    if (size == 0 || size > MAX_PAYLOAD_SIZE) {
        if (errorMessage) {
            *errorMessage = QString("非法消息长度: %1 字节").arg(size);
        }
        buffer.clear();
        return DecodeStatus::Invalid;
    }

    if (quint32(buffer.size()) < 4U + size) {
        return DecodeStatus::Incomplete;
    }

    const QByteArray body = buffer.mid(4, int(size));
    buffer.remove(0, int(4U + size));

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QString("JSON 消息格式错误: %1").arg(parseError.errorString());
        }
        buffer.clear();
        return DecodeStatus::Invalid;
    }
    if (!doc.isObject()) {
        if (errorMessage) {
            *errorMessage = "JSON 消息根节点必须是对象";
        }
        buffer.clear();
        return DecodeStatus::Invalid;
    }

    outJson = doc.object();
    return DecodeStatus::Complete;
}

}

#endif
