#include "chat_client.h"
#include "protocol.h"
#include "constants.h"

ChatClient::ChatClient(QObject *parent)
    : QObject(parent), port_(0), autoReconnectEnabled_(false), reconnectAttempts_(0) {
    socket_ = new QTcpSocket(this);
    heartbeatTimer_ = new QTimer(this);
    reconnectTimer_ = new QTimer(this);
    reconnectTimer_->setSingleShot(true);

    connect(socket_, &QTcpSocket::connected, this, &ChatClient::onConnected);
    connect(socket_, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &ChatClient::onError);
    connect(heartbeatTimer_, &QTimer::timeout, this, &ChatClient::onHeartbeat);
    connect(reconnectTimer_, &QTimer::timeout, this, &ChatClient::onReconnectTimeout);
}

ChatClient::~ChatClient() {
    disconnectFromServer();
}

void ChatClient::connectToServer(const QString &host, quint16 port) {
    if (host.trimmed().isEmpty() || port == 0) {
        emit errorOccurred("服务器地址或端口无效");
        return;
    }

    autoReconnectEnabled_ = false;
    heartbeatTimer_->stop();
    reconnectTimer_->stop();
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->abort();
    }

    host_ = host.trimmed();
    port_ = port;
    buffer_.clear();
    reconnectAttempts_ = 0;
    autoReconnectEnabled_ = true;
    socket_->connectToHost(host_, port_);
}

void ChatClient::disconnectFromServer() {
    autoReconnectEnabled_ = false;
    heartbeatTimer_->stop();
    reconnectTimer_->stop();
    buffer_.clear();
    username_.clear();
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        if (socket_->state() == QAbstractSocket::ConnectedState) {
            socket_->disconnectFromHost();
        } else {
            socket_->abort();
        }
    }
}

void ChatClient::sendMessage(const QJsonObject &message) {
    if (socket_->state() == QAbstractSocket::ConnectedState) {
        const QByteArray frame = Protocol::serializeMessage(message);
        if (!frame.isEmpty()) {
            socket_->write(frame);
        }
    }
}

bool ChatClient::isConnected() const {
    return socket_->state() == QAbstractSocket::ConnectedState;
}

void ChatClient::onConnected() {
    buffer_.clear();
    reconnectAttempts_ = 0;
    reconnectTimer_->stop();
    heartbeatTimer_->start(Constants::HEARTBEAT_INTERVAL);
    emit connected();
}

void ChatClient::onReadyRead() {
    buffer_.append(socket_->readAll());
    while (true) {
        QJsonObject json;
        QString errorMessage;
        const Protocol::DecodeStatus status =
            Protocol::deserializeMessage(buffer_, json, &errorMessage);
        if (status == Protocol::DecodeStatus::Complete) {
            emit messageReceived(json);
            continue;
        }
        if (status == Protocol::DecodeStatus::Invalid) {
            emit errorOccurred("服务端协议错误: " + errorMessage);
            autoReconnectEnabled_ = false;
            socket_->abort();
        }
        break;
    }
}

void ChatClient::onDisconnected() {
    heartbeatTimer_->stop();
    buffer_.clear();
    username_.clear();
    emit disconnected();
    scheduleReconnect();
}

void ChatClient::onError(QAbstractSocket::SocketError) {
    emit errorOccurred(socket_->errorString());
    if (socket_->state() == QAbstractSocket::UnconnectedState) {
        scheduleReconnect();
    }
}

void ChatClient::onHeartbeat() {
    sendMessage(Protocol::createHeartbeat());
}

void ChatClient::scheduleReconnect() {
    if (!autoReconnectEnabled_ || reconnectTimer_->isActive()
        || socket_->state() != QAbstractSocket::UnconnectedState) {
        return;
    }
    if (reconnectAttempts_ >= Constants::MAX_RECONNECT_ATTEMPTS) {
        autoReconnectEnabled_ = false;
        emit errorOccurred(QString("重连失败，已达到最大尝试次数 %1")
                           .arg(Constants::MAX_RECONNECT_ATTEMPTS));
        return;
    }

    ++reconnectAttempts_;
    reconnectTimer_->start(Constants::RECONNECT_INTERVAL);
}

void ChatClient::onReconnectTimeout() {
    if (!autoReconnectEnabled_ || socket_->state() != QAbstractSocket::UnconnectedState) {
        return;
    }
    socket_->connectToHost(host_, port_);
}
