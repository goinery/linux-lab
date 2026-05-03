#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QMutex>
#include <QSet>
#include <QDateTime>
#include "thread_pool.h"
#include "client_handler.h"

class ChatServer : public QObject {
    Q_OBJECT

public:
    explicit ChatServer(quint16 port, QObject *parent = nullptr);
    ~ChatServer();
    bool start();
    void stop();
    void muteUser(const QString &username);
    void unmuteUser(const QString &username);
    bool isUserMuted(const QString &username) const;
    QStringList mutedUsers() const;
    void serverBroadcast(const QString &content);

signals:
    void onlineUsersUpdated(const QStringList &users);
    void mutedUsersUpdated(const QStringList &users);
    void serverStatsUpdated(int totalConnections, int totalMessages, const QString &uptime);
    void serverLogUpdated(const QString &entry);

private slots:
    void onNewConnection();
    void onMessageReceived(qintptr socketDescriptor, QJsonObject message);
    void onClientDisconnected(qintptr socketDescriptor);

private:
    void handleLogin(ClientHandler *handler, const QJsonObject &msg);
    void handleRegister(ClientHandler *handler, const QJsonObject &msg);
    void handleChatMessage(const QJsonObject &msg);
    void handleGroupChat(const QJsonObject &msg);
    void handleLogout(qintptr socketDescriptor, const QJsonObject &msg);
    void broadcastMessage(const QJsonObject &message);
    void sendUserList();
    void broadcastSystemMessage(const QString &content);
    void appendLog(const QString &entry);
    void emitStats();

    QTcpServer *tcpServer_;
    ThreadPool threadPool_;
    QMap<qintptr, ClientHandler *> clients_;
    QSet<QString> mutedUsers_;
    mutable QMutex clientsMutex_;
    quint16 port_;
    QDateTime startTime_;
    int totalConnections_;
    int totalMessages_;
};

#endif
