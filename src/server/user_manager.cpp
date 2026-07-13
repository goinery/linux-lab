#include "user_manager.h"
#include "constants.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

#include <algorithm>

namespace {

bool isHexText(const QByteArray &value) {
    return !value.isEmpty()
        && std::all_of(value.cbegin(), value.cend(), [](char ch) {
            return (ch >= '0' && ch <= '9')
                || (ch >= 'a' && ch <= 'f')
                || (ch >= 'A' && ch <= 'F');
        });
}

} // namespace

UserManager &UserManager::instance() {
    static UserManager mgr;
    return mgr;
}

UserManager::UserManager() {
    loadFromFile();
}

QByteArray UserManager::generateSalt() {
    QByteArray salt(16, '\0');
    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = char(QRandomGenerator::system()->generate() & 0xFFU);
    }
    return salt;
}

QByteArray UserManager::hashPassword(const QString &password, const QByteArray &salt) {
    return QCryptographicHash::hash(salt + password.toUtf8(),
                                    QCryptographicHash::Sha256);
}

bool UserManager::registerUser(const QString &username, const QString &password) {
    QMutexLocker locker(&mutex_);
    if (users_.contains(username)) return false;
    UserInfo info;
    info.username = username;
    info.passwordSalt = generateSalt();
    info.passwordHash = hashPassword(password, info.passwordSalt);
    users_[username] = info;
    if (!saveToFile()) {
        users_.remove(username);
        return false;
    }
    return true;
}

bool UserManager::loginUser(const QString &username, const QString &password) {
    QMutexLocker locker(&mutex_);
    if (!users_.contains(username)) return false;
    UserInfo &info = users_[username];
    const bool success = info.passwordHash == hashPassword(password, info.passwordSalt);
    if (success && info.passwordSalt.isEmpty()) {
        info.passwordSalt = generateSalt();
        info.passwordHash = hashPassword(password, info.passwordSalt);
        saveToFile();
    }
    return success;
}

bool UserManager::userExists(const QString &username) {
    QMutexLocker locker(&mutex_);
    return users_.contains(username);
}

void UserManager::loadFromFile() {
    QFile file(Constants::USERS_FILE);
    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to read user database:" << file.errorString();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        qWarning() << "Ignoring invalid user database:" << parseError.errorString();
        return;
    }

    const QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        const QJsonObject obj = val.toObject();
        UserInfo info;
        info.username = obj["username"].toString().trimmed();
        info.passwordHash = QByteArray::fromHex(
            obj["password_hash"].toString().toLatin1());
        info.passwordSalt = QByteArray::fromHex(
            obj["password_salt"].toString().toLatin1());

        // 兼容旧版本中被二次十六进制编码的 SHA-256 文本。
        if (info.passwordSalt.isEmpty() && info.passwordHash.size() == 64
            && isHexText(info.passwordHash)) {
            info.passwordHash = QByteArray::fromHex(info.passwordHash);
        }

        if (info.username.isEmpty() || info.passwordHash.size() != 32
            || (!info.passwordSalt.isEmpty() && info.passwordSalt.size() != 16)) {
            qWarning() << "Skipping invalid user record:" << info.username;
            continue;
        }
        users_[info.username] = info;
    }
}

bool UserManager::saveToFile() {
    QJsonArray arr;
    for (auto it = users_.begin(); it != users_.end(); ++it) {
        QJsonObject obj;
        obj["username"] = it->username;
        obj["password_hash"] = QString(it->passwordHash.toHex());
        obj["password_salt"] = QString(it->passwordSalt.toHex());
        arr.append(obj);
    }

    QSaveFile file(Constants::USERS_FILE);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to write user database:" << file.errorString();
        return false;
    }
    const QByteArray contents = QJsonDocument(arr).toJson();
    if (file.write(contents) != contents.size() || !file.commit()) {
        qWarning() << "Unable to commit user database:" << file.errorString();
        return false;
    }
    return true;
}
