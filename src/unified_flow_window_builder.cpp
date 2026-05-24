#include "unified_flow_window.h"

#include <QApplication>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "chat_client.h"
#include "chat_server.h"
#include "constants.h"
#include "cursor_manager.h"
#include "mainwindow.h"

void UnifiedFlowWindow::buildUi(const QString &defaultHost, quint16 defaultPort,
                                bool defaultServerMode) {
    QWidget *root = new QWidget;
    root->setObjectName("rootWidget");
    setCentralWidget(root);
    qApp->installEventFilter(this);
    QVBoxLayout *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    titleBar_ = new QWidget;
    titleBar_->setObjectName("titleBar");
    titleBar_->setFixedHeight(36);
    QHBoxLayout *tbLayout = new QHBoxLayout(titleBar_);
    tbLayout->setContentsMargins(12, 0, 4, 0);
    tbLayout->setSpacing(8);

    QLabel *tbIcon = new QLabel("◈");
    tbIcon->setObjectName("titleBarIcon");
    tbIcon->setFixedSize(26, 26);
    tbIcon->setAlignment(Qt::AlignCenter);
    tbLayout->addWidget(tbIcon);

    QLabel *tbTitle = new QLabel(Constants::APP_NAME);
    tbTitle->setObjectName("titleBarLabel");
    tbLayout->addWidget(tbTitle);
    tbLayout->addStretch();

    auto winBtn = [&](const QString &text, const QString &tip, auto &&fn,
                      const QString &objName) {
        QPushButton *btn = new QPushButton(text);
        btn->setObjectName(objName);
        btn->setToolTip(tip);
        btn->setFixedSize(34, 26);
        btn->setCursor(CursorManager::instance().hand());
        connect(btn, &QPushButton::clicked, this, fn);
        tbLayout->addWidget(btn);
        return btn;
    };

    winBtn("–", "最小化", &UnifiedFlowWindow::onWinMinimize, "winMinBtn");
    maximizeBtn_ = winBtn("□", "最大化", &UnifiedFlowWindow::onWinMaxRestore, "winMaxBtn");
    winBtn("✕", "关闭", &UnifiedFlowWindow::onWinClose, "winCloseBtn");

    titleBar_->installEventFilter(this);

    QWidget *bodyWrap = new QWidget;
    bodyWrap->setObjectName("bodyWrap");
    QHBoxLayout *bodyLayout = new QHBoxLayout(bodyWrap);
    bodyLayout->setContentsMargins(6, 4, 6, 4);
    bodyLayout->setSpacing(0);

    QWidget *sideBar = new QWidget;
    sideBar->setObjectName("sideBar");
    sideBar->setFixedWidth(64);
    QVBoxLayout *sideLayout = new QVBoxLayout(sideBar);
    sideLayout->setContentsMargins(4, 8, 4, 8);
    sideLayout->setSpacing(2);

    auto sideBtn = [&](const QString &icon, const QString &tip, auto &&fn) {
        QPushButton *btn = new QPushButton(icon);
        btn->setObjectName("sideBarBtn");
        btn->setToolTip(tip);
        btn->setCursor(CursorManager::instance().hand());
        btn->setFixedSize(56, 40);
        connect(btn, &QPushButton::clicked, this, fn);
        sideLayout->addWidget(btn, 0, Qt::AlignHCenter);
        return btn;
    };

    QLabel *sideLogo = new QLabel("CH");
    sideLogo->setObjectName("sideLogo");
    sideLogo->setAlignment(Qt::AlignCenter);
    sideLogo->setFixedSize(42, 42);
    sideLayout->addWidget(sideLogo, 0, Qt::AlignHCenter);
    sideLayout->addSpacing(8);

    QPushButton *themeSideBtn = sideBtn("◑", "切换主题",
                                         &UnifiedFlowWindow::onToggleTheme);
    themeButton_ = themeSideBtn;

    QMenu *themeMenu = new QMenu(themeButton_);
    themeMenu->addAction("素白", this, [this]() { setTheme(0); });
    themeMenu->addAction("森氧", this, [this]() { setTheme(1); });
    themeMenu->addAction("云海", this, [this]() { setTheme(2); });
    themeMenu->addAction("夜航", this, [this]() { setTheme(3); });
    themeButton_->setMenu(themeMenu);

    sideLayout->addSpacing(12);

    disconnectSideBtn_ = new QPushButton("✕");
    disconnectSideBtn_->setObjectName("sideBarBtn");
    disconnectSideBtn_->setToolTip("断开连接 (Ctrl+D)");
    disconnectSideBtn_->setCursor(CursorManager::instance().hand());
    disconnectSideBtn_->setFixedSize(56, 40);
    connect(disconnectSideBtn_, &QPushButton::clicked, this, [this]() {
        if (client_->isConnected()) client_->permanentDisconnect();
        stopServerIfRunning();
        startupStatusLabel_->setText("服务端已停止");
        serverLogList_->clear();
        setCurrentPage(startupPage_, "启动页");
    });
    sideLayout->addWidget(disconnectSideBtn_, 0, Qt::AlignHCenter);

    sideLayout->addStretch();

    sideBtn("?", "关于", [this]() {
        QMessageBox box(this);
        box.setWindowTitle("关于 ChatRoom");
        box.setText("<h1 style='margin:0;color:#2d4a34;'>✦ ChatRoom v1.0</h1>");
        box.setInformativeText(
            "<p style='font-size:13px;line-height:1.8;'>"
            "现代化多线程图形界面聊天室<br>"
            "基于 <b>Qt + CMake</b> 构建 · 多主题切换</p>"
            "<hr style='border:0.5px solid #d0e0d0;'>"
            "<p style='font-size:12px;color:#666;'>"
            "<b>功能</b>　服务端/客户端双模式 · 禁言管理 · 服务器公告<br>"
            "<b>快捷键</b>　F11 全屏　·　Ctrl+D 断开</p>");
        box.setIconPixmap(QPixmap());
        box.setStandardButtons(QMessageBox::Ok);
        box.setDefaultButton(QMessageBox::Ok);

        QAbstractButton *okBtn = box.button(QMessageBox::Ok);
        if (okBtn) {
            okBtn->setText("知道了");
            okBtn->setMinimumWidth(100);
        }

        box.setStyleSheet(
            "QMessageBox { background: #f6faf4; border-radius: 16px; }"
            "QLabel { color: #2d4434; }"
            "QPushButton { border: 1px solid #bccfbc; border-radius: 10px;"
            "  padding: 7px 24px; background: #f0f5ee; color: #2d4a34; }"
            "QPushButton:hover { background: #e0ece0; }");

        QRect myGeo = geometry();
        QSize boxSize = box.sizeHint();
        int cx = myGeo.x() + (myGeo.width() - boxSize.width()) / 2;
        int cy = myGeo.y() + (myGeo.height() - boxSize.height()) / 2;
        box.move(cx, cy);
        box.exec();
    });

    QWidget *mainArea = new QWidget;
    mainArea->setObjectName("mainArea");
    QVBoxLayout *mainLayout = new QVBoxLayout(mainArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    QWidget *topBar = new QWidget;
    topBar->setObjectName("topBar");
    QHBoxLayout *topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(20, 10, 16, 10);
    topBarLayout->setSpacing(12);

    topBarTitleLabel_ = new QLabel(Constants::APP_NAME);
    topBarTitleLabel_->setObjectName("topBarTitle");
    topBarLayout->addWidget(topBarTitleLabel_);

    topBarSubtitleLabel_ = new QLabel("统一控制台");
    topBarSubtitleLabel_->setObjectName("topBarSubtitle");
    topBarLayout->addWidget(topBarSubtitleLabel_);
    topBarLayout->addStretch();

    pages_ = new QStackedWidget;
    pages_->setObjectName("pages");

    QScrollArea *pageScroll = new QScrollArea;
    pageScroll->setObjectName("pageScroll");
    pageScroll->setWidgetResizable(true);
    pageScroll->setWidget(pages_);
    pageScroll->setFrameShape(QFrame::NoFrame);

    startupPage_ = new QWidget;
    {
        QVBoxLayout *layout = new QVBoxLayout(startupPage_);
        layout->setContentsMargins(0, 0, 0, 0);

        QFrame *card = qobject_cast<QFrame *>(cardWidget("选择运行模式"));
        QVBoxLayout *cardLayout = qobject_cast<QVBoxLayout *>(card->layout());

        serverModeRadio_ = new QRadioButton("作为服务端");
        clientModeRadio_ = new QRadioButton("作为客户端");
        serverModeRadio_->setChecked(defaultServerMode);
        clientModeRadio_->setChecked(!defaultServerMode);
        cardLayout->addWidget(serverModeRadio_);
        cardLayout->addWidget(clientModeRadio_);

        serverFields_ = new QWidget;
        QHBoxLayout *serverLayout = new QHBoxLayout(serverFields_);
        serverLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *serverPortTitle = new QLabel("服务端口");
        serverPortEdit_ = new QLineEdit(QString::number(defaultPort));
        serverLayout->addWidget(serverPortTitle);
        serverLayout->addWidget(serverPortEdit_);

        clientFields_ = new QWidget;
        QGridLayout *clientLayout = new QGridLayout(clientFields_);
        clientLayout->setContentsMargins(0, 0, 0, 0);
        clientLayout->addWidget(new QLabel("服务端地址"), 0, 0);
        clientLayout->addWidget(new QLabel("服务端口"), 0, 1);
        clientHostEdit_ = new QLineEdit(defaultHost);
        clientPortEdit_ = new QLineEdit(QString::number(defaultPort));
        clientLayout->addWidget(clientHostEdit_, 1, 0);
        clientLayout->addWidget(clientPortEdit_, 1, 1);

        startupStatusLabel_ = new QLabel;
        startupStatusLabel_->setObjectName("statusLabel");
        QPushButton *nextButton = new QPushButton("下一步");
        nextButton->setObjectName("primaryButton");

        cardLayout->addWidget(serverFields_);
        cardLayout->addWidget(clientFields_);
        cardLayout->addWidget(startupStatusLabel_);
        cardLayout->addWidget(nextButton);
        layout->addWidget(card, 0, Qt::AlignCenter);

        connect(serverModeRadio_, &QRadioButton::toggled, this,
                &UnifiedFlowWindow::onModeChanged);
        connect(nextButton, &QPushButton::clicked, this,
                &UnifiedFlowWindow::onNextClicked);
        onModeChanged();
    }

    serverPage_ = new QWidget;
    {
        QVBoxLayout *layout = new QVBoxLayout(serverPage_);
        layout->setContentsMargins(0, 0, 0, 0);
        QFrame *card = qobject_cast<QFrame *>(cardWidget("服务端监控"));
        QVBoxLayout *cardLayout = qobject_cast<QVBoxLayout *>(card->layout());

        serverIpLabel_ = new QLabel("地址: -");
        serverPortLabel_ = new QLabel("端口: -");
        serverInfoLabel_ = new QLabel("当前在线人数: 0");

        QHBoxLayout *statsRow = new QHBoxLayout;
        statsRow->setSpacing(16);
        serverConnectionsLabel_ = new QLabel("连接数: 0");
        serverMessagesLabel_ = new QLabel("消息数: 0");
        serverUptimeLabel_ = new QLabel("运行时间: -");
        statsRow->addWidget(serverConnectionsLabel_);
        statsRow->addWidget(serverMessagesLabel_);
        statsRow->addWidget(serverUptimeLabel_);
        statsRow->addStretch();

        QHBoxLayout *onlineRow = new QHBoxLayout;
        onlineRow->setSpacing(6);
        onlineUsersList_ = new QListWidget;
        onlineUsersList_->setMaximumHeight(180);
        QVBoxLayout *onlineActions = new QVBoxLayout;
        onlineActions->setSpacing(6);
        muteButton_ = new QPushButton("禁言选中");
        muteButton_->setObjectName("primaryButton");
        muteButton_->setCursor(CursorManager::instance().hand());
        unmuteButton_ = new QPushButton("解除禁言");
        unmuteButton_->setCursor(CursorManager::instance().hand());
        onlineActions->addWidget(muteButton_);
        onlineActions->addWidget(unmuteButton_);
        onlineActions->addStretch();
        onlineRow->addWidget(onlineUsersList_, 1);
        onlineRow->addLayout(onlineActions);

        QLabel *broadcastTitle = new QLabel("服务器公告");
        broadcastTitle->setObjectName("cardTitle");
        QHBoxLayout *broadcastRow = new QHBoxLayout;
        broadcastRow->setSpacing(6);
        serverBroadcastEdit_ = new QLineEdit;
        serverBroadcastEdit_->setPlaceholderText("输入公告内容...");
        QPushButton *broadcastButton = new QPushButton("发送公告");
        broadcastButton->setObjectName("primaryButton");
        broadcastButton->setCursor(CursorManager::instance().hand());
        broadcastRow->addWidget(serverBroadcastEdit_, 1);
        broadcastRow->addWidget(broadcastButton);

        QLabel *logTitle = new QLabel("运行日志");
        logTitle->setObjectName("cardTitle");
        serverLogList_ = new QListWidget;
        serverLogList_->setMaximumHeight(160);
        serverLogList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QHBoxLayout *bottomActions = new QHBoxLayout;
        QPushButton *backButton = new QPushButton("返回启动页");
        QPushButton *clearLogButton = new QPushButton("清空日志");
        bottomActions->addWidget(backButton);
        bottomActions->addWidget(clearLogButton);
        bottomActions->addStretch();

        cardLayout->addWidget(serverIpLabel_);
        cardLayout->addWidget(serverPortLabel_);
        cardLayout->addWidget(serverInfoLabel_);
        cardLayout->addLayout(statsRow);
        cardLayout->addLayout(onlineRow);
        cardLayout->addWidget(broadcastTitle);
        cardLayout->addLayout(broadcastRow);
        cardLayout->addWidget(logTitle);
        cardLayout->addWidget(serverLogList_);
        cardLayout->addLayout(bottomActions);
        layout->addWidget(card);

        connect(backButton, &QPushButton::clicked, this, [this]() {
            stopServerIfRunning();
            startupStatusLabel_->setText("服务端已停止");
            serverLogList_->clear();
            setCurrentPage(startupPage_, "启动页");
        });

        connect(clearLogButton, &QPushButton::clicked, this, [this]() {
            serverLogList_->clear();
        });

        connect(muteButton_, &QPushButton::clicked, this, [this]() {
            QListWidgetItem *item = onlineUsersList_->currentItem();
            if (!item || server_ == nullptr) return;
            server_->muteUser(item->data(Qt::UserRole).toString());
        });

        connect(unmuteButton_, &QPushButton::clicked, this, [this]() {
            QListWidgetItem *item = onlineUsersList_->currentItem();
            if (!item || server_ == nullptr) return;
            server_->unmuteUser(item->data(Qt::UserRole).toString());
        });

        connect(broadcastButton, &QPushButton::clicked, this, [this]() {
            if (server_ == nullptr) return;
            QString text = serverBroadcastEdit_->text().trimmed();
            if (text.isEmpty()) return;
            server_->serverBroadcast(text);
            serverBroadcastEdit_->clear();
        });

        connect(serverBroadcastEdit_, &QLineEdit::returnPressed,
                broadcastButton, &QPushButton::click);
    }

    authPage_ = new QWidget;
    {
        QVBoxLayout *layout = new QVBoxLayout(authPage_);
        layout->setContentsMargins(0, 0, 0, 0);
        QFrame *card = qobject_cast<QFrame *>(cardWidget("客户端认证"));
        QVBoxLayout *cardLayout = qobject_cast<QVBoxLayout *>(card->layout());

        endpointLabel_ = new QLabel("目标服务器: -");
        loginModeRadio_ = new QRadioButton("登录");
        registerModeRadio_ = new QRadioButton("注册");
        loginModeRadio_->setChecked(true);

        loginFields_ = new QWidget;
        QGridLayout *loginLayout = new QGridLayout(loginFields_);
        loginLayout->addWidget(new QLabel("用户名"), 0, 0);
        loginLayout->addWidget(new QLabel("密码"), 0, 1);
        loginUserEdit_ = new QLineEdit;
        loginPassEdit_ = new QLineEdit;
        loginPassEdit_->setEchoMode(QLineEdit::Password);
        loginLayout->addWidget(loginUserEdit_, 1, 0);
        loginLayout->addWidget(loginPassEdit_, 1, 1);

        registerFields_ = new QWidget;
        QGridLayout *regLayout = new QGridLayout(registerFields_);
        regLayout->addWidget(new QLabel("用户名"), 0, 0);
        regLayout->addWidget(new QLabel("密码"), 0, 1);
        regLayout->addWidget(new QLabel("确认密码"), 0, 2);
        regUserEdit_ = new QLineEdit;
        regPassEdit_ = new QLineEdit;
        regConfirmEdit_ = new QLineEdit;
        regPassEdit_->setEchoMode(QLineEdit::Password);
        regConfirmEdit_->setEchoMode(QLineEdit::Password);
        regLayout->addWidget(regUserEdit_, 1, 0);
        regLayout->addWidget(regPassEdit_, 1, 1);
        regLayout->addWidget(regConfirmEdit_, 1, 2);

        authStatusLabel_ = new QLabel;
        authStatusLabel_->setObjectName("statusLabel");

        QHBoxLayout *actions = new QHBoxLayout;
        QPushButton *backBtn = new QPushButton("返回");
        authActionButton_ = new QPushButton("登录");
        authActionButton_->setObjectName("primaryButton");
        actions->addWidget(backBtn);
        actions->addWidget(authActionButton_);

        cardLayout->addWidget(endpointLabel_);
        cardLayout->addWidget(loginModeRadio_);
        cardLayout->addWidget(registerModeRadio_);
        cardLayout->addWidget(loginFields_);
        cardLayout->addWidget(registerFields_);
        cardLayout->addWidget(authStatusLabel_);
        cardLayout->addLayout(actions);
        layout->addWidget(card, 0, Qt::AlignCenter);

        connect(loginModeRadio_, &QRadioButton::toggled, this,
                &UnifiedFlowWindow::onAuthModeChanged);
        connect(authActionButton_, &QPushButton::clicked, this,
                &UnifiedFlowWindow::onAuthAction);
        connect(backBtn, &QPushButton::clicked, this,
                &UnifiedFlowWindow::onBackToStart);
        onAuthModeChanged();
    }

    pages_->addWidget(startupPage_);
    pages_->addWidget(serverPage_);
    pages_->addWidget(authPage_);
    setCurrentPage(startupPage_, "启动页");

    mainLayout->addWidget(topBar);
    mainLayout->addWidget(pageScroll, 1);

    bodyLayout->addWidget(sideBar);
    bodyLayout->addWidget(mainArea, 1);

    rootLayout->addWidget(titleBar_);
    rootLayout->addWidget(bodyWrap);
}
