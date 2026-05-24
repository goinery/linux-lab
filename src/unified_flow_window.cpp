#include "unified_flow_window.h"
#include "cursor_manager.h"
#include "theme_manager.h"

#include <QApplication>
#include <QAbstractButton>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHostAddress>
#include <QJsonObject>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkInterface>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QWindow>

#include "chat_client.h"
#include "chat_server.h"
#include "constants.h"
#include "mainwindow.h"
#include "protocol.h"

UnifiedFlowWindow::UnifiedFlowWindow(const QString &defaultHost, quint16 defaultPort,
                                     bool defaultServerMode, QWidget *parent)
    : QMainWindow(parent), client_(new ChatClient(this)), server_(nullptr),
      chatPage_(nullptr), themeIndex_(0), pendingAuthType_(None),
      dragging_(false), maximized_(false) {
    setWindowTitle(Constants::APP_NAME + " " + Constants::APP_VERSION);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint | Qt::Window);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMinimumSize(800, 540);

    buildUi(defaultHost, defaultPort, defaultServerMode);
    centerToHalfScreen();
    applyTheme();

    connect(client_, &ChatClient::connected, this, &UnifiedFlowWindow::onClientConnected);
    connect(client_, &ChatClient::disconnected, this, &UnifiedFlowWindow::onClientDisconnected);
    connect(client_, &ChatClient::errorOccurred, this, &UnifiedFlowWindow::onClientError);
    connect(client_, &ChatClient::messageReceived, this, &UnifiedFlowWindow::onClientMessageReceived);
}

void UnifiedFlowWindow::onModeChanged() {
    const bool serverMode = serverModeRadio_->isChecked();
    serverFields_->setVisible(serverMode);
    clientFields_->setVisible(!serverMode);
}

void UnifiedFlowWindow::onNextClicked() {
    startupStatusLabel_->setText("");
    bool ok = false;

    if (serverModeRadio_->isChecked()) {
        int p = serverPortEdit_->text().trimmed().toInt(&ok);
        if (!ok || p <= 0 || p > 65535) {
            startupStatusLabel_->setText("请输入有效的服务端口(1-65535)");
            return;
        }

        if (server_ != nullptr) {
            server_->deleteLater();
            server_ = nullptr;
        }

        server_ = new ChatServer(quint16(p), this);
        connect(server_, &ChatServer::onlineUsersUpdated, this, &UnifiedFlowWindow::onServerUsersUpdated);
        connect(server_, &ChatServer::mutedUsersUpdated, this, &UnifiedFlowWindow::onServerMutedUpdated);
        connect(server_, &ChatServer::serverStatsUpdated, this, &UnifiedFlowWindow::onServerStatsUpdated);
        connect(server_, &ChatServer::serverLogUpdated, this, &UnifiedFlowWindow::onServerLogUpdated);

        if (!server_->start()) {
            startupStatusLabel_->setText("服务端启动失败，请检查端口是否被占用");
            return;
        }

        QStringList ips;
        for (const QHostAddress &addr : QNetworkInterface::allAddresses()) {
            if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost) {
                ips << addr.toString();
            }
        }
        if (ips.isEmpty()) {
            ips << "127.0.0.1";
        }

        serverIpLabel_->setText(QString("地址: %1").arg(ips.join(", ")));
        serverPortLabel_->setText(QString("端口: %1").arg(p));
        serverInfoLabel_->setText("服务端已启动，等待客户端连接...");
        onlineUsersList_->clear();
        setCurrentPage(serverPage_, "服务端监控");
        return;
    }

    const QString host = clientHostEdit_->text().trimmed();
    int p = clientPortEdit_->text().trimmed().toInt(&ok);
    if (host.isEmpty()) {
        startupStatusLabel_->setText("请输入服务端地址");
        return;
    }
    if (!ok || p <= 0 || p > 65535) {
        startupStatusLabel_->setText("请输入有效的服务端口(1-65535)");
        return;
    }

    endpointLabel_->setText(QString("目标服务器: %1:%2").arg(host).arg(p));
    authStatusLabel_->setText("正在连接服务器...");
    authActionButton_->setEnabled(false);
    pendingHost_ = host;
    pendingPort_ = quint16(p);
    client_->connectToServer(host, quint16(p));
    setCurrentPage(authPage_, "客户端认证");
}

void UnifiedFlowWindow::onBackToStart() {
    if (client_->isConnected()) {
        client_->disconnectFromServer();
    }
    pendingAuthType_ = None;
    authStatusLabel_->setText("");
    setCurrentPage(startupPage_, "启动页");
}

void UnifiedFlowWindow::onAuthModeChanged() {
    const bool loginMode = loginModeRadio_->isChecked();
    loginFields_->setVisible(loginMode);
    registerFields_->setVisible(!loginMode);
    authActionButton_->setText(loginMode ? "登录" : "注册");
    authStatusLabel_->setText("");
}

void UnifiedFlowWindow::onAuthAction() {
    if (!client_->isConnected()) {
        authStatusLabel_->setText("尚未连接服务器，请返回重新连接");
        return;
    }

    if (loginModeRadio_->isChecked()) {
        const QString user = loginUserEdit_->text().trimmed();
        const QString pass = loginPassEdit_->text();
        if (user.isEmpty() || pass.isEmpty()) {
            authStatusLabel_->setText("请输入用户名和密码");
            return;
        }
        pendingAuthType_ = Login;
        authStatusLabel_->setText("正在登录...");
        client_->sendMessage(Protocol::createLoginRequest(user, pass));
    } else {
        const QString user = regUserEdit_->text().trimmed();
        const QString pass = regPassEdit_->text();
        const QString confirm = regConfirmEdit_->text();
        if (user.length() < 2) {
            authStatusLabel_->setText("用户名至少2个字符");
            return;
        }
        if (pass.length() < 4) {
            authStatusLabel_->setText("密码至少4个字符");
            return;
        }
        if (pass != confirm) {
            authStatusLabel_->setText("两次输入的密码不一致");
            return;
        }
        pendingAuthType_ = Register;
        authStatusLabel_->setText("正在注册...");
        client_->sendMessage(Protocol::createRegisterRequest(user, pass));
    }
}

void UnifiedFlowWindow::onClientConnected() {
    authStatusLabel_->setText("连接成功，请登录或注册");
    authActionButton_->setEnabled(true);
}

void UnifiedFlowWindow::onClientDisconnected() {
    authActionButton_->setEnabled(false);
    authStatusLabel_->setText("已断开连接");
}

void UnifiedFlowWindow::onClientError(const QString &err) {
    authActionButton_->setEnabled(false);
    authStatusLabel_->setText("连接失败: " + err);
}

void UnifiedFlowWindow::onClientMessageReceived(const QJsonObject &msg) {
    const QString type = msg["type"].toString();
    if (type == "login_response") {
        bool success = msg["success"].toBool();
        QString text = msg["message"].toString();
        if (success) {
            client_->setUsername(loginUserEdit_->text().trimmed());
            authStatusLabel_->setText("登录成功，正在进入聊天页...");
            if (!chatPage_) {
                chatPage_ = new MainWindow(client_, this);
                chatPage_->setObjectName("embeddedChatPage");
                pages_->addWidget(chatPage_);
            }
            setCurrentPage(chatPage_, "聊天室");
        } else {
            authStatusLabel_->setText(text);
        }
        pendingAuthType_ = None;
    } else if (type == "register_response") {
        bool success = msg["success"].toBool();
        QString text = msg["message"].toString();
        authStatusLabel_->setText(text);
        if (success) {
            loginModeRadio_->setChecked(true);
            loginUserEdit_->setText(regUserEdit_->text().trimmed());
            regPassEdit_->clear();
            regConfirmEdit_->clear();
            onAuthModeChanged();
        }
        pendingAuthType_ = None;
    }
}

void UnifiedFlowWindow::onToggleTheme() {
    setTheme((themeIndex_ + 1) % ThemeManager::themeCount());
}

void UnifiedFlowWindow::setTheme(int index) {
    themeIndex_ = index;
    applyTheme();
}

void UnifiedFlowWindow::onWinMinimize() { showMinimized(); }

void UnifiedFlowWindow::onWinMaxRestore() {
    if (maximized_) {
        setGeometry(normalGeometry_);
        maximized_ = false;
        if (maximizeBtn_) maximizeBtn_->setText("□");
    } else {
        normalGeometry_ = geometry();
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            setGeometry(screen->availableGeometry());
        }
        maximized_ = true;
        if (maximizeBtn_) maximizeBtn_->setText("⊞");
    }
}

void UnifiedFlowWindow::onWinClose() { close(); }

void UnifiedFlowWindow::onServerUsersUpdated(const QStringList &users) {
    onlineUsersList_->clear();
    for (const QString &u : users) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setData(Qt::UserRole, u);
        item->setText(server_ && server_->isUserMuted(u) ? "[禁] " + u : u);
        onlineUsersList_->addItem(item);
    }
    serverInfoLabel_->setText(QString("当前在线人数: %1").arg(users.size()));
}

void UnifiedFlowWindow::onServerMutedUpdated(const QStringList &mutedUsers) {
    QSet<QString> mutedSet(mutedUsers.begin(), mutedUsers.end());
    for (int i = 0; i < onlineUsersList_->count(); ++i) {
        QListWidgetItem *item = onlineUsersList_->item(i);
        QString username = item->data(Qt::UserRole).toString();
        item->setText(mutedSet.contains(username) ? "[禁] " + username : username);
    }
}

void UnifiedFlowWindow::onServerStatsUpdated(int totalConnections, int totalMessages,
                                             const QString &uptime) {
    serverConnectionsLabel_->setText(QString("连接数: %1").arg(totalConnections));
    serverMessagesLabel_->setText(QString("消息数: %1").arg(totalMessages));
    serverUptimeLabel_->setText(QString("运行时间: %1").arg(uptime));
}

void UnifiedFlowWindow::onServerLogUpdated(const QString &entry) {
    if (!serverLogList_) return;
    serverLogList_->addItem(entry);
    serverLogList_->scrollToBottom();
}

QWidget *UnifiedFlowWindow::cardWidget(const QString &title) {
    QFrame *card = new QFrame;
    card->setObjectName("card");
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setObjectName("cardTitle");
    layout->addWidget(titleLabel);
    return card;
}

void UnifiedFlowWindow::stopServerIfRunning() {
    if (server_ != nullptr) {
        server_->stop();
        server_->deleteLater();
        server_ = nullptr;
    }
}

void UnifiedFlowWindow::setCurrentPage(QWidget *page, const QString &subtitle) {
    pages_->setCurrentWidget(page);
    if (topBarSubtitleLabel_) {
        topBarSubtitleLabel_->setText(subtitle);
    }
}

void UnifiedFlowWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (maximizeBtn_) {
            maximizeBtn_->setText(maximized_ ? "⊞" : "□");
        }
    }
    QMainWindow::changeEvent(event);
}

void UnifiedFlowWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    resize(qMax(width(), 800), qMax(height(), 540));
}

bool UnifiedFlowWindow::eventFilter(QObject *obj, QEvent *event) {
    QWidget *w = qobject_cast<QWidget *>(obj);
    const bool inTitleBar = titleBar_ != nullptr
        && w != nullptr
        && (w == titleBar_ || titleBar_->isAncestorOf(w));
    const bool titleBarAction = inTitleBar && qobject_cast<QAbstractButton *>(w) == nullptr;

    if (titleBarAction) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                dragStartPos_ = me->globalPos() - frameGeometry().topLeft();
                dragging_ = true;
                titleBar_->setCursor(CursorManager::instance().move());
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            titleBar_->setCursor(CursorManager::instance().move());
            if (dragging_) {
                move(me->globalPos() - dragStartPos_);
            }
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            dragging_ = false;
            titleBar_->setCursor(CursorManager::instance().move());
            return true;
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            onWinMaxRestore();
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
        if (w && !maximized_ && isAncestorOf(w)) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            const QPoint local = mapFromGlobal(me->globalPos());
            if (event->type() == QEvent::MouseMove) {
                setEdgeCursor(calcEdge(local, size()));
            } else if (me->button() == Qt::LeftButton) {
                Qt::Edges e = calcEdge(local, size());
                if (e != 0 && windowHandle()) {
                    windowHandle()->startSystemResize(e);
                    return true;
                }
            }
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        dragging_ = false;
    }

    return QMainWindow::eventFilter(obj, event);
}

Qt::Edges UnifiedFlowWindow::calcEdge(const QPoint &pos, const QSize &sz) const {
    const int m = 10;
    int e = 0;
    if (pos.x() <= m) e |= Qt::LeftEdge;
    if (pos.x() >= sz.width() - m) e |= Qt::RightEdge;
    if (pos.y() <= m) e |= Qt::TopEdge;
    if (pos.y() >= sz.height() - m) e |= Qt::BottomEdge;
    return Qt::Edges(e);
}

void UnifiedFlowWindow::setEdgeCursor(Qt::Edges e) {
    auto &cm = CursorManager::instance();
    switch (e) {
    case Qt::LeftEdge: case Qt::RightEdge:
        setCursor(cm.sizeHor()); break;
    case Qt::TopEdge: case Qt::BottomEdge:
        setCursor(cm.sizeVer()); break;
    case Qt::LeftEdge | Qt::TopEdge: case Qt::RightEdge | Qt::BottomEdge:
        setCursor(cm.sizeFDiag()); break;
    case Qt::LeftEdge | Qt::BottomEdge: case Qt::RightEdge | Qt::TopEdge:
        setCursor(cm.sizeBDiag()); break;
    default: setCursor(cm.arrow()); break;
    }
}

void UnifiedFlowWindow::centerToHalfScreen() {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        resize(1000, 700);
        return;
    }

    const QRect area = screen->availableGeometry();
    const int w = int(area.width() * 0.45);
    const int h = int(area.height() * 0.45);
    resize(w, h);
    move(area.x() + (area.width() - w) / 2, area.y() + (area.height() - h) / 2);
}

void UnifiedFlowWindow::applyTheme() {
    const auto &palette = ThemeManager::paletteAt(themeIndex_);
    const QString styleSheet = ThemeManager::render(palette);
    if (styleSheet.isEmpty()) {
        qWarning() << "Failed to render theme:" << palette.id;
        qApp->setStyleSheet(QString());
    } else {
        qApp->setStyleSheet(styleSheet);
    }

    if (themeButton_) {
        themeButton_->setToolTip("主题: " + palette.name + " (点击选择)");
    }
}
