#include "chat_client.h"
#include "constants.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

#include <cstdlib>
#include <functional>

namespace {

void require(bool condition, const char *message) {
    if (!condition) {
        qCritical() << "FAILED:" << message;
        std::exit(EXIT_FAILURE);
    }
}

bool waitUntil(const std::function<bool()> &condition, int timeoutMs) {
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

QTcpSocket *acceptConnection(QTcpServer &server) {
    require(waitUntil([&server]() { return server.hasPendingConnections(); }, 2000),
            "server should receive a connection");
    QTcpSocket *socket = server.nextPendingConnection();
    require(socket != nullptr, "accepted socket should exist");
    return socket;
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QTcpServer server;
    require(server.listen(QHostAddress::LocalHost, 0),
            "test server should listen on an ephemeral port");

    ChatClient client;
    int connectedCount = 0;
    QObject::connect(&client, &ChatClient::connected,
                     [&connectedCount]() { ++connectedCount; });

    client.connectToServer("127.0.0.1", server.serverPort());
    require(waitUntil([&client]() { return client.isConnected(); }, 2000),
            "initial client connection should succeed");
    QTcpSocket *firstPeer = acceptConnection(server);

    client.disconnectFromServer();
    require(waitUntil([&client]() { return !client.isConnected(); }, 2000),
            "intentional disconnect should complete");
    waitUntil([]() { return false; }, Constants::RECONNECT_INTERVAL + 250);
    require(connectedCount == 1 && !client.isConnected()
                && !server.hasPendingConnections(),
            "intentional disconnect must not trigger automatic reconnect");
    firstPeer->deleteLater();

    client.connectToServer("127.0.0.1", server.serverPort());
    require(waitUntil([&client]() { return client.isConnected(); }, 2000),
            "second client connection should succeed");
    QTcpSocket *secondPeer = acceptConnection(server);
    require(connectedCount == 2, "second connection should emit connected once");

    secondPeer->abort();
    require(waitUntil([&client]() { return !client.isConnected(); }, 2000),
            "unexpected server-side close should disconnect client");
    require(waitUntil([&client, &connectedCount]() {
        return client.isConnected() && connectedCount == 3;
    }, Constants::RECONNECT_INTERVAL + 2500),
            "unexpected disconnect should trigger automatic reconnect");
    QTcpSocket *reconnectedPeer = acceptConnection(server);

    client.disconnectFromServer();
    reconnectedPeer->deleteLater();
    qInfo() << "PASS: intentional disconnect and automatic reconnect behavior";
    return EXIT_SUCCESS;
}
