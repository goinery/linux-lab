#include "message_widget.h"
#include <QFontMetrics>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextFrameFormat>

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

    bubbleEdit_ = new QTextEdit;
    bubbleEdit_->setObjectName(side == Right ? "bubbleSelf" : "bubbleOther");
    bubbleEdit_->setReadOnly(true);
    QString wrappedContent = content;
    while (wrappedContent.endsWith('\n')) {
        wrappedContent.chop(1);
    }
    bubbleEdit_->setPlainText(wrappedContent);
    bubbleEdit_->setFrameStyle(QFrame::NoFrame);
    bubbleEdit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bubbleEdit_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bubbleEdit_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    bubbleEdit_->setTabChangesFocus(true);
    bubbleEdit_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    fitBubbleSize();

    QLabel *timeLabel = new QLabel(time);
    timeLabel->setObjectName("timeLabel");
    QFont timeFont;
    timeFont.setPointSize(9);
    timeLabel->setFont(timeFont);

    bubbleLayout_->addWidget(nameLabel);
    bubbleLayout_->addWidget(bubbleEdit_);
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
    bubbleEdit_->ensurePolished();
    QTextDocument *doc = bubbleEdit_->document();

    const int margin = 12;
    doc->setDocumentMargin(margin);

    QTextFrameFormat rootFormat = doc->rootFrame()->frameFormat();
    rootFormat.setMargin(0);
    rootFormat.setPadding(0);
    doc->rootFrame()->setFrameFormat(rootFormat);

    const QFontMetrics fm(bubbleEdit_->fontMetrics());
    const QStringList lines = contentText_.split('\n');
    int maxLineW = 0;
    for (const QString &line : lines) {
        maxLineW = qMax(maxLineW, fm.horizontalAdvance(line));
    }

    const int totalMargin = margin * 2;
    const int bubbleW = qBound(20, qMin(maxLineW + totalMargin + 6, maxBubbleWidth_), maxBubbleWidth_);

    doc->setTextWidth(bubbleW - totalMargin);
    bubbleEdit_->setFixedWidth(bubbleW);

    const int bubbleH = qMax(32, int(doc->size().height()));
    bubbleEdit_->setFixedHeight(bubbleH);

    setMinimumHeight(qMax(70, bubbleH + 46));
}

void MessageWidget::updateWidth(int newMaxWidth) {
    if (maxBubbleWidth_ == newMaxWidth) return;
    maxBubbleWidth_ = newMaxWidth;
    fitBubbleSize();
}
