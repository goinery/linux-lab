#include "user_manager.h"
#include "constants.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QCryptographicHash>

UserManager &UserManager::instance() {
    static UserManager mgr;
    return mgr;
}

UserManager::UserManager() {
    loadFromFile();
}

QByteArray UserManager::hashPassword(const QString &password) {
    return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
}

bool UserManager::registerUser(const QString &username, const QString &password) {
    QMutexLocker locker(&mutex_);
    if (users_.contains(username)) return false;
    UserInfo info;
    info.username = username;
    info.passwordHash = hashPassword(password);
    users_[username] = info;
    saveToFile();
    return true;
}

bool UserManager::loginUser(const QString &username, const QString &password) {
    QMutexLocker locker(&mutex_);
    if (!users_.contains(username)) return false;
    return users_[username].passwordHash == hashPassword(password);
}

bool UserManager::userExists(const QString &username) {
    QMutexLocker locker(&mutex_);
    return users_.contains(username);
}

QStringList UserManager::allUsernames() {
    QMutexLocker locker(&mutex_);
    return users_.keys();
}

void UserManager::loadFromFile() {
    QFile file(Constants::USERS_FILE);
    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray()) return;
    QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();
        UserInfo info;
        info.username = obj["username"].toString();
        info.passwordHash = QByteArray::fromHex(obj["password_hash"].toString().toUtf8());
        users_[info.username] = info;
    }
}

void UserManager::saveToFile() {
    QJsonArray arr;
    for (auto it = users_.begin(); it != users_.end(); ++it) {
        QJsonObject obj;
        obj["username"] = it->username;
        obj["password_hash"] = QString(it->passwordHash.toHex());
        arr.append(obj);
    }
    QFile file(Constants::USERS_FILE);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(arr).toJson());
    file.close();
}
