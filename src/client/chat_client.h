#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QTimer>
#include <QByteArray>

class ChatClient : public QObject {
    Q_OBJECT

public:
    explicit ChatClient(QObject *parent = nullptr);
    ~ChatClient();

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    void permanentDisconnect();
    void sendMessage(const QJsonObject &message);
    bool isConnected() const;
    void setUsername(const QString &username) { username_ = username; }
    QString username() const { return username_; }

signals:
    void connected();
    void disconnected();
    void messageReceived(QJsonObject message);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onHeartbeat();

private:
    void attemptReconnect();

    QTcpSocket *socket_;
    QByteArray buffer_;
    QTimer *heartbeatTimer_;
    QString host_;
    quint16 port_;
    QString username_;
    bool reconnecting_;
    int reconnectAttempts_;
};

#endif
