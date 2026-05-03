#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <QString>
#include <QMap>
#include <QMutex>
#include <QByteArray>

struct UserInfo {
    QString username;
    QByteArray passwordHash;
};

class UserManager {
public:
    static UserManager &instance();

    bool registerUser(const QString &username, const QString &password);
    bool loginUser(const QString &username, const QString &password);
    bool userExists(const QString &username);
    QStringList allUsernames();

private:
    UserManager();
    UserManager(const UserManager &) = delete;
    UserManager &operator=(const UserManager &) = delete;

    QByteArray hashPassword(const QString &password);
    void loadFromFile();
    void saveToFile();

    QMutex mutex_;
    QMap<QString, UserInfo> users_;
};

#endif
