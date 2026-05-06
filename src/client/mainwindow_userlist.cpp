#include "mainwindow.h"
#include "protocol.h"
#include "constants.h"

#include <QAction>
#include <QJsonArray>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

void MainWindow::setupMenuBar() {
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");

    QAction *disconnectAction = fileMenu->addAction("断开连接(&D)");
    disconnectAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(disconnectAction, &QAction::triggered, this, [this]() {
        client_->disconnectFromServer();
        close();
    });

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction("退出(&Q)");
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu("视图(&V)");

    QAction *fullscreenAction = viewMenu->addAction("全屏(&F)");
    fullscreenAction->setShortcut(QKeySequence("F11"));
    fullscreenAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(fullscreenAction, &QAction::triggered, this, &MainWindow::toggleFullscreen);

    viewMenu->addSeparator();

    QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于 ChatRoom",
            "<h2>ChatRoom v1.0</h2>"
            "<p>现代化多线程图形界面聊天室</p>"
            "<p>基于 Qt + CMake 构建</p>"
            "<hr>"
            "<p><b>快捷键：</b></p>"
            "<p>F11 - 全屏切换</p>"
            "<p>Ctrl++ / Ctrl+= - 放大</p>"
            "<p>Ctrl+- - 缩小</p>"
            "<p>Ctrl+0 - 重置缩放</p>"
            "<p>Ctrl+Enter - 发送消息</p>");
    });
}

void MainWindow::onUserListReceived(const QJsonArray &users) {
    QString selectedUser = currentChat_;
    QSet<QString> onlineUsers;
    onlineUsers.insert("general");

    userListWidget_->clear();
    ensureUserItem("general");

    for (const QJsonValue &val : users) {
        QString username = val.toString();
        if (username == client_->username()) continue;
        onlineUsers.insert(username);
        ensureUserItem(username);
    }

    const QList<QString> unreadKeys = unreadCounts_.keys();
    for (const QString &chatName : unreadKeys) {
        if (!onlineUsers.contains(chatName) && chatName != "general") {
            unreadCounts_.remove(chatName);
        }
    }

    refreshAllUserItems();

    bool selectedRestored = false;
    for (int i = 0; i < userListWidget_->count(); ++i) {
        QListWidgetItem *item = userListWidget_->item(i);
        if (item->data(Qt::UserRole).toString() == selectedUser) {
            item->setSelected(true);
            selectedRestored = true;
            break;
        }
    }

    if (!selectedRestored && userListWidget_->count() > 0) {
        userListWidget_->item(0)->setSelected(true);
        currentChat_ = "general";
    }
}

QListWidgetItem *MainWindow::findUserItem(const QString &chatName) const {
    for (int i = 0; i < userListWidget_->count(); ++i) {
        QListWidgetItem *item = userListWidget_->item(i);
        if (item->data(Qt::UserRole).toString() == chatName) {
            return item;
        }
    }
    return nullptr;
}

QListWidgetItem *MainWindow::ensureUserItem(const QString &chatName) {
    QListWidgetItem *item = findUserItem(chatName);
    if (!item) {
        item = new QListWidgetItem;
        item->setData(Qt::UserRole, chatName);
        item->setSizeHint(QSize(0, 56));
        userListWidget_->addItem(item);
    }

    refreshUserItemVisual(chatName);
    return item;
}

void MainWindow::refreshUserItemVisual(const QString &chatName) {
    QListWidgetItem *item = findUserItem(chatName);
    if (!item) {
        return;
    }

    QWidget *row = new QWidget;
    row->setAttribute(Qt::WA_TransparentForMouseEvents);
    row->setMinimumHeight(44);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 4, 12, 4);
    layout->setSpacing(6);

    QLabel *nameLabel = new QLabel(displayNameForChat(chatName));
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setMinimumWidth(0);
    const int unreadCount = unreadCounts_.value(chatName, 0);
    const bool hasUnread = unreadCount > 0 && chatName != currentChat_;
    QFont nameFont = nameLabel->font();
    nameFont.setBold(hasUnread || item->isSelected());
    nameLabel->setFont(nameFont);
    nameLabel->setStyleSheet(item->isSelected() ? "color: #ffffff;"
                                                 : "color: #b0b8c8;");

    QLabel *badgeLabel = new QLabel;
    badgeLabel->setAlignment(Qt::AlignCenter);
    badgeLabel->setFixedHeight(22);
    badgeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    badgeLabel->setStyleSheet(
        "background-color: #ff4d4f;"
        "color: #ffffff;"
        "border-radius: 11px;"
        "font-size: 11px;"
        "font-weight: 700;"
        "padding: 0 4px;");

    if (hasUnread) {
        const QString badgeText = unreadCount > 99 ? "99+" : QString::number(unreadCount);
        badgeLabel->setText(badgeText);
        const QFontMetrics fm(badgeLabel->font());
        const int badgeWidth = qMax(22, fm.horizontalAdvance(badgeText) + 12);
        badgeLabel->setFixedWidth(badgeWidth);
        badgeLabel->show();
    } else {
        badgeLabel->hide();
    }

    layout->addWidget(nameLabel);
    layout->addStretch();
    layout->addWidget(badgeLabel);

    QWidget *oldWidget = userListWidget_->itemWidget(item);
    if (oldWidget) {
        oldWidget->deleteLater();
    }
    userListWidget_->setItemWidget(item, row);
}

void MainWindow::refreshAllUserItems() {
    for (int i = 0; i < userListWidget_->count(); ++i) {
        QListWidgetItem *item = userListWidget_->item(i);
        refreshUserItemVisual(item->data(Qt::UserRole).toString());
    }
}

void MainWindow::incrementUnread(const QString &chatName) {
    unreadCounts_[chatName] = unreadCounts_.value(chatName, 0) + 1;
    refreshUserItemVisual(chatName);
}

void MainWindow::clearUnread(const QString &chatName) {
    unreadCounts_.remove(chatName);
    refreshUserItemVisual(chatName);
}

QString MainWindow::displayNameForChat(const QString &chatName) const {
    return chatName == "general" ? "群聊" : QString("%1").arg(chatName);
}
