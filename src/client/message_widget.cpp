#include "message_widget.h"
#include <QFontMetrics>

MessageWidget::MessageWidget(const QString &username, const QString &content,
                             const QString &time, MessageSide side,
                             int maxBubbleWidth, QWidget *parent)
    : QWidget(parent), maxBubbleWidth_(maxBubbleWidth) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 6, 10, 6);
    mainLayout->setSpacing(2);

    QHBoxLayout *rowLayout = new QHBoxLayout;
    rowLayout->setSpacing(10);

    QChar firstChar = username.isEmpty() ? QChar('?') : username.at(0);
    bool isChinese = firstChar.script() == QChar::Script_Han;

    QLabel *avatarLabel = new QLabel(isChinese ? username.left(1) : username.left(1).toUpper());
    avatarLabel->setObjectName(side == Right ? "avatarSelf" : "avatarOther");
    avatarLabel->setFixedSize(40, 40);
    avatarLabel->setAlignment(Qt::AlignCenter);
    QFont avatarFont;
    avatarFont.setPointSize(isChinese ? 14 : 16);
    avatarFont.setBold(true);
    avatarLabel->setFont(avatarFont);

    bubbleLayout_ = new QVBoxLayout;
    bubbleLayout_->setSpacing(3);

    QLabel *nameLabel = new QLabel(username);
    nameLabel->setObjectName(side == Right ? "nameSelf" : "nameOther");
    QFont nameFont;
    nameFont.setPointSize(10);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    QString wrappedContent;
    const QStringList originalLines = content.split('\n');
    for (int i = 0; i < originalLines.size(); ++i) {
        const QString &line = originalLines[i];
        for (int j = 0; j < line.length(); j += 48) {
            if (j > 0) wrappedContent += '\n';
            wrappedContent += line.mid(j, 48);
        }
        if (i < originalLines.size() - 1) wrappedContent += '\n';
    }

    bubbleLabel_ = new QLabel;
    bubbleLabel_->setObjectName(side == Right ? "bubbleSelf" : "bubbleOther");
    bubbleLabel_->setTextFormat(Qt::PlainText);
    bubbleLabel_->setText(wrappedContent);
    bubbleLabel_->setWordWrap(false);
    bubbleLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubbleLabel_->setMinimumSize(0, 0);

    fitBubbleSize(wrappedContent);

    QLabel *timeLabel = new QLabel(time);
    timeLabel->setObjectName("timeLabel");
    QFont timeFont;
    timeFont.setPointSize(9);
    timeLabel->setFont(timeFont);

    bubbleLayout_->addWidget(nameLabel);
    bubbleLayout_->addWidget(bubbleLabel_);
    bubbleLayout_->addWidget(timeLabel);

    if (side == Right) {
        rowLayout->addStretch(1);
        rowLayout->addLayout(bubbleLayout_, 0);
        rowLayout->addWidget(avatarLabel, 0);
        nameLabel->setAlignment(Qt::AlignRight);
        timeLabel->setAlignment(Qt::AlignRight);
    } else {
        rowLayout->addWidget(avatarLabel, 0);
        rowLayout->addLayout(bubbleLayout_, 0);
        rowLayout->addStretch(1);
        nameLabel->setAlignment(Qt::AlignLeft);
        timeLabel->setAlignment(Qt::AlignLeft);
    }

    mainLayout->addLayout(rowLayout);

    const int minHeight = qMax(56, bubbleH_ + 30);
    setMinimumHeight(minHeight);
}

void MessageWidget::fitBubbleSize(const QString &wrappedText) {
    QFont msgFont;
    msgFont.setPointSize(13);
    bubbleLabel_->setFont(msgFont);

    QFontMetrics fm(msgFont);
    const QStringList lines = wrappedText.split('\n');
    int maxLineW = 0;
    for (const QString &l : lines) {
        maxLineW = qMax(maxLineW, fm.horizontalAdvance(l));
    }

    bubbleW_ = qMin(maxLineW + 30, maxBubbleWidth_);
    if (bubbleW_ < 20) bubbleW_ = 20;

    bubbleH_ = fm.lineSpacing() * lines.size() + 24;
    if (bubbleH_ < 20) bubbleH_ = 32;
    bubbleLabel_->setFixedSize(bubbleW_, bubbleH_);
}

void MessageWidget::updateWidth(int newMaxWidth) {
    maxBubbleWidth_ = newMaxWidth;
}
