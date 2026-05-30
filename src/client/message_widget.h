#ifndef MESSAGE_WIDGET_H
#define MESSAGE_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class QFrame;
class QTextEdit;

class MessageWidget : public QWidget {
    Q_OBJECT

public:
    enum MessageSide {
        Left,
        Right
    };

    explicit MessageWidget(const QString &username, const QString &content,
                           const QString &time, MessageSide side,
                           int maxBubbleWidth = 400, QWidget *parent = nullptr);

    void updateWidth(int newMaxWidth);

private:
    void fitBubbleSize();

    int maxBubbleWidth_;
    QFrame *bubbleFrame_;
    QTextEdit *bubbleEdit_;
    QVBoxLayout *bubbleLayout_;
    QString contentText_;
};

#endif
