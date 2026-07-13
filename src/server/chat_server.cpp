#include "chat_server.h"

#include "protocol.h"
#include "user_manager.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QMutexLocker>
#include <QTcpSocket>

namespace {

constexpr int kMinUsernameLength = 2;
constexpr int kMaxUsernameLength = 32;
constexpr int kMinPasswordLength = 4;
constexpr int kMaxPasswordLength = 128;
constexpr int kMaxChatLength = 4096;

QString logPrefix() {
    return QDateTime::currentDateTime().toString(QStringLiteral("[hh:mm:ss] "));
}

bool validUsername(const QString &username) {
    if (username != username.trimmed()
        || username.size() < kMinUsernameLength
        || username.size() > kMaxUsernameLength) {
        return false;
    }
    for (const QChar ch : username) {
        if (!ch.isPrint() || ch.isSpace()) {
            return false;
        }
    }
    return true;
}

bool validPassword(const QString &password) {
    return password.size() >= kMinPasswordLength
        && password.size() <= kMaxPasswordLength;
}

} // namespace

ChatServer::ChatServer(quint16 port, QObject *parent)
    : QObject(parent), tcpServer_(new QTcpServer(this)), port_(port), totalMessages_(0) {
    connect(tcpServer_, &QTcpServer::newConnection,
            this, &ChatServer::onNewConnection);
}

ChatServer::~ChatServer() = default;

bool ChatServer::start() {
    if (tcpServer_->isListening()) {
        return true;
    }
    if (!tcpServer_->listen(QHostAddress::Any, port_)) {
        qCritical() << "Server failed to start on port" << port_
                    << ":" << tcpServer_->errorString();
        return false;
    }

    startTime_ = QDateTime::currentDateTime();
    totalMessages_ = 0;
    appendLog(logPrefix() + QString("服务端已启动，监听端口 %1").arg(port_));
    emitStats();
    qInfo() << "Chat server started on port" << port_;
    return true;
}

void ChatServer::stop() {
    bool wasRunning = tcpServer_->isListening();
    tcpServer_->close();

    QList<ClientHandler *> handlers;
    {
        QMutexLocker locker(&clientsMutex_);
        handlers = clients_.values();
        wasRunning = wasRunning || !handlers.isEmpty();
        clients_.clear();
        mutedUsers_.clear();
    }

    for (ClientHandler *handler : handlers) {
        handler->disconnectClient();
        handler->deleteLater();
    }

    totalMessages_ = 0;
    startTime_ = {};
    emit onlineUsersUpdated({});
    emit mutedUsersUpdated({});
    emitStats();
    if (wasRunning) {
        appendLog(logPrefix() + "服务端已停止");
        qInfo() << "Chat server stopped";
    }
}

void ChatServer::onNewConnection() {
    while (tcpServer_->hasPendingConnections()) {
        QTcpSocket *socket = tcpServer_->nextPendingConnection();
        if (!socket) {
            continue;
        }

        const qintptr descriptor = socket->socketDescriptor();
        auto *handler = new ClientHandler(socket, this);
        {
            QMutexLocker locker(&clientsMutex_);
            clients_.insert(descriptor, handler);
        }

        connect(handler, &ClientHandler::messageReceived,
                this, &ChatServer::onMessageReceived);
        connect(handler, &ClientHandler::disconnected,
                this, &ChatServer::onClientDisconnected);
        handler->start();

        const QString peerInfo = QString("%1:%2")
            .arg(socket->peerAddress().toString())
            .arg(socket->peerPort());
        const QString entry = logPrefix() + "新连接 " + peerInfo;
        appendLog(entry);
        emitStats();
        qInfo() << entry;
    }
}

void ChatServer::onMessageReceived(qintptr socketDescriptor, QJsonObject message) {
    ClientHandler *handler = nullptr;
    {
        QMutexLocker locker(&clientsMutex_);
        handler = clients_.value(socketDescriptor, nullptr);
    }
    if (!handler) {
        qWarning() << "Message from unknown descriptor:" << socketDescriptor;
        return;
    }

    const QString typeName = message.value("type").toString();
    const Protocol::MessageType type = Protocol::typeFromString(typeName);
    switch (type) {
    case Protocol::MessageType::Register:
        handleRegister(handler, message);
        return;
    case Protocol::MessageType::Login:
        handleLogin(handler, message);
        return;
    case Protocol::MessageType::Heartbeat:
        return;
    default:
        break;
    }

    if (!handler->isAuthenticated()) {
        handler->sendMessage(Protocol::createSystemMessage("请先登录后再执行该操作"));
        return;
    }

    switch (type) {
    case Protocol::MessageType::Chat:
        handleChatMessage(handler, message);
        break;
    case Protocol::MessageType::GroupChat:
        handleGroupChat(handler, message);
        break;
    case Protocol::MessageType::UserListRequest:
        sendUserList();
        break;
    case Protocol::MessageType::Logout:
        handleLogout(handler);
        break;
    default:
        qWarning() << "Unknown or client-forbidden message type:" << typeName;
        handler->sendMessage(Protocol::createSystemMessage("不支持的消息类型"));
        break;
    }
}

void ChatServer::onClientDisconnected(qintptr socketDescriptor) {
    ClientHandler *handler = nullptr;
    QString username;
    bool wasAuthenticated = false;
    QStringList muted;
    {
        QMutexLocker locker(&clientsMutex_);
        handler = clients_.take(socketDescriptor);
        if (handler) {
            username = handler->username();
            wasAuthenticated = handler->isAuthenticated() && !username.isEmpty();
            mutedUsers_.remove(username);
            muted = mutedUsers_.values();
        }
    }

    if (!handler) {
        return;
    }
    handler->deleteLater();

    if (wasAuthenticated) {
        broadcastSystemMessage(username + " 离开了聊天室");
        sendUserList();
        emit mutedUsersUpdated(muted);
        appendLog(logPrefix() + username + " 断开连接（已认证）");
    } else {
        appendLog(logPrefix() + "未认证客户端断开连接");
    }
    emitStats();
}

void ChatServer::handleLogin(ClientHandler *handler, const QJsonObject &msg) {
    if (handler->isAuthenticated()) {
        handler->sendMessage(Protocol::createLoginResponse(false, "当前连接已经登录"));
        return;
    }

    const QString username = msg.value("username").toString().trimmed();
    const QString password = msg.value("password").toString();
    if (!validUsername(username) || !validPassword(password)) {
        handler->sendMessage(Protocol::createLoginResponse(false, "用户名或密码格式错误"));
        return;
    }

    if (!UserManager::instance().loginUser(username, password)) {
        handler->sendMessage(Protocol::createLoginResponse(false, "用户名或密码错误"));
        return;
    }

    bool duplicateLogin = false;
    {
        QMutexLocker locker(&clientsMutex_);
        for (ClientHandler *client : clients_) {
            if (client != handler && client->isAuthenticated()
                && client->username() == username) {
                duplicateLogin = true;
                break;
            }
        }
    }
    if (duplicateLogin) {
        handler->sendMessage(Protocol::createLoginResponse(
            false, "该账号已在其他地方登录"));
        return;
    }

    handler->setUsername(username);
    handler->setAuthenticated(true);
    handler->sendMessage(Protocol::createLoginResponse(true, "登录成功"));
    broadcastSystemMessage(username + " 加入了聊天室");
    sendUserList();
    appendLog(logPrefix() + username + " 登录成功");
    emitStats();
}

void ChatServer::handleRegister(ClientHandler *handler, const QJsonObject &msg) {
    if (handler->isAuthenticated()) {
        handler->sendMessage(Protocol::createRegisterResponse(
            false, "请先退出当前账号再注册新用户"));
        return;
    }

    const QString username = msg.value("username").toString().trimmed();
    const QString password = msg.value("password").toString();
    if (!validUsername(username) || !validPassword(password)) {
        handler->sendMessage(Protocol::createRegisterResponse(
            false, "用户名需为2-32个字符，密码需为4-128个字符"));
        return;
    }

    UserManager &users = UserManager::instance();
    if (users.userExists(username)) {
        handler->sendMessage(Protocol::createRegisterResponse(false, "用户名已存在"));
        return;
    }
    if (!users.registerUser(username, password)) {
        handler->sendMessage(Protocol::createRegisterResponse(false, "用户数据保存失败"));
        return;
    }
    handler->sendMessage(Protocol::createRegisterResponse(true, "注册成功，请登录"));
}

void ChatServer::handleChatMessage(ClientHandler *handler, const QJsonObject &msg) {
    const QString to = msg.value("to").toString().trimmed();
    const QString content = msg.value("content").toString().trimmed();
    if (!validUsername(to) || content.isEmpty() || content.size() > kMaxChatLength) {
        handler->sendMessage(Protocol::createSystemMessage(
            "私聊目标或消息内容无效", to));
        return;
    }

    ++totalMessages_;
    emitStats();
    if (isUserMuted(handler->username())) {
        handler->sendMessage(Protocol::createSystemMessage(
            "你已被禁言，无法发送消息", to));
        return;
    }

    ClientHandler *recipient = nullptr;
    {
        QMutexLocker locker(&clientsMutex_);
        for (ClientHandler *client : clients_) {
            if (client->isAuthenticated() && client->username() == to) {
                recipient = client;
                break;
            }
        }
    }
    if (!recipient) {
        handler->sendMessage(Protocol::createSystemMessage(
            "目标用户当前不在线", to));
        return;
    }

    const QJsonObject trustedMessage = Protocol::createChatMessage(
        handler->username(), to, content);
    recipient->sendMessage(trustedMessage);
    if (recipient != handler) {
        handler->sendMessage(trustedMessage);
    }
}

void ChatServer::handleGroupChat(ClientHandler *handler, const QJsonObject &msg) {
    const QString content = msg.value("content").toString().trimmed();
    if (content.isEmpty() || content.size() > kMaxChatLength) {
        handler->sendMessage(Protocol::createSystemMessage("群聊消息内容无效", "general"));
        return;
    }

    ++totalMessages_;
    emitStats();
    if (isUserMuted(handler->username())) {
        handler->sendMessage(Protocol::createSystemMessage(
            "你已被禁言，无法发送消息", "general"));
        return;
    }

    broadcastMessage(Protocol::createGroupChatMessage(handler->username(), content));
}

void ChatServer::handleLogout(ClientHandler *handler) {
    const QString username = handler->username();
    handler->setAuthenticated(false);
    handler->setUsername({});

    QStringList muted;
    {
        QMutexLocker locker(&clientsMutex_);
        mutedUsers_.remove(username);
        muted = mutedUsers_.values();
    }
    if (!username.isEmpty()) {
        broadcastSystemMessage(username + " 退出了聊天室");
        appendLog(logPrefix() + username + " 主动退出");
    }
    emit mutedUsersUpdated(muted);
    sendUserList();
    emitStats();
}

void ChatServer::broadcastMessage(const QJsonObject &message) {
    QList<ClientHandler *> recipients;
    {
        QMutexLocker locker(&clientsMutex_);
        for (ClientHandler *client : clients_) {
            if (client->isAuthenticated()) {
                recipients.append(client);
            }
        }
    }
    for (ClientHandler *recipient : recipients) {
        recipient->sendMessage(message);
    }
}

void ChatServer::sendUserList() {
    QStringList onlineUsers;
    {
        QMutexLocker locker(&clientsMutex_);
        for (ClientHandler *client : clients_) {
            if (client->isAuthenticated()) {
                onlineUsers.append(client->username());
            }
        }
    }
    onlineUsers.sort(Qt::CaseInsensitive);

    QJsonArray users;
    for (const QString &username : onlineUsers) {
        users.append(username);
    }
    broadcastMessage(Protocol::createUserListMessage(users));
    emit onlineUsersUpdated(onlineUsers);
}

void ChatServer::broadcastSystemMessage(const QString &content) {
    broadcastMessage(Protocol::createSystemMessage(content));
}

void ChatServer::muteUser(const QString &username) {
    const QString normalized = username.trimmed();
    ClientHandler *target = nullptr;
    QStringList muted;
    {
        QMutexLocker locker(&clientsMutex_);
        for (ClientHandler *client : clients_) {
            if (client->isAuthenticated() && client->username() == normalized) {
                target = client;
                break;
            }
        }
        if (!target || mutedUsers_.contains(normalized)) {
            return;
        }
        mutedUsers_.insert(normalized);
        muted = mutedUsers_.values();
    }

    appendLog(logPrefix() + "禁言用户: " + normalized);
    broadcastSystemMessage(normalized + " 已被管理员禁言");
    target->sendMessage(Protocol::createSystemMessage("你已被管理员禁言，无法发送消息"));
    emit mutedUsersUpdated(muted);
}

void ChatServer::unmuteUser(const QString &username) {
    const QString normalized = username.trimmed();
    ClientHandler *target = nullptr;
    QStringList muted;
    {
        QMutexLocker locker(&clientsMutex_);
        if (!mutedUsers_.remove(normalized)) {
            return;
        }
        for (ClientHandler *client : clients_) {
            if (client->isAuthenticated() && client->username() == normalized) {
                target = client;
                break;
            }
        }
        muted = mutedUsers_.values();
    }

    appendLog(logPrefix() + "解除禁言: " + normalized);
    broadcastSystemMessage(normalized + " 已被管理员解除禁言");
    if (target) {
        target->sendMessage(Protocol::createSystemMessage("你已被解除禁言，可以正常发言"));
    }
    emit mutedUsersUpdated(muted);
}

bool ChatServer::isUserMuted(const QString &username) const {
    QMutexLocker locker(&clientsMutex_);
    return mutedUsers_.contains(username);
}

void ChatServer::serverBroadcast(const QString &content) {
    const QString normalized = content.trimmed();
    if (normalized.isEmpty() || normalized.size() > kMaxChatLength) {
        return;
    }
    broadcastMessage(Protocol::createSystemMessage("【服务器公告】" + normalized));
    appendLog(logPrefix() + "服务器公告已发送");
}

void ChatServer::appendLog(const QString &entry) {
    emit serverLogUpdated(entry);
}

void ChatServer::emitStats() {
    int activeConnections = 0;
    {
        QMutexLocker locker(&clientsMutex_);
        activeConnections = clients_.size();
    }

    QString uptime = "-";
    if (startTime_.isValid()) {
        const qint64 seconds = qMax<qint64>(
            0, startTime_.secsTo(QDateTime::currentDateTime()));
        const qint64 hours = seconds / 3600;
        const qint64 minutes = (seconds % 3600) / 60;
        const qint64 remainingSeconds = seconds % 60;
        if (hours > 0) {
            uptime = QString("%1时%2分%3秒")
                .arg(hours).arg(minutes).arg(remainingSeconds);
        } else if (minutes > 0) {
            uptime = QString("%1分%2秒").arg(minutes).arg(remainingSeconds);
        } else {
            uptime = QString("%1秒").arg(remainingSeconds);
        }
    }
    emit serverStatsUpdated(activeConnections, totalMessages_, uptime);
}
