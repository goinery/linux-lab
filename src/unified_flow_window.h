#ifndef UNIFIED_FLOW_WINDOW_H
#define UNIFIED_FLOW_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>

class ChatClient;
class ChatServer;
class MainWindow;

class UnifiedFlowWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit UnifiedFlowWindow(const QString &defaultHost, quint16 defaultPort,
                               bool defaultServerMode, QWidget *parent = nullptr);

    void centerToHalfScreen();
    void applyTheme();

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onModeChanged();
    void onNextClicked();
    void onBackToStart();
    void onAuthModeChanged();
    void onAuthAction();
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const QString &err);
    void onClientMessageReceived(const QJsonObject &msg);
    void onToggleTheme();
    void setTheme(int index);
    void onWinMinimize();
    void onWinMaxRestore();
    void onWinClose();
    void onServerUsersUpdated(const QStringList &users);
    void onServerMutedUpdated(const QStringList &mutedUsers);
    void onServerStatsUpdated(int totalConnections, int totalMessages,
                              const QString &uptime);
    void onServerLogUpdated(const QString &entry);

private:
    enum PendingAuthType {
        None,
        Login,
        Register,
    };

    QWidget *cardWidget(const QString &title);
    void stopServerIfRunning();
    void buildUi(const QString &defaultHost, quint16 defaultPort,
                 bool defaultServerMode);
    void setCurrentPage(QWidget *page, const QString &subtitle);
    void setStatus(QLabel *label, const QString &text, const char *kind);
    Qt::Edges calcEdge(const QPoint &pos, const QSize &sz) const;
    void setEdgeCursor(Qt::Edges e);
    QRect resizedGeometry(const QPoint &globalPos) const;

    QStackedWidget *pages_;
    QWidget *startupPage_;
    QWidget *serverPage_;
    QWidget *authPage_;

    QRadioButton *serverModeRadio_;
    QRadioButton *clientModeRadio_;
    QWidget *serverFields_;
    QWidget *clientFields_;
    QLineEdit *serverPortEdit_;
    QLineEdit *clientHostEdit_;
    QLineEdit *clientPortEdit_;
    QLabel *startupStatusLabel_;
    QLabel *topBarTitleLabel_;
    QLabel *topBarSubtitleLabel_;

    QLabel *serverIpLabel_;
    QLabel *serverPortLabel_;
    QLabel *serverInfoLabel_;
    QLabel *serverConnectionsLabel_;
    QLabel *serverMessagesLabel_;
    QLabel *serverUptimeLabel_;
    QListWidget *onlineUsersList_;
    QLineEdit *serverBroadcastEdit_;
    QListWidget *serverLogList_;
    QPushButton *muteButton_;
    QPushButton *unmuteButton_;

    QLabel *endpointLabel_;
    QRadioButton *loginModeRadio_;
    QRadioButton *registerModeRadio_;
    QWidget *loginFields_;
    QWidget *registerFields_;
    QLineEdit *loginUserEdit_;
    QLineEdit *loginPassEdit_;
    QLineEdit *regUserEdit_;
    QLineEdit *regPassEdit_;
    QLineEdit *regConfirmEdit_;
    QLabel *authStatusLabel_;
    QPushButton *authActionButton_;
    QPushButton *themeButton_;
    QPushButton *maximizeBtn_;
    QWidget *titleBar_;
    QPushButton *disconnectSideBtn_;

    ChatClient *client_;
    ChatServer *server_;
    MainWindow *chatPage_;
    int themeIndex_;
    PendingAuthType pendingAuthType_;
    QString pendingHost_;
    quint16 pendingPort_;
    bool dragging_;
    bool resizing_;
    bool maximized_;
    QPoint dragStartPos_;
    QPoint resizeStartPos_;
    Qt::Edges resizeEdges_;
    QRect normalGeometry_;
    QRect resizeStartGeometry_;
};

#endif
