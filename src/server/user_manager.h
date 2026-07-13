#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <QString>
#include <QMap>
#include <QMutex>
#include <QByteArray>

struct UserInfo {
    QString username;
    QByteArray passwordHash;
    QByteArray passwordSalt;
};

class UserManager {
public:
    static UserManager &instance();

    bool registerUser(const QString &username, const QString &password);
    bool loginUser(const QString &username, const QString &password);
    bool userExists(const QString &username);

private:
    UserManager();
    UserManager(const UserManager &) = delete;
    UserManager &operator=(const UserManager &) = delete;

    static QByteArray generateSalt();
    static QByteArray hashPassword(const QString &password, const QByteArray &salt);
    void loadFromFile();
    bool saveToFile();

    QMutex mutex_;
    QMap<QString, UserInfo> users_;
};

#endif
