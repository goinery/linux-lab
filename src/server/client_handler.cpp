#include "client_handler.h"
#include "protocol.h"

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent), socket_(socket), socketDescriptor_(-1), authenticated_(false) {
    if (socket_) {
        socketDescriptor_ = socket_->socketDescriptor();
        socket_->setParent(this);
    }
}

ClientHandler::~ClientHandler() {
}

void ClientHandler::start() {
    if (!socket_) {
        emit disconnected(socketDescriptor_);
        return;
    }

    connect(socket_, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
}

void ClientHandler::onReadyRead() {
    if (!socket_) {
        return;
    }

    buffer_.append(socket_->readAll());
    QJsonObject json;
    while (Protocol::deserializeMessage(buffer_, json)) {
        emit messageReceived(socketDescriptor_, json);
    }
}

void ClientHandler::onDisconnected() {
    emit disconnected(socketDescriptor_);
}

void ClientHandler::sendMessage(const QJsonObject &message) {
    if (socket_ && socket_->state() == QAbstractSocket::ConnectedState) {
        socket_->write(Protocol::serializeMessage(message));
        socket_->flush();
    }
}

void ClientHandler::disconnectClient() {
    if (socket_ && socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
    }
}
