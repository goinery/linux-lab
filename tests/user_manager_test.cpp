#include "user_manager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <cstdlib>

namespace {

void require(bool condition, const char *message) {
    if (!condition) {
        qCritical() << "FAILED:" << message;
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QTemporaryDir temporaryDir;
    require(temporaryDir.isValid(), "temporary directory should be available");
    require(QDir::setCurrent(temporaryDir.path()), "should enter temporary directory");

    // 构造旧版本的二次十六进制编码记录，验证登录后自动迁移。
    const QByteArray legacyHash = QCryptographicHash::hash(
        QByteArray("pass1234"), QCryptographicHash::Sha256).toHex();
    const QJsonArray legacyUsers{
        QJsonObject{{"username", "alice"},
                    {"password_hash", QString(legacyHash.toHex())}}
    };
    QFile legacyFile("users.json");
    require(legacyFile.open(QIODevice::WriteOnly), "legacy database should be writable");
    require(legacyFile.write(QJsonDocument(legacyUsers).toJson()) > 0,
            "legacy database should be created");
    legacyFile.close();

    UserManager &users = UserManager::instance();
    require(users.loginUser("alice", "pass1234"), "legacy alice login should pass");
    require(users.registerUser("bob", "pass1234"), "bob registration should pass");
    require(!users.registerUser("alice", "otherpass"), "duplicate registration should fail");
    require(users.loginUser("alice", "pass1234"), "correct password should pass");
    require(!users.loginUser("alice", "wrongpass"), "wrong password should fail");

    QFile file("users.json");
    require(file.open(QIODevice::ReadOnly), "users.json should be readable");
    const QByteArray databaseBytes = file.readAll();
    const QJsonDocument document = QJsonDocument::fromJson(databaseBytes);
    require(document.isArray() && document.array().size() == 2,
            "database should contain two users");

    const QJsonObject alice = document.array().at(0).toObject();
    const QJsonObject bob = document.array().at(1).toObject();
    require(!alice.value("password_salt").toString().isEmpty(),
            "alice should have a password salt");
    require(alice.value("password_hash").toString()
                != bob.value("password_hash").toString(),
            "equal passwords should have different salted hashes");
    require(!databaseBytes.contains("pass1234"),
            "database should not contain the plain-text password");

    qInfo() << "PASS: user registration, salted hashes and atomic persistence";
    return EXIT_SUCCESS;
}
