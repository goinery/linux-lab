#include "message_widget.h"
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <cmath>

namespace {

constexpr int kBubbleHorizontalPadding = 10;
constexpr int kBubbleVerticalPadding = 6;
constexpr int kMinBubbleContentWidth = 32;

class BubbleTextWidget : public QWidget {
public:
    explicit BubbleTextWidget(const QString &text, QWidget *parent = nullptr)
        : QWidget(parent), text_(text) {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setAttribute(Qt::WA_StyledBackground, true);
    }

    QSize textSizeForWidth(int width) const {
        QTextDocument doc;
        configureDocument(&doc, width);
        return QSize(int(std::ceil(doc.size().width())),
                     int(std::ceil(doc.size().height())));
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);

        QStyleOption option;
        option.initFrom(this);
        style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

        QTextDocument doc;
        configureDocument(&doc, qMax(0, width() - kBubbleHorizontalPadding * 2));
        painter.translate(kBubbleHorizontalPadding, kBubbleVerticalPadding);
        doc.drawContents(&painter);
    }

private:
    void configureDocument(QTextDocument *doc, int width) const {
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        option.setAlignment(Qt::AlignLeft);

        doc->setDocumentMargin(0);
        doc->setDefaultFont(font());
        doc->setDefaultTextOption(option);
        doc->setPlainText(text_);
        doc->setTextWidth(width);

        QTextCursor cursor(doc);
        cursor.select(QTextCursor::Document);
        QTextCharFormat format;
        format.setForeground(palette().color(QPalette::WindowText));
        cursor.mergeCharFormat(format);
    }

    QString text_;
};

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

    QString wrappedContent = content;
    while (wrappedContent.endsWith('\n')) {
        wrappedContent.chop(1);
    }
    contentText_ = wrappedContent;
    bubbleWidget_ = new BubbleTextWidget(wrappedContent);
    bubbleWidget_->setObjectName(side == Right ? "bubbleSelf" : "bubbleOther");

    fitBubbleSize();

    QLabel *timeLabel = new QLabel(time);
    timeLabel->setObjectName("timeLabel");
    QFont timeFont;
    timeFont.setPointSize(9);
    timeLabel->setFont(timeFont);

    bubbleLayout_->addWidget(nameLabel);
    bubbleLayout_->addWidget(bubbleWidget_);
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
    bubbleWidget_->ensurePolished();
    const QFontMetrics fm(bubbleWidget_->fontMetrics());
    const QStringList lines = contentText_.split('\n');
    int maxLineW = 0;
    for (const QString &line : lines) {
        maxLineW = qMax(maxLineW, fm.horizontalAdvance(line));
    }

    const int horizontalPadding = kBubbleHorizontalPadding * 2;
    const int verticalPadding = kBubbleVerticalPadding * 2;
    const int maxContentWidth = qMax(kMinBubbleContentWidth, maxBubbleWidth_ - horizontalPadding);
    const int contentWidth = qBound(kMinBubbleContentWidth, maxLineW, maxContentWidth);

    QSize textSize = static_cast<BubbleTextWidget *>(bubbleWidget_)->textSizeForWidth(contentWidth);
    const int bubbleW = contentWidth + horizontalPadding;
    const int bubbleH = qMax(fm.lineSpacing() + verticalPadding,
                             textSize.height() + verticalPadding);
    bubbleWidget_->setFixedSize(bubbleW, bubbleH);

    setMinimumHeight(qMax(70, bubbleH + 46));
}

void MessageWidget::updateWidth(int newMaxWidth) {
    if (maxBubbleWidth_ == newMaxWidth) return;
    maxBubbleWidth_ = newMaxWidth;
    fitBubbleSize();
}
