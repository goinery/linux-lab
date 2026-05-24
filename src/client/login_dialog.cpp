#include "login_dialog.h"
#include "cursor_manager.h"
#include "chat_client.h"
#include "protocol.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QScreen>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

LoginDialog::LoginDialog(ChatClient *client, QWidget *parent)
    : QDialog(parent), client_(client) {
    setWindowTitle("ChatRoom - 登录");
    setMinimumSize(380, 420);
    resize(440, 500);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
    connectSignals();
}

void LoginDialog::setupUI() {
    stackedWidget_ = new QStackedWidget(this);

    auto createLineEdit = [](const QString &placeholder, bool isPassword = false) {
        QLineEdit *edit = new QLineEdit;
        edit->setPlaceholderText(placeholder);
        edit->setMinimumHeight(44);
        edit->setAttribute(Qt::WA_InputMethodEnabled, true);
        edit->setInputMethodHints(Qt::ImhNone);
        if (isPassword) edit->setEchoMode(QLineEdit::Password);
        return edit;
    };

    loginPage_ = new QWidget;
    {
        QVBoxLayout *layout = new QVBoxLayout(loginPage_);
        layout->setSpacing(12);
        layout->setContentsMargins(48, 48, 48, 48);

        QLabel *titleLabel = new QLabel("💬 ChatRoom");
        titleLabel->setObjectName("titleLabel");
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont titleFont;
        titleFont.setPointSize(26);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);

        QLabel *subtitleLabel = new QLabel("欢迎回来，请登录你的账号");
        subtitleLabel->setObjectName("subtitleLabel");
        subtitleLabel->setAlignment(Qt::AlignCenter);

        loginUsernameEdit_ = createLineEdit("用户名");
        loginPasswordEdit_ = createLineEdit("密码", true);

        loginButton_ = new QPushButton("登 录");
        loginButton_->setObjectName("primaryButton");
        loginButton_->setMinimumHeight(48);
        loginButton_->setCursor(CursorManager::instance().hand());

        loginStatusLabel_ = new QLabel;
        loginStatusLabel_->setObjectName("statusLabel");
        loginStatusLabel_->setAlignment(Qt::AlignCenter);
        loginStatusLabel_->setWordWrap(true);
        loginStatusLabel_->setMinimumHeight(20);

        switchToRegisterButton_ = new QPushButton("还没有账号？点击注册");
        switchToRegisterButton_->setObjectName("linkButton");
        switchToRegisterButton_->setCursor(CursorManager::instance().hand());
        switchToRegisterButton_->setFlat(true);

        layout->addWidget(titleLabel);
        layout->addWidget(subtitleLabel);
        layout->addSpacing(24);
        layout->addWidget(loginUsernameEdit_);
        layout->addWidget(loginPasswordEdit_);
        layout->addSpacing(8);
        layout->addWidget(loginButton_);
        layout->addWidget(loginStatusLabel_);
        layout->addSpacing(12);
        layout->addWidget(switchToRegisterButton_);
        layout->addStretch();
    }

    registerPage_ = new QWidget;
    {
        QVBoxLayout *layout = new QVBoxLayout(registerPage_);
        layout->setSpacing(12);
        layout->setContentsMargins(48, 48, 48, 48);

        QLabel *titleLabel = new QLabel("💬 ChatRoom");
        titleLabel->setObjectName("titleLabel");
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont titleFont;
        titleFont.setPointSize(26);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);

        QLabel *subtitleLabel = new QLabel("创建新账号，开始聊天");
        subtitleLabel->setObjectName("subtitleLabel");
        subtitleLabel->setAlignment(Qt::AlignCenter);

        regUsernameEdit_ = createLineEdit("用户名（至少2个字符）");
        regPasswordEdit_ = createLineEdit("密码（至少4个字符）", true);
        regConfirmPasswordEdit_ = createLineEdit("确认密码", true);

        registerButton_ = new QPushButton("注 册");
        registerButton_->setObjectName("primaryButton");
        registerButton_->setMinimumHeight(48);
        registerButton_->setCursor(CursorManager::instance().hand());

        regStatusLabel_ = new QLabel;
        regStatusLabel_->setObjectName("statusLabel");
        regStatusLabel_->setAlignment(Qt::AlignCenter);
        regStatusLabel_->setWordWrap(true);
        regStatusLabel_->setMinimumHeight(20);

        switchToLoginButton_ = new QPushButton("已有账号？点击登录");
        switchToLoginButton_->setObjectName("linkButton");
        switchToLoginButton_->setCursor(CursorManager::instance().hand());
        switchToLoginButton_->setFlat(true);

        layout->addWidget(titleLabel);
        layout->addWidget(subtitleLabel);
        layout->addSpacing(24);
        layout->addWidget(regUsernameEdit_);
        layout->addWidget(regPasswordEdit_);
        layout->addWidget(regConfirmPasswordEdit_);
        layout->addSpacing(8);
        layout->addWidget(registerButton_);
        layout->addWidget(regStatusLabel_);
        layout->addSpacing(12);
        layout->addWidget(switchToLoginButton_);
        layout->addStretch();
    }

    stackedWidget_->addWidget(loginPage_);
    stackedWidget_->addWidget(registerPage_);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(stackedWidget_);
}

void LoginDialog::connectSignals() {
    connect(loginButton_, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerButton_, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(switchToRegisterButton_, &QPushButton::clicked, this, &LoginDialog::onSwitchToRegister);
    connect(switchToLoginButton_, &QPushButton::clicked, this, &LoginDialog::onSwitchToLogin);
    connect(client_, &ChatClient::messageReceived, this, &LoginDialog::onMessageReceived);

    connect(loginPasswordEdit_, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(regConfirmPasswordEdit_, &QLineEdit::returnPressed, this, &LoginDialog::onRegisterClicked);
}

void LoginDialog::onLoginClicked() {
    QString username = loginUsernameEdit_->text().trimmed();
    QString password = loginPasswordEdit_->text();

    if (username.isEmpty() || password.isEmpty()) {
        loginStatusLabel_->setText("⚠ 请输入用户名和密码");
        loginStatusLabel_->setStyleSheet("color: #e74c3c;");
        return;
    }

    if (!client_->isConnected()) {
        loginStatusLabel_->setText("⚠ 未连接到服务器，请稍候...");
        loginStatusLabel_->setStyleSheet("color: #e74c3c;");
        return;
    }

    loginButton_->setEnabled(false);
    loginStatusLabel_->setText("正在登录...");
    loginStatusLabel_->setStyleSheet("color: #7f8c8d;");
    client_->sendMessage(Protocol::createLoginRequest(username, password));
}

void LoginDialog::onRegisterClicked() {
    QString username = regUsernameEdit_->text().trimmed();
    QString password = regPasswordEdit_->text();
    QString confirm = regConfirmPasswordEdit_->text();

    if (username.isEmpty() || password.isEmpty() || confirm.isEmpty()) {
        regStatusLabel_->setText("⚠ 请填写所有字段");
        regStatusLabel_->setStyleSheet("color: #e74c3c;");
        return;
    }
    if (username.length() < 2) {
        regStatusLabel_->setText("⚠ 用户名至少2个字符");
        regStatusLabel_->setStyleSheet("color: #e74c3c;");
        return;
    }
    if (password.length() < 4) {
        regStatusLabel_->setText("⚠ 密码至少4个字符");
        regStatusLabel_->setStyleSheet("color: #e74c3c;");
        return;
    }
    if (password != confirm) {
        regStatusLabel_->setText("⚠ 两次密码输入不一致");
        regStatusLabel_->setStyleSheet("color: #e74c3c;");
        return;
    }

    registerButton_->setEnabled(false);
    regStatusLabel_->setText("正在注册...");
    regStatusLabel_->setStyleSheet("color: #7f8c8d;");
    client_->sendMessage(Protocol::createRegisterRequest(username, password));
}

void LoginDialog::onSwitchToRegister() {
    stackedWidget_->setCurrentWidget(registerPage_);
    setWindowTitle("ChatRoom - 注册");
    loginStatusLabel_->clear();
}

void LoginDialog::onSwitchToLogin() {
    stackedWidget_->setCurrentWidget(loginPage_);
    setWindowTitle("ChatRoom - 登录");
    regStatusLabel_->clear();
}

void LoginDialog::onMessageReceived(QJsonObject message) {
    QString type = message["type"].toString();

    if (type == "login_response") {
        loginButton_->setEnabled(true);
        bool success = message["success"].toBool();
        QString msg = message["message"].toString();
        if (success) {
            client_->setUsername(loginUsernameEdit_->text().trimmed());
            emit loginSuccess(loginUsernameEdit_->text().trimmed());
            accept();
        } else {
            loginStatusLabel_->setText("⚠ " + msg);
            loginStatusLabel_->setStyleSheet("color: #e74c3c;");
        }
    } else if (type == "register_response") {
        registerButton_->setEnabled(true);
        bool success = message["success"].toBool();
        QString msg = message["message"].toString();
        if (success) {
            regStatusLabel_->setText("✓ " + msg + " 即将跳转登录...");
            regStatusLabel_->setStyleSheet("color: #27ae60;");
            QTimer::singleShot(1500, this, [this]() {
                onSwitchToLogin();
                loginUsernameEdit_->setText(regUsernameEdit_->text());
                regUsernameEdit_->clear();
                regPasswordEdit_->clear();
                regConfirmPasswordEdit_->clear();
            });
        } else {
            regStatusLabel_->setText("⚠ " + msg);
            regStatusLabel_->setStyleSheet("color: #e74c3c;");
        }
    }
}
