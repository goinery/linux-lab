#include "chat_server.h"
#include "protocol.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QTemporaryDir>
#include <QThread>

#include <cstdlib>
#include <functional>

namespace {

struct Peer {
    QTcpSocket socket;
    QByteArray buffer;
    QList<QJsonObject> messages;
    bool protocolError = false;
};

void require(bool condition, const char *message) {
    if (!condition) {
        qCritical() << "FAILED:" << message;
        std::exit(EXIT_FAILURE);
    }
}

bool waitUntil(const std::function<bool()> &condition, int timeoutMs = 3000) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (condition()) {
            return true;
        }
        QThread::msleep(5);
    }
    return condition();
}

void initializePeer(Peer &peer) {
    QObject::connect(&peer.socket, &QTcpSocket::readyRead, [&peer]() {
        peer.buffer.append(peer.socket.readAll());
        while (true) {
            QJsonObject message;
            const Protocol::DecodeStatus status =
                Protocol::deserializeMessage(peer.buffer, message);
            if (status == Protocol::DecodeStatus::Complete) {
                peer.messages.append(message);
                continue;
            }
            if (status == Protocol::DecodeStatus::Invalid) {
                peer.protocolError = true;
            }
            break;
        }
    });
}

void connectPeer(Peer &peer, quint16 port) {
    peer.socket.connectToHost(QHostAddress::LocalHost, port);
    require(waitUntil([&peer]() {
        return peer.socket.state() == QAbstractSocket::ConnectedState;
    }), "client should connect to server");
}

void send(Peer &peer, const QJsonObject &message) {
    const QByteArray frame = Protocol::serializeMessage(message);
    require(!frame.isEmpty(), "test message should serialize");
    require(peer.socket.write(frame) == frame.size(), "test frame should be queued");
}

QJsonObject takeType(Peer &peer, const QString &type, int timeoutMs = 3000) {
    const bool received = waitUntil([&peer, &type]() {
        for (const QJsonObject &message : peer.messages) {
            if (message.value("type").toString() == type) {
                return true;
            }
        }
        return false;
    }, timeoutMs);
    require(received, "expected message type was not received");

    for (int i = 0; i < peer.messages.size(); ++i) {
        if (peer.messages.at(i).value("type").toString() == type) {
            return peer.messages.takeAt(i);
        }
    }
    return {};
}

void clearType(Peer &peer, const QString &type) {
    for (int i = peer.messages.size() - 1; i >= 0; --i) {
        if (peer.messages.at(i).value("type").toString() == type) {
            peer.messages.removeAt(i);
        }
    }
}

bool containsChatContent(const Peer &peer, const QString &content) {
    for (const QJsonObject &message : peer.messages) {
        const QString type = message.value("type").toString();
        if ((type == "chat" || type == "group_chat")
            && message.value("content").toString() == content) {
            return true;
        }
    }
    return false;
}

void registerAndLogin(Peer &peer, const QString &username) {
    send(peer, Protocol::createRegisterRequest(username, "pass1234"));
    require(takeType(peer, "register_response").value("success").toBool(),
            "registration should succeed");
    send(peer, Protocol::createLoginRequest(username, "pass1234"));
    require(takeType(peer, "login_response").value("success").toBool(),
            "login should succeed");
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QTemporaryDir temporaryDir;
    require(temporaryDir.isValid(), "temporary directory should be available");
    require(QDir::setCurrent(temporaryDir.path()), "should enter temporary directory");

    ChatServer server(0);
    require(server.start(), "server should start on an ephemeral port");
    require(server.listeningPort() != 0, "server should expose its listening port");

    Peer unauthenticated;
    initializePeer(unauthenticated);
    connectPeer(unauthenticated, server.listeningPort());
    send(unauthenticated,
         Protocol::createGroupChatMessage("forged-admin", "unauthenticated"));
    require(takeType(unauthenticated, "system").value("content")
                .toString().contains("登录"),
            "unauthenticated chat should be rejected");
    unauthenticated.socket.disconnectFromHost();

    Peer alice;
    Peer bob;
    initializePeer(alice);
    initializePeer(bob);
    connectPeer(alice, server.listeningPort());
    connectPeer(bob, server.listeningPort());
    registerAndLogin(alice, "alice");
    registerAndLogin(bob, "bob");

    const QString groupContent = "group sender verification";
    send(alice, {{"type", "group_chat"},
                 {"from", "forged-admin"},
                 {"content", groupContent}});
    const QJsonObject aliceGroup = takeType(alice, "group_chat");
    const QJsonObject bobGroup = takeType(bob, "group_chat");
    require(aliceGroup.value("from").toString() == "alice"
                && bobGroup.value("from").toString() == "alice",
            "server should replace forged group sender identity");

    const QString privateContent = "private sender verification";
    send(alice, {{"type", "chat"},
                 {"from", "forged-admin"},
                 {"to", "bob"},
                 {"content", privateContent}});
    const QJsonObject alicePrivate = takeType(alice, "chat");
    const QJsonObject bobPrivate = takeType(bob, "chat");
    require(alicePrivate.value("from").toString() == "alice"
                && bobPrivate.value("from").toString() == "alice",
            "server should replace forged private sender identity");

    server.muteUser("alice");
    clearType(bob, "group_chat");
    const QString blockedContent = "blocked by mute";
    send(alice, Protocol::createGroupChatMessage("alice", blockedContent));
    require(waitUntil([&alice]() {
        for (const QJsonObject &message : alice.messages) {
            if (message.value("type").toString() == "system"
                && message.value("content").toString().contains("禁言")) {
                return true;
            }
        }
        return false;
    }), "muted sender should receive feedback");
    waitUntil([]() { return false; }, 200);
    require(!containsChatContent(bob, blockedContent),
            "muted group message should not reach other users");

    require(!alice.protocolError && !bob.protocolError,
            "valid integration traffic should not cause protocol errors");
    server.stop();
    qInfo() << "PASS: authentication, sender binding, routing and mute control";
    return EXIT_SUCCESS;
}
