#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QByteArray>
#include <functional>

class ClientHandler : public QObject {
    Q_OBJECT

public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientHandler();

    void sendMessage(const QJsonObject &message);
    void disconnectClient();
    QString username() const { return username_; }
    void setUsername(const QString &name) { username_ = name; }
    bool isAuthenticated() const { return authenticated_; }
    void setAuthenticated(bool auth) { authenticated_ = auth; }

signals:
    void messageReceived(qintptr socketDescriptor, QJsonObject message);
    void disconnected(qintptr socketDescriptor);

public slots:
    void start();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *socket_;
    qintptr socketDescriptor_;
    QByteArray buffer_;
    QString username_;
    bool authenticated_;
};

#endif
