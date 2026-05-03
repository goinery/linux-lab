#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>

class ChatClient;

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(ChatClient *client, QWidget *parent = nullptr);

signals:
    void loginSuccess(const QString &username);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onSwitchToRegister();
    void onSwitchToLogin();
    void onMessageReceived(QJsonObject message);

private:
    void setupUI();
    void connectSignals();

    ChatClient *client_;

    QStackedWidget *stackedWidget_;

    QWidget *loginPage_;
    QLineEdit *loginUsernameEdit_;
    QLineEdit *loginPasswordEdit_;
    QPushButton *loginButton_;
    QPushButton *switchToRegisterButton_;
    QLabel *loginStatusLabel_;

    QWidget *registerPage_;
    QLineEdit *regUsernameEdit_;
    QLineEdit *regPasswordEdit_;
    QLineEdit *regConfirmPasswordEdit_;
    QPushButton *registerButton_;
    QPushButton *switchToLoginButton_;
    QLabel *regStatusLabel_;
};

#endif
