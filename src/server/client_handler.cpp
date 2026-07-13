#include "client_handler.h"
#include "protocol.h"

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent), socket_(socket), socketDescriptor_(-1), authenticated_(false) {
    if (socket_) {
        socketDescriptor_ = socket_->socketDescriptor();
        socket_->setParent(this);
    }
}

ClientHandler::~ClientHandler() = default;

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
    while (true) {
        QJsonObject json;
        QString errorMessage;
        const Protocol::DecodeStatus status =
            Protocol::deserializeMessage(buffer_, json, &errorMessage);
        if (status == Protocol::DecodeStatus::Complete) {
            emit messageReceived(socketDescriptor_, json);
            continue;
        }
        if (status == Protocol::DecodeStatus::Invalid) {
            qWarning() << "Disconnecting client with invalid protocol frame:"
                       << errorMessage;
            socket_->disconnectFromHost();
        }
        break;
    }
}

void ClientHandler::onDisconnected() {
    emit disconnected(socketDescriptor_);
}

void ClientHandler::sendMessage(const QJsonObject &message) {
    if (socket_ && socket_->state() == QAbstractSocket::ConnectedState) {
        const QByteArray frame = Protocol::serializeMessage(message);
        if (!frame.isEmpty()) {
            socket_->write(frame);
        }
    }
}

void ClientHandler::disconnectClient() {
    if (socket_ && socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
    }
}
