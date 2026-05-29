#include "message_widget.h"
#include <QFontMetrics>
#include <cmath>

namespace {

constexpr int kBubbleHorizontalPadding = 14;
constexpr int kBubbleVerticalPadding = 8;
constexpr int kMinBubbleContentWidth = 32;

}

MessageWidget::MessageWidget(const QString &username, const QString &content,
                             const QString &time, MessageSide side,
                             int maxBubbleWidth, QWidget *parent)
    : QWidget(parent), maxBubbleWidth_(maxBubbleWidth), contentText_(content) {
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

    bubbleLabel_ = new QLabel;
    bubbleLabel_->setObjectName(side == Right ? "bubbleSelf" : "bubbleOther");
    QString wrappedContent = content;
    while (wrappedContent.endsWith('\n')) {
        wrappedContent.chop(1);
    }
    contentText_ = wrappedContent;
    bubbleLabel_->setText(wrappedContent);
    bubbleLabel_->setTextFormat(Qt::PlainText);
    bubbleLabel_->setWordWrap(true);
    bubbleLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    bubbleLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    bubbleLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    fitBubbleSize();

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
}

void MessageWidget::fitBubbleSize() {
    bubbleLabel_->ensurePolished();
    const QFontMetrics fm(bubbleLabel_->fontMetrics());
    const QStringList lines = contentText_.split('\n');
    int maxLineW = 0;
    for (const QString &line : lines) {
        maxLineW = qMax(maxLineW, fm.horizontalAdvance(line));
    }

    const int horizontalPadding = kBubbleHorizontalPadding * 2;
    const int verticalPadding = kBubbleVerticalPadding * 2;
    const int maxContentWidth = qMax(kMinBubbleContentWidth, maxBubbleWidth_ - horizontalPadding);
    const int contentWidth = qBound(kMinBubbleContentWidth, maxLineW, maxContentWidth);

    QRect textRect = fm.boundingRect(QRect(0, 0, contentWidth, 10000),
                                     Qt::TextWordWrap | Qt::TextWrapAnywhere,
                                     contentText_);
    const int bubbleW = contentWidth + horizontalPadding;
    const int bubbleH = qMax(fm.lineSpacing() + verticalPadding,
                             int(std::ceil(textRect.height())) + verticalPadding);
    bubbleLabel_->setFixedSize(bubbleW, bubbleH);

    setMinimumHeight(qMax(70, bubbleH + 46));
}

void MessageWidget::updateWidth(int newMaxWidth) {
    if (maxBubbleWidth_ == newMaxWidth) return;
    maxBubbleWidth_ = newMaxWidth;
    fitBubbleSize();
}
