#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QMutex>
#include <QSet>
#include <QDateTime>
#include "client_handler.h"

class ChatServer : public QObject {
    Q_OBJECT

public:
    explicit ChatServer(quint16 port, QObject *parent = nullptr);
    ~ChatServer();
    bool start();
    void stop();
    quint16 listeningPort() const { return tcpServer_->serverPort(); }
    void muteUser(const QString &username);
    void unmuteUser(const QString &username);
    bool isUserMuted(const QString &username) const;
    void serverBroadcast(const QString &content);

signals:
    void onlineUsersUpdated(const QStringList &users);
    void mutedUsersUpdated(const QStringList &users);
    void serverStatsUpdated(int activeConnections, int totalMessages, const QString &uptime);
    void serverLogUpdated(const QString &entry);

private slots:
    void onNewConnection();
    void onMessageReceived(qintptr socketDescriptor, QJsonObject message);
    void onClientDisconnected(qintptr socketDescriptor);

private:
    void handleLogin(ClientHandler *handler, const QJsonObject &msg);
    void handleRegister(ClientHandler *handler, const QJsonObject &msg);
    void handleChatMessage(ClientHandler *handler, const QJsonObject &msg);
    void handleGroupChat(ClientHandler *handler, const QJsonObject &msg);
    void handleLogout(ClientHandler *handler);
    void broadcastMessage(const QJsonObject &message);
    void sendUserList();
    void broadcastSystemMessage(const QString &content);
    void appendLog(const QString &entry);
    void emitStats();

    QTcpServer *tcpServer_;
    QMap<qintptr, ClientHandler *> clients_;
    QSet<QString> mutedUsers_;
    mutable QMutex clientsMutex_;
    quint16 port_;
    QDateTime startTime_;
    int totalMessages_;
};

#endif
