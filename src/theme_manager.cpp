#include "theme_manager.h"

#include <QDebug>
#include <QFile>

namespace {

// ===== 主题工厂 ============================================================
// 修改某主题颜色 → 编辑下方对应函数中的字段值即可。
// 新增主题 → 复制其中一个函数，调整配色，并加入 palettes() 列表。

ThemeManager::ThemePalette grayPalette() {
    ThemeManager::ThemePalette p;
    p.id   = QStringLiteral("gray");
    p.name = QStringLiteral("素白");
    p.primary       = "#5c6bc0";
    p.primaryHover  = "#4a5ab0";
    p.primaryDark   = "#3a4aa0";
    p.onPrimary     = "#ffffff";

    p.bgWindow       = "#f2f3f5";
    p.bgTitleBar     = "#ffffff";
    p.bgTitleBarIcon = "#eef0fa";
    p.bgSideBar      = "#ffffff";
    p.bgTopBar       = "#ffffff";
    p.bgPanelLeft    = "#fafbfc";
    p.bgPanelRight   = "#f2f3f5";
    p.bgCard         = "#ffffff";
    p.bgInput        = "#ffffff";
    p.bgChatArea     = "#fafbfc";
    p.bgPageScroll   = "transparent";
    p.bgMenu         = "#ffffff";
    p.bgMenuBar      = "#ffffff";
    p.bgStatusBar    = "#ffffff";
    p.bgButton       = "#f7f8fa";
    p.bgBubbleOther  = "#ffffff";
    p.bgTooltip      = "#444444";
    p.bgTopBarBadge  = "#eef0fa";

    p.bgMenuBarItemHover = "#eef0f5";
    p.bgMenuItemHover    = "#eef0f5";
    p.bgButtonHover      = "#eef0f5";
    p.bgWinBtnHover      = "#e8eaed";
    p.bgWinCloseHover    = "#e07070";
    p.bgSideBtnHover     = "#eef0f5";
    p.bgSideBtnPressed   = "#dde1ec";
    p.bgUserListItemHover = "#eef0f5";

    p.borderLine        = "#e0e2e5";
    p.borderPanel       = "#e8eaed";
    p.borderInput       = "#e0e4ec";
    p.borderCard        = "#e8eaed";
    p.borderBubbleOther = "#e8eaed";
    p.borderAvatarSelf  = "#8c98d5";
    p.borderAvatarOther = "#bbbbbb";
    p.borderRadio       = "#c0c4cc";

    p.textBody             = "#333333";
    p.textLabel            = "#444444";
    p.textTitle            = "#333333";
    p.textSubtitle         = "#6b7280";
    p.textInput            = "#333333";
    p.textInputPlaceholder = "#a0a4ad";
    p.textListItem         = "#666666";
    p.textButton           = "#444444";
    p.textMenuItem         = "#333333";
    p.textMenuBar          = "#333333";
    p.textStatusBar        = "#888888";
    p.textTooltip          = "#ffffff";
    p.textWinBtn           = "#666666";
    p.textSideBtn          = "#888888";
    p.textBubbleOther      = "#333333";
    p.textNameSelf         = "#5c6bc0";
    p.textNameOther        = "#888888";
    p.textTimeLabel        = "#8a9099";
    p.textSystemMessage    = "#7f858d";
    p.textStatusLabel      = "#c0392b";
    p.textStatusOk         = "#2e8b57";

    p.avatarOtherBg      = "#888888";
    p.avatarOtherFg      = "#ffffff";
    p.badgeBg            = "#e5484d";
    p.badgeFg            = "#ffffff";
    p.scrollHandle       = "#c0c4cc";
    p.scrollHandleHover  = "#909399";
    p.splitterBg         = "#e0e2e5";

    return p;
}

ThemeManager::ThemePalette mintPalette() {
    ThemeManager::ThemePalette p;
    p.id   = QStringLiteral("mint");
    p.name = QStringLiteral("森氧");
    p.primary       = "#5c9b6f";
    p.primaryHover  = "#4d8b60";
    p.primaryDark   = "#4a8760";
    p.onPrimary     = "#ffffff";

    p.bgWindow       = "#e8efe6";
    p.bgTitleBar     = "#d4e2d0";
    p.bgTitleBarIcon = "#f0f7ef";
    p.bgSideBar      = "#d8e5d4";
    p.bgTopBar       = "#f6faf3";
    p.bgPanelLeft    = "#eaf3ea";
    p.bgPanelRight   = "#f6faf4";
    p.bgCard         = "#fafef9";
    p.bgInput        = "#ffffff";
    p.bgChatArea     = "#f9fdf6";
    p.bgPageScroll   = "transparent";
    p.bgMenu         = "#ffffff";
    p.bgMenuBar      = "#f8faf6";
    p.bgStatusBar    = "#f0f6ef";
    p.bgButton       = "#f6faf4";
    p.bgBubbleOther  = "#eaf5e8";
    p.bgTooltip      = "#2d4a34";
    p.bgTopBarBadge  = "#e0f0e0";

    p.bgMenuBarItemHover = "#d5ebd5";
    p.bgMenuItemHover    = "#e6f4e6";
    p.bgButtonHover      = "#e2f0e2";
    p.bgWinBtnHover      = "#c4d6c0";
    p.bgWinCloseHover    = "#e07070";
    p.bgSideBtnHover     = "#c4d8c0";
    p.bgSideBtnPressed   = "#aec8aa";
    p.bgUserListItemHover = "#deefde";

    p.borderLine        = "#c4d6c0";
    p.borderPanel       = "#d4e4d4";
    p.borderInput       = "#c8dcc8";
    p.borderCard        = "#d9e8d9";
    p.borderBubbleOther = "#c6dcc6";
    p.borderAvatarSelf  = "#88c498";
    p.borderAvatarOther = "#a3bed4";
    p.borderRadio       = "#bccfbc";

    p.textBody             = "#2d3748";
    p.textLabel            = "#2d4434";
    p.textTitle            = "#2d4a34";
    p.textSubtitle         = "#6b876f";
    p.textInput            = "#243a2a";
    p.textInputPlaceholder = "#7a957d";
    p.textListItem         = "#2d4434";
    p.textButton           = "#2d4a34";
    p.textMenuItem         = "#2d4a34";
    p.textMenuBar          = "#2d4a34";
    p.textStatusBar        = "#39503a";
    p.textTooltip          = "#ffffff";
    p.textWinBtn           = "#4a6e52";
    p.textSideBtn          = "#3b5a3e";
    p.textBubbleOther      = "#243a2a";
    p.textNameSelf         = "#667a68";
    p.textNameOther        = "#667a68";
    p.textTimeLabel        = "#667a68";
    p.textSystemMessage    = "#667a68";
    p.textStatusLabel      = "#c0392b";
    p.textStatusOk         = "#3d7a4e";

    p.avatarOtherBg      = "#7a9db8";
    p.avatarOtherFg      = "#ffffff";
    p.badgeBg            = "#d9534f";
    p.badgeFg            = "#ffffff";
    p.scrollHandle       = "#b8ccb8";
    p.scrollHandleHover  = "#9cb89c";
    p.splitterBg         = "#d0e0d0";

    return p;
}

ThemeManager::ThemePalette skyPalette() {
    ThemeManager::ThemePalette p;
    p.id   = QStringLiteral("sky");
    p.name = QStringLiteral("云海");
    p.primary       = "#4a8fd4";
    p.primaryHover  = "#3b7ec2";
    p.primaryDark   = "#3c7bc0";
    p.onPrimary     = "#ffffff";

    p.bgWindow       = "#e7f0f8";
    p.bgTitleBar     = "#cddfef";
    p.bgTitleBarIcon = "#eef5fc";
    p.bgSideBar      = "#d2e4f4";
    p.bgTopBar       = "#f5faff";
    p.bgPanelLeft    = "#e7f0fa";
    p.bgPanelRight   = "#f4f8fd";
    p.bgCard         = "#fbfdff";
    p.bgInput        = "#ffffff";
    p.bgChatArea     = "#f9fcff";
    p.bgPageScroll   = "transparent";
    p.bgMenu         = "#ffffff";
    p.bgMenuBar      = "#f6f9fd";
    p.bgStatusBar    = "#eef5fc";
    p.bgButton       = "#f5f9ff";
    p.bgBubbleOther  = "#e9f1fc";
    p.bgTooltip      = "#2a4163";
    p.bgTopBarBadge  = "#ddeefc";

    p.bgMenuBarItemHover = "#d9e9f7";
    p.bgMenuItemHover    = "#e5f0fa";
    p.bgButtonHover      = "#dfecf9";
    p.bgWinBtnHover      = "#bcd0e8";
    p.bgWinCloseHover    = "#e07070";
    p.bgSideBtnHover     = "#bed5ec";
    p.bgSideBtnPressed   = "#a8c6e2";
    p.bgUserListItemHover = "#d9e9f7";

    p.borderLine        = "#bfd4e8";
    p.borderPanel       = "#d2e4f3";
    p.borderInput       = "#c6d8ec";
    p.borderCard        = "#d8e6f5";
    p.borderBubbleOther = "#c4d8ee";
    p.borderAvatarSelf  = "#7db5e6";
    p.borderAvatarOther = "#afc4db";
    p.borderRadio       = "#bccde1";

    p.textBody             = "#2d3a52";
    p.textLabel            = "#2a3f5e";
    p.textTitle            = "#2a4163";
    p.textSubtitle         = "#60799e";
    p.textInput            = "#223754";
    p.textInputPlaceholder = "#7c92b0";
    p.textListItem         = "#2a3f5e";
    p.textButton           = "#2a4163";
    p.textMenuItem         = "#2a4163";
    p.textMenuBar          = "#2a4163";
    p.textStatusBar        = "#344a67";
    p.textTooltip          = "#ffffff";
    p.textWinBtn           = "#485e7a";
    p.textSideBtn          = "#35567a";
    p.textBubbleOther      = "#223754";
    p.textNameSelf         = "#5c708f";
    p.textNameOther        = "#5c708f";
    p.textTimeLabel        = "#5c708f";
    p.textSystemMessage    = "#5c708f";
    p.textStatusLabel      = "#c03b46";
    p.textStatusOk         = "#2f7d5a";

    p.avatarOtherBg      = "#8ba4c2";
    p.avatarOtherFg      = "#ffffff";
    p.badgeBg            = "#e5484d";
    p.badgeFg            = "#ffffff";
    p.scrollHandle       = "#b8c8dc";
    p.scrollHandleHover  = "#9db0c8";
    p.splitterBg         = "#cdddec";

    return p;
}

ThemeManager::ThemePalette nightPalette() {
    ThemeManager::ThemePalette p;
    p.id   = QStringLiteral("night");
    p.name = QStringLiteral("夜航");
    p.primary       = "#2d9eb6";
    p.primaryHover  = "#268ea5";
    p.primaryDark   = "#268ca3";
    p.onPrimary     = "#f4fbff";

    p.bgWindow       = "#0a1019";
    p.bgTitleBar     = "#111b2e";
    p.bgTitleBarIcon = "#1a2a42";
    p.bgSideBar      = "#10192a";
    p.bgTopBar       = "#131f34";
    p.bgPanelLeft    = "#101e32";
    p.bgPanelRight   = "#0f1929";
    p.bgCard         = "#132138";
    p.bgInput        = "#0e1826";
    p.bgChatArea     = "#101c2e";
    p.bgPageScroll   = "#0a1019";
    p.bgMenu         = "#182438";
    p.bgMenuBar      = "#141f32";
    p.bgStatusBar    = "#0f192a";
    p.bgButton       = "#162438";
    p.bgBubbleOther  = "#192c44";
    p.bgTooltip      = "#182438";
    p.bgTopBarBadge  = "#1a3048";

    p.bgMenuBarItemHover = "#233652";
    p.bgMenuItemHover    = "#253a56";
    p.bgButtonHover      = "#1f3048";
    p.bgWinBtnHover      = "#1f3148";
    p.bgWinCloseHover    = "#c04444";
    p.bgSideBtnHover     = "#1a2a42";
    p.bgSideBtnPressed   = "#243652";
    p.bgUserListItemHover = "#243856";

    p.borderLine        = "#1f3148";
    p.borderPanel       = "#243858";
    p.borderInput       = "#294868";
    p.borderCard        = "#274668";
    p.borderBubbleOther = "#335378";
    p.borderAvatarSelf  = "#55c0d0";
    p.borderAvatarOther = "#5c749c";
    p.borderRadio       = "#2f4a6a";

    p.textBody             = "#c8d6e5";
    p.textLabel            = "#c4d6eb";
    p.textTitle            = "#dce8f8";
    p.textSubtitle         = "#6e86a8";
    p.textInput            = "#d4e4f6";
    p.textInputPlaceholder = "#6a80a0";
    p.textListItem         = "#c4d6eb";
    p.textButton           = "#c8daf2";
    p.textMenuItem         = "#d0e0f5";
    p.textMenuBar          = "#ccdef8";
    p.textStatusBar        = "#bccadf";
    p.textTooltip          = "#dce8f8";
    p.textWinBtn           = "#6c84a2";
    p.textSideBtn          = "#7a96b8";
    p.textBubbleOther      = "#d2e2f5";
    p.textNameSelf         = "#64c8d8";
    p.textNameOther        = "#8eb8f0";
    p.textTimeLabel        = "#788ba8";
    p.textSystemMessage    = "#788ba8";
    p.textStatusLabel      = "#ff8b94";
    p.textStatusOk         = "#5dd39e";

    p.avatarOtherBg      = "#3a5075";
    p.avatarOtherFg      = "#dce8f5";
    p.badgeBg            = "#d43a3c";
    p.badgeFg            = "#ffffff";
    p.scrollHandle       = "#2a4462";
    p.scrollHandleHover  = "#345478";
    p.splitterBg         = "#243654";

    return p;
}

} // namespace

// ===== ThemeManager 实现 ===================================================

const QVector<ThemeManager::ThemePalette> &ThemeManager::palettes() {
    static const QVector<ThemePalette> kPalettes = {
        grayPalette(),
        mintPalette(),
        skyPalette(),
        nightPalette(),
    };
    return kPalettes;
}

const ThemeManager::ThemePalette &ThemeManager::paletteAt(int index) {
    const auto &all = palettes();
    if (index < 0 || index >= all.size()) {
        return all.at(0);
    }
    return all.at(index);
}

int ThemeManager::themeCount() {
    return palettes().size();
}

QString ThemeManager::loadTemplate() {
    QFile file(QStringLiteral(":/theme_template.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ThemeManager: failed to open template" << file.errorString();
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

QString ThemeManager::render(const ThemePalette &p) {
    QString tpl = loadTemplate();
    if (tpl.isEmpty()) {
        return QString();
    }

    // 单一映射表：每新增一个 token，仅需在 ThemePalette 加字段并在此追加一行。
    const QVector<QPair<QString, QString>> tokens = {
        { "@primary@",               p.primary },
        { "@primaryHover@",          p.primaryHover },
        { "@primaryDark@",           p.primaryDark },
        { "@onPrimary@",             p.onPrimary },

        { "@bgWindow@",              p.bgWindow },
        { "@bgTitleBar@",            p.bgTitleBar },
        { "@bgTitleBarIcon@",        p.bgTitleBarIcon },
        { "@bgSideBar@",             p.bgSideBar },
        { "@bgTopBar@",              p.bgTopBar },
        { "@bgPanelLeft@",           p.bgPanelLeft },
        { "@bgPanelRight@",          p.bgPanelRight },
        { "@bgCard@",                p.bgCard },
        { "@bgInput@",               p.bgInput },
        { "@bgChatArea@",            p.bgChatArea },
        { "@bgPageScroll@",          p.bgPageScroll },
        { "@bgMenu@",                p.bgMenu },
        { "@bgMenuBar@",             p.bgMenuBar },
        { "@bgStatusBar@",           p.bgStatusBar },
        { "@bgButton@",              p.bgButton },
        { "@bgBubbleOther@",         p.bgBubbleOther },
        { "@bgTooltip@",             p.bgTooltip },
        { "@bgTopBarBadge@",         p.bgTopBarBadge },

        { "@bgMenuBarItemHover@",    p.bgMenuBarItemHover },
        { "@bgMenuItemHover@",       p.bgMenuItemHover },
        { "@bgButtonHover@",         p.bgButtonHover },
        { "@bgWinBtnHover@",         p.bgWinBtnHover },
        { "@bgWinCloseHover@",       p.bgWinCloseHover },
        { "@bgSideBtnHover@",        p.bgSideBtnHover },
        { "@bgSideBtnPressed@",      p.bgSideBtnPressed },
        { "@bgUserListItemHover@",   p.bgUserListItemHover },

        { "@borderLine@",            p.borderLine },
        { "@borderPanel@",           p.borderPanel },
        { "@borderInput@",           p.borderInput },
        { "@borderCard@",            p.borderCard },
        { "@borderBubbleOther@",     p.borderBubbleOther },
        { "@borderAvatarSelf@",      p.borderAvatarSelf },
        { "@borderAvatarOther@",     p.borderAvatarOther },
        { "@borderRadio@",           p.borderRadio },

        { "@textBody@",              p.textBody },
        { "@textLabel@",             p.textLabel },
        { "@textTitle@",             p.textTitle },
        { "@textSubtitle@",          p.textSubtitle },
        { "@textInput@",             p.textInput },
        { "@textInputPlaceholder@",  p.textInputPlaceholder },
        { "@textListItem@",          p.textListItem },
        { "@textButton@",            p.textButton },
        { "@textMenuItem@",          p.textMenuItem },
        { "@textMenuBar@",           p.textMenuBar },
        { "@textStatusBar@",         p.textStatusBar },
        { "@textTooltip@",           p.textTooltip },
        { "@textWinBtn@",            p.textWinBtn },
        { "@textSideBtn@",           p.textSideBtn },
        { "@textBubbleOther@",       p.textBubbleOther },
        { "@textNameSelf@",          p.textNameSelf },
        { "@textNameOther@",         p.textNameOther },
        { "@textTimeLabel@",         p.textTimeLabel },
        { "@textSystemMessage@",     p.textSystemMessage },
        { "@textStatusLabel@",       p.textStatusLabel },
        { "@textStatusOk@",          p.textStatusOk },

        { "@avatarOtherBg@",         p.avatarOtherBg },
        { "@avatarOtherFg@",         p.avatarOtherFg },
        { "@badgeBg@",               p.badgeBg },
        { "@badgeFg@",               p.badgeFg },
        { "@scrollHandle@",          p.scrollHandle },
        { "@scrollHandleHover@",     p.scrollHandleHover },
        { "@splitterBg@",            p.splitterBg },
    };

    for (const auto &kv : tokens) {
        tpl.replace(kv.first, kv.second);
    }
    return tpl;
}

QString ThemeManager::renderForIndex(int index) {
    return render(paletteAt(index));
}
