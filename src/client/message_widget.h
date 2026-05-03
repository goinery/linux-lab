#ifndef MESSAGE_WIDGET_H
#define MESSAGE_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

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
    void fitBubbleSize(const QString &wrappedText);

    int maxBubbleWidth_;
    QLabel *bubbleLabel_;
    QVBoxLayout *bubbleLayout_;
    int bubbleW_;
    int bubbleH_;
};

#endif
