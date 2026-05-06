#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QSplitter>
#include <QMap>
#include <QSet>
#include <QJsonArray>
#include <QStatusBar>
#include <QShortcut>
#include "chat_client.h"
#include "message_widget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ChatClient *client, QWidget *parent = nullptr);
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSendClicked();
    void onUserItemClicked(QListWidgetItem *item);
    void onMessageReceived(QJsonObject message);
    void onUserListReceived(const QJsonArray &users);
    void onSystemMessage(const QString &content);
    void toggleFullscreen();

private:
    void setupUI();
    void setupMenuBar();
    void setupShortcuts();
    void setupStatusBar();
    void addChatMessage(const QString &username, const QString &content,
                        const QString &time, MessageWidget::MessageSide side,
                        const QString &targetChat = QString());
    void addSystemMessage(const QString &content);
    void ensureChatPage(const QString &chatName);
    void addSystemMessageToChat(const QString &content, const QString &chatName);
    void updateUnreadBadge(const QString &chatName, int count);
    QWidget *createChatPage(const QString &name);
    void flushPendingMessagesForChat(const QString &chatName);
    void updateBubbleMaxWidth();
    QListWidgetItem *findUserItem(const QString &chatName) const;
    QListWidgetItem *ensureUserItem(const QString &chatName);
    void refreshUserItemVisual(const QString &chatName);
    void refreshAllUserItems();
    void incrementUnread(const QString &chatName);
    void clearUnread(const QString &chatName);
    QString displayNameForChat(const QString &chatName) const;

    ChatClient *client_;
    QString currentChat_;

    QWidget *centralWidget_;
    QHBoxLayout *mainLayout_;

    QSplitter *splitter_;
    QListWidget *userListWidget_;
    QStackedWidget *chatStack_;
    QMap<QString, QWidget *> chatPages_;
    QMap<QString, QVBoxLayout *> chatLayouts_;
    QMap<QString, QScrollArea *> chatScrollAreas_;
    QMap<QString, QList<QJsonObject>> pendingMessages_;
    QMap<QString, int> unreadCounts_;

    QTextEdit *inputEdit_;
    QPushButton *sendButton_;
    QLabel *chatTitleLabel_;
    QLabel *statusLabel_;
};

#endif
