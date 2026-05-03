#include "chat_client.h"
#include "protocol.h"
#include "constants.h"

ChatClient::ChatClient(QObject *parent)
    : QObject(parent), port_(0), reconnecting_(false), reconnectAttempts_(0) {
    socket_ = new QTcpSocket(this);
    heartbeatTimer_ = new QTimer(this);

    connect(socket_, &QTcpSocket::connected, this, &ChatClient::onConnected);
    connect(socket_, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &ChatClient::onError);
    connect(heartbeatTimer_, &QTimer::timeout, this, &ChatClient::onHeartbeat);
}

ChatClient::~ChatClient() {
    disconnectFromServer();
}

void ChatClient::connectToServer(const QString &host, quint16 port) {
    host_ = host;
    port_ = port;
    reconnectAttempts_ = 0;
    socket_->connectToHost(host, port);
}

void ChatClient::disconnectFromServer() {
    heartbeatTimer_->stop();
    reconnecting_ = false;
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
    }
}

void ChatClient::permanentDisconnect() {
    heartbeatTimer_->stop();
    reconnecting_ = false;
    reconnectAttempts_ = Constants::MAX_RECONNECT_ATTEMPTS;
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
    }
}

void ChatClient::sendMessage(const QJsonObject &message) {
    if (socket_->state() == QAbstractSocket::ConnectedState) {
        socket_->write(Protocol::serializeMessage(message));
        socket_->flush();
    }
}

bool ChatClient::isConnected() const {
    return socket_->state() == QAbstractSocket::ConnectedState;
}

void ChatClient::onConnected() {
    reconnectAttempts_ = 0;
    reconnecting_ = false;
    heartbeatTimer_->start(Constants::HEARTBEAT_INTERVAL);
    emit connected();
}

void ChatClient::onReadyRead() {
    buffer_.append(socket_->readAll());
    QJsonObject json;
    while (Protocol::deserializeMessage(buffer_, json)) {
        emit messageReceived(json);
    }
}

void ChatClient::onDisconnected() {
    heartbeatTimer_->stop();
    emit disconnected();
    attemptReconnect();
}

void ChatClient::onError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    emit errorOccurred(socket_->errorString());
}

void ChatClient::onHeartbeat() {
    sendMessage(Protocol::createHeartbeat());
}

void ChatClient::attemptReconnect() {
    if (reconnectAttempts_ >= Constants::MAX_RECONNECT_ATTEMPTS) return;
    reconnecting_ = true;
    reconnectAttempts_++;
    QTimer::singleShot(Constants::RECONNECT_INTERVAL, this, [this]() {
        if (reconnecting_) {
            socket_->connectToHost(host_, port_);
        }
    });
}
