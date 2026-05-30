#include "message_widget.h"
#include <QAbstractTextDocumentLayout>
#include <QFrame>
#include <QFontMetrics>
#include <QScrollBar>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextOption>
#include <QWheelEvent>
#include <cmath>

namespace {

constexpr int kBubbleHorizontalPadding = 10;
constexpr int kBubbleVerticalPadding = 6;
constexpr int kMinBubbleContentWidth = 32;
constexpr int kMinBubbleHeight = 34;

class BubbleTextEdit : public QTextEdit {
public:
    explicit BubbleTextEdit(QWidget *parent = nullptr)
        : QTextEdit(parent) {
        setReadOnly(true);
        setAcceptRichText(false);
        setFrameStyle(QFrame::NoFrame);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setLineWrapMode(QTextEdit::FixedPixelWidth);
        setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setTabChangesFocus(true);
        setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        setAutoFillBackground(false);
        viewport()->setAutoFillBackground(false);
        viewport()->setAttribute(Qt::WA_TranslucentBackground);
        document()->setDocumentMargin(0);
        applyWrapOption(document());
    }

    void setBubbleText(const QString &text) {
        setPlainText(text);
    }

    QSize updateContentSize(int contentWidth) {
        ensurePolished();

        document()->setDefaultFont(font());
        document()->setDocumentMargin(0);
        applyWrapOption(document());

        setLineWrapColumnOrWidth(contentWidth);
        document()->setTextWidth(contentWidth);

        const QSizeF textSize = document()->documentLayout()->documentSize();
        const int textHeight = int(std::ceil(textSize.height()));
        setFixedSize(contentWidth, textHeight);
        verticalScrollBar()->setValue(0);
        horizontalScrollBar()->setValue(0);
        return QSize(contentWidth, textHeight);
    }

    int naturalTextWidth() const {
        QTextDocument measureDoc;
        measureDoc.setDefaultFont(font());
        measureDoc.setDocumentMargin(0);
        applyWrapOption(&measureDoc);
        measureDoc.setPlainText(toPlainText());
        measureDoc.setTextWidth(-1);
        return int(std::ceil(measureDoc.idealWidth()));
    }

protected:
    void wheelEvent(QWheelEvent *event) override {
        event->ignore();
    }

private:
    static void applyWrapOption(QTextDocument *doc) {
        QTextOption option = doc->defaultTextOption();
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        option.setAlignment(Qt::AlignLeft);
        doc->setDefaultTextOption(option);
    }

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
    bubbleFrame_ = new QFrame;
    bubbleFrame_->setObjectName(side == Right ? "bubbleSelf" : "bubbleOther");
    bubbleFrame_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QVBoxLayout *bubbleFrameLayout = new QVBoxLayout(bubbleFrame_);
    const int bubbleBorder = side == Left ? 1 : 0;
    bubbleFrameLayout->setContentsMargins(kBubbleHorizontalPadding + bubbleBorder,
                                          kBubbleVerticalPadding + bubbleBorder,
                                          kBubbleHorizontalPadding + bubbleBorder,
                                          kBubbleVerticalPadding + bubbleBorder);
    bubbleFrameLayout->setSpacing(0);

    bubbleEdit_ = new BubbleTextEdit(bubbleFrame_);
    bubbleEdit_->setObjectName("bubbleText");
    static_cast<BubbleTextEdit *>(bubbleEdit_)->setBubbleText(wrappedContent);
    bubbleFrameLayout->addWidget(bubbleEdit_);

    fitBubbleSize();

    QLabel *timeLabel = new QLabel(time);
    timeLabel->setObjectName("timeLabel");
    QFont timeFont;
    timeFont.setPointSize(9);
    timeLabel->setFont(timeFont);

    bubbleLayout_->addWidget(nameLabel);
    bubbleLayout_->addWidget(bubbleFrame_);
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
    bubbleFrame_->ensurePolished();
    bubbleEdit_->ensurePolished();

    const int borderWidth = bubbleFrame_->objectName() == QLatin1String("bubbleOther") ? 1 : 0;
    const int horizontalChrome = (kBubbleHorizontalPadding + borderWidth) * 2;
    const int verticalChrome = (kBubbleVerticalPadding + borderWidth) * 2;
    const int maxContentWidth = qMax(kMinBubbleContentWidth,
                                     maxBubbleWidth_ - horizontalChrome);
    const int naturalWidth = qMax(kMinBubbleContentWidth,
                                  static_cast<BubbleTextEdit *>(bubbleEdit_)->naturalTextWidth());
    const int contentWidth = qBound(kMinBubbleContentWidth, naturalWidth, maxContentWidth);
    const QSize textSize = static_cast<BubbleTextEdit *>(bubbleEdit_)->updateContentSize(contentWidth);

    const QFontMetrics fm(bubbleEdit_->fontMetrics());
    const int bubbleHeight = qMax(kMinBubbleHeight,
                                  qMax(fm.lineSpacing() + verticalChrome,
                                       textSize.height() + verticalChrome));
    bubbleFrame_->setFixedSize(contentWidth + horizontalChrome, bubbleHeight);
    setMinimumHeight(qMax(70, bubbleFrame_->height() + 46));
}

void MessageWidget::updateWidth(int newMaxWidth) {
    if (maxBubbleWidth_ == newMaxWidth) return;
    maxBubbleWidth_ = newMaxWidth;
    fitBubbleSize();
}
