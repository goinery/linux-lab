#include "mainwindow.h"
#include "cursor_manager.h"
#include "message_widget.h"
#include "protocol.h"
#include "constants.h"

#include <QApplication>
#include <QSplitter>
#include <QScrollBar>
#include <QDateTime>
#include <QShortcut>
#include <QKeySequence>
#include <QStatusBar>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QTimer>
#include <QSizePolicy>

MainWindow::MainWindow(ChatClient *client, QWidget *parent)
    : QMainWindow(parent), client_(client), currentChat_("general") {
    setWindowTitle(Constants::APP_NAME + " " + Constants::APP_VERSION);
    setMinimumSize(640, 480);
    resize(1000, 700);
    setupMenuBar();
    setupUI();
    setupShortcuts();
    setupStatusBar();

    connect(client_, &ChatClient::messageReceived, this, &MainWindow::onMessageReceived);
    connect(client_, &ChatClient::connected, this, [this]() {
        statusLabel_->setText("已连接到服务器");
        client_->sendMessage(Protocol::createUserListRequest());
    });
    connect(client_, &ChatClient::disconnected, this, [this]() {
        statusLabel_->setText("已断开连接");
    });
    connect(client_, &ChatClient::errorOccurred, this, [this](const QString &err) {
        statusLabel_->setText("连接错误: " + err);
    });

    QWidget *generalPage = createChatPage("general");
    chatStack_->addWidget(generalPage);
    chatPages_["general"] = generalPage;

    chatTitleLabel_->setText("群聊");
    currentChat_ = "general";

    statusLabel_->setText(client_->isConnected() ? "已连接到服务器" : "正在连接服务器...");
    if (client_->isConnected()) {
        client_->sendMessage(Protocol::createUserListRequest());
    }

}

void MainWindow::setupUI() {
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);

    mainLayout_ = new QHBoxLayout(centralWidget_);
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    mainLayout_->setSpacing(0);

    splitter_ = new QSplitter(Qt::Horizontal, this);

    QWidget *leftPanel = new QWidget;
    leftPanel->setObjectName("leftPanel");
    leftPanel->setMinimumWidth(160);
    leftPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    QLabel *userListTitle = new QLabel("在 线 用 户");
    userListTitle->setObjectName("userListTitle");
    userListTitle->setAlignment(Qt::AlignCenter);
    userListTitle->setFixedHeight(48);

    userListWidget_ = new QListWidget;
    userListWidget_->setObjectName("userList");
    userListWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ensureUserItem("general");
    userListWidget_->item(0)->setSelected(true);
    refreshAllUserItems();

    leftLayout->addWidget(userListTitle);
    leftLayout->addWidget(userListWidget_);

    connect(userListWidget_, &QListWidget::itemClicked,
            this, &MainWindow::onUserItemClicked);
    connect(userListWidget_, &QListWidget::itemSelectionChanged,
            this, &MainWindow::refreshAllUserItems);

    QWidget *rightPanel = new QWidget;
    rightPanel->setObjectName("rightPanel");
    rightPanel->setMinimumWidth(0);
    rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    chatTitleLabel_ = new QLabel;
    chatTitleLabel_->setObjectName("chatTitleLabel");
    chatTitleLabel_->setAlignment(Qt::AlignCenter);
    chatTitleLabel_->setFixedHeight(48);

    chatStack_ = new QStackedWidget;
    chatStack_->setObjectName("chatStack");

    QWidget *inputArea = new QWidget;
    inputArea->setObjectName("inputArea");
    QHBoxLayout *inputLayout = new QHBoxLayout(inputArea);
    inputLayout->setContentsMargins(12, 10, 12, 10);

    inputEdit_ = new QTextEdit;
    inputEdit_->setObjectName("inputEdit");
    inputEdit_->setMinimumHeight(50);
    inputEdit_->setMaximumHeight(120);
    inputEdit_->setPlaceholderText("输入消息... (Ctrl+Enter 发送)");
    inputEdit_->setCursor(CursorManager::instance().ibeam());
    inputEdit_->viewport()->setCursor(CursorManager::instance().ibeam());
    inputEdit_->setAcceptRichText(false);
    inputEdit_->setAttribute(Qt::WA_InputMethodEnabled, true);
    inputEdit_->setInputMethodHints(Qt::ImhNone);
    inputEdit_->setTabChangesFocus(true);

    sendButton_ = new QPushButton("发 送");
    sendButton_->setObjectName("sendButton");
    sendButton_->setMinimumSize(80, 50);
    sendButton_->setCursor(CursorManager::instance().hand());

    inputLayout->addWidget(inputEdit_, 1);
    inputLayout->addWidget(sendButton_);

    rightLayout->addWidget(chatTitleLabel_);
    rightLayout->addWidget(chatStack_, 1);
    rightLayout->addWidget(inputArea);

    splitter_->addWidget(leftPanel);
    splitter_->addWidget(rightPanel);
    splitter_->setStretchFactor(0, 0);
    splitter_->setStretchFactor(1, 1);
    splitter_->setCollapsible(0, false);
    splitter_->setCollapsible(1, true);

    mainLayout_->addWidget(splitter_);

    connect(sendButton_, &QPushButton::clicked, this, &MainWindow::onSendClicked);
}

void MainWindow::setupShortcuts() {
    QShortcut *fullscreenShortcut = new QShortcut(QKeySequence("F11"), this);
    fullscreenShortcut->setContext(Qt::ApplicationShortcut);
    connect(fullscreenShortcut, &QShortcut::activated, this, &MainWindow::toggleFullscreen);

    QShortcut *sendShortcut = new QShortcut(QKeySequence("Ctrl+Return"), this);
    sendShortcut->setContext(Qt::ApplicationShortcut);
    connect(sendShortcut, &QShortcut::activated, this, &MainWindow::onSendClicked);
}

void MainWindow::setupStatusBar() {
    statusLabel_ = new QLabel("正在连接服务器...");
    statusBar()->addWidget(statusLabel_, 1);
}

void MainWindow::toggleFullscreen() {
    QWidget *host = window();
    if (host && host != this) {
        if (host->isFullScreen()) {
            host->showNormal();
        } else {
            host->showFullScreen();
        }
        return;
    }

    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::updateBubbleMaxWidth() {
    for (auto it = chatPages_.begin(); it != chatPages_.end(); ++it) {
        QWidget *page = it.value();
        int maxWidth = bubbleMaxWidthForChat(it.key());
        QList<MessageWidget *> msgs = page->findChildren<MessageWidget *>();
        for (MessageWidget *msg : msgs) {
            msg->updateWidth(maxWidth);
        }
    }
}

int MainWindow::bubbleMaxWidthForChat(const QString &chatName) const {
    const QScrollArea *scrollArea = chatScrollAreas_.value(chatName, nullptr);
    const int viewportWidth = scrollArea && scrollArea->viewport()
        ? scrollArea->viewport()->width()
        : qMax(320, width());
    const int availableWidth = qMax(120, viewportWidth - 92);
    const int preferredWidth = qMax(120, int(viewportWidth * 0.68));
    return qBound(120, preferredWidth, availableWidth);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    QMainWindow::keyPressEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateBubbleMaxWidth();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    return QMainWindow::eventFilter(obj, event);
}

QWidget *MainWindow::createChatPage(const QString &name) {
    QWidget *page = new QWidget;
    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(8, 8, 8, 8);
    pageLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setObjectName("chatScrollArea");
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *messagesContainer = new QWidget;
    messagesContainer->setObjectName("messagesContainer");
    QVBoxLayout *messagesLayout = new QVBoxLayout(messagesContainer);
    messagesLayout->setAlignment(Qt::AlignTop);
    messagesLayout->setSpacing(4);

    scrollArea->setWidget(messagesContainer);
    pageLayout->addWidget(scrollArea);

    chatLayouts_[name] = messagesLayout;
    chatScrollAreas_[name] = scrollArea;

    return page;
}

void MainWindow::flushPendingMessagesForChat(const QString &chatName) {
    if (!pendingMessages_.contains(chatName) || pendingMessages_[chatName].isEmpty()) {
        return;
    }

    for (const QJsonObject &msg : pendingMessages_[chatName]) {
        QString from = msg["from"].toString();
        QString content = msg["content"].toString();
        QString time;
        if (msg.contains("timestamp")) {
            time = QDateTime::fromSecsSinceEpoch(
                msg["timestamp"].toVariant().toLongLong()).toString("hh:mm");
        } else {
            time = QDateTime::currentDateTime().toString("hh:mm");
        }
        addChatMessage(from, content, time, MessageWidget::Left, chatName);
    }

    pendingMessages_.remove(chatName);
}

void MainWindow::onSendClicked() {
    QString text = inputEdit_->toPlainText().trimmed();
    if (text.isEmpty()) return;

    if (currentChat_ == "general") {
        client_->sendMessage(Protocol::createGroupChatMessage(client_->username(), text));
    } else {
        client_->sendMessage(Protocol::createChatMessage(client_->username(), currentChat_, text));
    }

    inputEdit_->clear();
    inputEdit_->setFocus();
}

void MainWindow::onUserItemClicked(QListWidgetItem *item) {
    QString name = item->data(Qt::UserRole).toString();
    if (name.isEmpty()) return;

    if (!chatPages_.contains(name)) {
        QWidget *page = createChatPage(name);
        chatStack_->addWidget(page);
        chatPages_[name] = page;
    }

    flushPendingMessagesForChat(name);

    chatStack_->setCurrentWidget(chatPages_[name]);
    currentChat_ = name;
    chatTitleLabel_->setText(name == "general" ? "群聊" : "与 " + name + " 的对话");
    clearUnread(name);
}

void MainWindow::onMessageReceived(QJsonObject message) {
    QString type = message["type"].toString();

    if (type == "chat") {
        QString from = message["from"].toString();
        QString to = message["to"].toString();
        QString content = message["content"].toString();
        QString time;
        if (message.contains("timestamp")) {
            time = QDateTime::fromSecsSinceEpoch(
                message["timestamp"].toVariant().toLongLong()).toString("hh:mm");
        } else {
            time = QDateTime::currentDateTime().toString("hh:mm");
        }

        if (from == client_->username()) {
            ensureChatPage(to);
            ensureUserItem(to);
            addChatMessage(from, content, time, MessageWidget::Right, to);
        } else if (currentChat_ == from && chatPages_.contains(from)) {
            addChatMessage(from, content, time, MessageWidget::Left, from);
        } else {
            pendingMessages_[from].append(message);
            if (!chatPages_.contains(from)) {
                QWidget *page = createChatPage(from);
                chatStack_->addWidget(page);
                chatPages_[from] = page;
            }
            ensureUserItem(from);
            incrementUnread(from);
        }
    } else if (type == "group_chat") {
        QString from = message["from"].toString();
        QString content = message["content"].toString();
        QString time;
        if (message.contains("timestamp")) {
            time = QDateTime::fromSecsSinceEpoch(
                message["timestamp"].toVariant().toLongLong()).toString("hh:mm");
        } else {
            time = QDateTime::currentDateTime().toString("hh:mm");
        }

        if (from == client_->username()) {
            addChatMessage(from, content, time, MessageWidget::Right, "general");
        } else {
            addChatMessage(from, content, time, MessageWidget::Left, "general");
            if (currentChat_ != "general") {
                incrementUnread("general");
            }
        }
    } else if (type == "user_list") {
        onUserListReceived(message["users"].toArray());
    } else if (type == "system") {
        QString content = message["content"].toString();
        QString targetChat = message["target_chat"].toString();
        if (!targetChat.isEmpty()) {
            ensureChatPage(targetChat);
            addSystemMessageToChat(content, targetChat);
        } else {
            onSystemMessage(content);
        }
    }
}

void MainWindow::addChatMessage(const QString &username, const QString &content,
                                const QString &time, MessageWidget::MessageSide side,
                                const QString &targetChat) {
    const QString chatName = targetChat.isEmpty() ? currentChat_ : targetChat;
    if (!chatLayouts_.contains(chatName)) return;

    QVBoxLayout *layout = chatLayouts_[chatName];

    int maxWidth = bubbleMaxWidthForChat(chatName);
    MessageWidget *msgWidget = new MessageWidget(username, content, time, side, maxWidth);
    layout->addWidget(msgWidget);

    QScrollArea *scrollArea = chatScrollAreas_[chatName];
    if (scrollArea) {
        QPointer<QScrollBar> bar(scrollArea->verticalScrollBar());
        QPointer<MessageWidget> msgPtr(msgWidget);
        QTimer::singleShot(0, this, [this, chatName, msgPtr]() {
            if (msgPtr) {
                msgPtr->updateWidth(bubbleMaxWidthForChat(chatName));
            }
        });
        QTimer::singleShot(50, this, [this, bar, chatName, msgPtr]() {
            if (msgPtr) {
                msgPtr->updateWidth(bubbleMaxWidthForChat(chatName));
            }
            if (bar) {
                bar->setValue(bar->maximum());
            }
        });
    }
}

void MainWindow::onSystemMessage(const QString &content) {
    if (chatPages_.contains("general")) {
        addSystemMessage(content);
    }
}

void MainWindow::addSystemMessage(const QString &content) {
    if (!chatLayouts_.contains("general")) return;

    QVBoxLayout *layout = chatLayouts_["general"];

    QLabel *sysLabel = new QLabel(content);
    sysLabel->setObjectName("systemMessage");
    sysLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(sysLabel);

    QScrollArea *scrollArea = chatScrollAreas_["general"];
    if (scrollArea) {
        QScrollBar *bar = scrollArea->verticalScrollBar();
        QTimer::singleShot(50, this, [bar]() { bar->setValue(bar->maximum()); });
    }
}

void MainWindow::ensureChatPage(const QString &chatName) {
    if (!chatPages_.contains(chatName)) {
        QWidget *page = createChatPage(chatName);
        chatStack_->addWidget(page);
        chatPages_[chatName] = page;
    }
}

void MainWindow::addSystemMessageToChat(const QString &content, const QString &chatName) {
    if (!chatLayouts_.contains(chatName)) return;

    QVBoxLayout *layout = chatLayouts_[chatName];

    QLabel *sysLabel = new QLabel(content);
    sysLabel->setObjectName("systemMessage");
    sysLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(sysLabel);

    QScrollArea *scrollArea = chatScrollAreas_[chatName];
    if (scrollArea) {
        QScrollBar *bar = scrollArea->verticalScrollBar();
        QTimer::singleShot(50, this, [bar]() { bar->setValue(bar->maximum()); });
    }
}
