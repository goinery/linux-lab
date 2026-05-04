#include "chat_server.h"
#include "user_manager.h"
#include "protocol.h"
#include "constants.h"
#include <QJsonArray>

ChatServer::ChatServer(quint16 port, QObject *parent)
    : QObject(parent), port_(port), threadPool_(4), totalConnections_(0), totalMessages_(0) {}

ChatServer::~ChatServer() {
    tcpServer_->close();
}

bool ChatServer::start() {
    tcpServer_ = new QTcpServer(this);
    if (!tcpServer_->listen(QHostAddress::Any, port_)) {
        qCritical() << "Server failed to start on port" << port_ << ":" << tcpServer_->errorString();
        return false;
    }
    connect(tcpServer_, &QTcpServer::newConnection, this, &ChatServer::onNewConnection);
    startTime_ = QDateTime::currentDateTime();
    qInfo() << "Chat server started on port" << port_;
    return true;
}

void ChatServer::stop() {
    if (tcpServer_ && tcpServer_->isListening()) {
        tcpServer_->close();
    }
    {
        QMutexLocker locker(&clientsMutex_);
        QList<ClientHandler *> handlers = clients_.values();
        clients_.clear();
        locker.unlock();
        for (ClientHandler *h : handlers) {
            h->disconnectClient();
            h->deleteLater();
        }
    }
    mutedUsers_.clear();
    totalConnections_ = 0;
    totalMessages_ = 0;
    emit onlineUsersUpdated(QStringList());
    emit mutedUsersUpdated(QStringList());
    emitStats();
    appendLog(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + "服务端已停止");
    qInfo() << "Chat server stopped";
}

void ChatServer::onNewConnection() {
    while (tcpServer_->hasPendingConnections()) {
        QTcpSocket *socket = tcpServer_->nextPendingConnection();
        qintptr descriptor = socket->socketDescriptor();
        ClientHandler *handler = new ClientHandler(socket, this);
        {
            QMutexLocker locker(&clientsMutex_);
            clients_[descriptor] = handler;
        }
        connect(handler, &ClientHandler::messageReceived, this, &ChatServer::onMessageReceived);
        connect(handler, &ClientHandler::disconnected, this, &ChatServer::onClientDisconnected);
        handler->start();
        ++totalConnections_;
        const QString peerInfo = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());
        QString entry = QDateTime::currentDateTime().toString("[hh:mm:ss] ") + "新连接 " + peerInfo;
        appendLog(entry);
        emitStats();
        qInfo() << entry;
    }
}

void ChatServer::onMessageReceived(qintptr socketDescriptor, QJsonObject message) {
    ClientHandler *handler = nullptr;
    {
        QMutexLocker locker(&clientsMutex_);
        if (!clients_.contains(socketDescriptor)) {
            qWarning() << "Message from unknown descriptor:" << socketDescriptor;
            return;
        }
        handler = clients_.value(socketDescriptor);
    }

    if (!handler) {
        qWarning() << "Null handler for descriptor:" << socketDescriptor;
        return;
    }

    QString typeStr = message["type"].toString();
    Protocol::MessageType type = Protocol::typeFromString(typeStr);

    if (type == Protocol::MessageType::Chat || type == Protocol::MessageType::GroupChat) {
        ++totalMessages_;
        emitStats();
    }

    switch (type) {
    case Protocol::MessageType::Login:
        handleLogin(handler, message);
        break;
    case Protocol::MessageType::Register:
        handleRegister(handler, message);
        break;
    case Protocol::MessageType::Chat:
        handleChatMessage(message);
        break;
    case Protocol::MessageType::GroupChat:
        handleGroupChat(message);
        break;
    case Protocol::MessageType::UserListRequest:
        if (handler->isAuthenticated()) {
            sendUserList();
        }
        break;
    case Protocol::MessageType::Logout:
        handleLogout(socketDescriptor, message);
        break;
    case Protocol::MessageType::Heartbeat:
        break;
    default:
        qWarning() << "Unknown message type:" << typeStr;
        break;
    }
}

void ChatServer::onClientDisconnected(qintptr socketDescriptor) {
    QMutexLocker locker(&clientsMutex_);
    ClientHandler *handler = clients_.take(socketDescriptor);
    if (handler) {
        QString username = handler->username();
        if (!username.isEmpty()) {
            locker.unlock();
            broadcastSystemMessage(username + " 离开了聊天室");
            sendUserList();
            appendLog(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + username + " 断开连接（已认证）");
            locker.relock();
            mutedUsers_.remove(username);
            emit mutedUsersUpdated(mutedUsers_.values());
        } else {
            appendLog(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + "未认证客户端断开连接");
        }
        handler->deleteLater();
    }
    locker.unlock();
    emitStats();
}

void ChatServer::handleLogin(ClientHandler *handler, const QJsonObject &msg) {
    QString username = msg["username"].toString();
    QString password = msg["password"].toString();

    bool success = UserManager::instance().loginUser(username, password);
    QJsonObject response = Protocol::createLoginResponse(success,
        success ? "登录成功" : "用户名或密码错误");

    if (success) {
        handler->setUsername(username);
        handler->setAuthenticated(true);
        QMutexLocker locker(&clientsMutex_);
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            if (it.value() != handler && it.value()->isAuthenticated() && it.value()->username() == username) {
                response = Protocol::createLoginResponse(false, "该账号已在其他地方登录");
                handler->setAuthenticated(false);
                handler->setUsername("");
                break;
            }
        }
    }

    handler->sendMessage(response);

    if (handler->isAuthenticated()) {
        broadcastSystemMessage(username + " 加入了聊天室");
        sendUserList();
        appendLog(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + username + " 登录成功");
        emitStats();
    }
}

void ChatServer::handleRegister(ClientHandler *handler, const QJsonObject &msg) {
    QString username = msg["username"].toString();
    QString password = msg["password"].toString();

    if (username.length() < 2 || password.length() < 4) {
        handler->sendMessage(Protocol::createRegisterResponse(false, "用户名至少2个字符，密码至少4个字符"));
        return;
    }

    bool success = UserManager::instance().registerUser(username, password);
    handler->sendMessage(Protocol::createRegisterResponse(success,
        success ? "注册成功，请登录" : "用户名已存在"));
}

void ChatServer::handleChatMessage(const QJsonObject &msg) {
    QString from = msg["from"].toString();
    QString to = msg["to"].toString();
    {
        QMutexLocker locker(&clientsMutex_);
        if (mutedUsers_.contains(from)) {
            for (auto it = clients_.begin(); it != clients_.end(); ++it) {
                if (it.value()->isAuthenticated() && it.value()->username() == from) {
                    it.value()->sendMessage(Protocol::createSystemMessage("你已被禁言，无法发送消息", to));
                    break;
                }
            }
            return;
        }
    }
    QMutexLocker locker(&clientsMutex_);
    bool sentToRecipient = false;
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        if (it.value()->isAuthenticated() && it.value()->username() == to) {
            it.value()->sendMessage(msg);
            sentToRecipient = true;
            break;
        }
    }
    if (sentToRecipient) {
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            if (it.value()->isAuthenticated() && it.value()->username() == from) {
                it.value()->sendMessage(msg);
                break;
            }
        }
    }
}

void ChatServer::handleGroupChat(const QJsonObject &msg) {
    QString from = msg["from"].toString();
    {
        QMutexLocker locker(&clientsMutex_);
        if (mutedUsers_.contains(from)) {
            for (auto it = clients_.begin(); it != clients_.end(); ++it) {
                if (it.value()->isAuthenticated() && it.value()->username() == from) {
                    it.value()->sendMessage(Protocol::createSystemMessage("你已被禁言，无法发送消息", "general"));
                    break;
                }
            }
            return;
        }
    }
    broadcastMessage(msg);
}

void ChatServer::handleLogout(qintptr socketDescriptor, const QJsonObject &msg) {
    Q_UNUSED(msg)
    QMutexLocker locker(&clientsMutex_);
    if (clients_.contains(socketDescriptor)) {
        ClientHandler *handler = clients_[socketDescriptor];
        handler->setAuthenticated(false);
    }
}

void ChatServer::broadcastMessage(const QJsonObject &message) {
    QMutexLocker locker(&clientsMutex_);
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        if (it.value()->isAuthenticated()) {
            it.value()->sendMessage(message);
        }
    }
}

void ChatServer::sendUserList() {
    QStringList onlineUsers;
    {
        QMutexLocker locker(&clientsMutex_);
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            if (it.value()->isAuthenticated()) {
                onlineUsers.append(it.value()->username());
            }
        }
    }
    QJsonArray arr;
    for (const QString &u : onlineUsers) arr.append(u);
    broadcastMessage(Protocol::createUserListMessage(arr));
    emit onlineUsersUpdated(onlineUsers);
}

void ChatServer::broadcastSystemMessage(const QString &content) {
    broadcastMessage(Protocol::createSystemMessage(content));
}

void ChatServer::muteUser(const QString &username) {
    QMutexLocker locker(&clientsMutex_);
    mutedUsers_.insert(username);
    locker.unlock();
    appendLog(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + "禁言用户: " + username);
    broadcastSystemMessage(username + " 已被管理员禁言");
    sendUserList();
    emit mutedUsersUpdated(mutedUsers_.values());
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        if (it.value()->isAuthenticated() && it.value()->username() == username) {
            it.value()->sendMessage(Protocol::createSystemMessage("你已被管理员禁言，无法发送消息"));
            break;
        }
    }
}

void ChatServer::unmuteUser(const QString &username) {
    QMutexLocker locker(&clientsMutex_);
    mutedUsers_.remove(username);
    locker.unlock();
    appendLog(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + "解除禁言: " + username);
    broadcastSystemMessage(username + " 已被管理员解除禁言");
    sendUserList();
    emit mutedUsersUpdated(mutedUsers_.values());
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        if (it.value()->isAuthenticated() && it.value()->username() == username) {
            it.value()->sendMessage(Protocol::createSystemMessage("你已被解除禁言，可以正常发言"));
            break;
        }
    }
}

bool ChatServer::isUserMuted(const QString &username) const {
    QMutexLocker locker(&clientsMutex_);
    return mutedUsers_.contains(username);
}

QStringList ChatServer::mutedUsers() const {
    QMutexLocker locker(&clientsMutex_);
    return mutedUsers_.values();
}

void ChatServer::serverBroadcast(const QString &content) {
    broadcastMessage(Protocol::createSystemMessage("【服务器公告】" + content));
    appendLog(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + "服务器公告已发送");
}

void ChatServer::appendLog(const QString &entry) {
    emit serverLogUpdated(entry);
}

void ChatServer::emitStats() {
    if (!startTime_.isValid()) return;
    qint64 secs = startTime_.secsTo(QDateTime::currentDateTime());
    int hours = int(secs) / 3600;
    int mins = (int(secs) % 3600) / 60;
    int s = int(secs) % 60;
    QString uptime;
    if (hours > 0) {
        uptime = QString("%1时%2分%3秒").arg(hours).arg(mins).arg(s);
    } else if (mins > 0) {
        uptime = QString("%1分%2秒").arg(mins).arg(s);
    } else {
        uptime = QString("%1秒").arg(s);
    }
    QMutexLocker locker(&clientsMutex_);
    emit serverStatsUpdated(clients_.size(), totalMessages_, uptime);
}
