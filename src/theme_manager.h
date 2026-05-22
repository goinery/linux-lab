#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <QString>
#include <QVector>

/**
 * @brief 主题管理器：集中维护所有主题配色。
 *
 * 设计目标：避免每个主题重复书写整套 QSS。仅有一份模板
 *   (resources/themes/template.qss) 描述结构与选择器，
 *   各主题只通过一组颜色（ThemePalette）参数化。
 *
 * 修改 / 新增主题：编辑本类 .cpp 中的 palette 工厂即可。
 * 修改样式结构：编辑 template.qss 即可。
 */
class ThemeManager {
public:
    /// 主题调色板：模板中每个 @token@ 在此都有对应字段。
    struct ThemePalette {
        QString id;     // 资源标识，如 "gray"
        QString name;   // 显示名，如 "素白"

        // ---- 字体 ----
        QString fontFamily;

        // ---- 品牌主色 ----
        QString primary;        // 主色（侧栏Logo、发送按钮、选中态、焦点边框）
        QString primaryHover;   // 主色悬停态
        QString primaryDark;    // 主色加深（用户列表选中边框、Radio 选中边框）
        QString onPrimary;      // 主色背景上的文字色

        // ---- 背景层 ----
        QString bgWindow;       // 窗口根背景
        QString bgTitleBar;     // 标题栏
        QString bgTitleBarIcon; // 标题栏圆形图标背景
        QString bgSideBar;      // 侧边栏
        QString bgTopBar;       // 顶部 TopBar
        QString bgPanelLeft;    // 左侧用户列表面板
        QString bgPanelRight;   // 右侧聊天面板
        QString bgCard;         // 卡片
        QString bgInput;        // 输入框 / 列表 / Radio Indicator
        QString bgChatArea;     // 聊天消息滚动区
        QString bgPageScroll;   // 页面滚动区（深色主题给纯色，浅色给 transparent）
        QString bgMenu;         // 弹出菜单
        QString bgMenuBar;      // 菜单栏
        QString bgStatusBar;    // 状态栏
        QString bgButton;       // 通用按钮
        QString bgBubbleOther;  // 他人气泡
        QString bgTooltip;      // ToolTip 背景
        QString bgTopBarBadge;  // TopBar 上的徽章

        // ---- 交互态背景 ----
        QString bgMenuBarItemHover;
        QString bgMenuItemHover;
        QString bgButtonHover;
        QString bgWinBtnHover;
        QString bgWinCloseHover;
        QString bgSideBtnHover;
        QString bgSideBtnPressed;
        QString bgUserListItemHover;

        // ---- 边框 ----
        QString borderLine;       // 标题栏、状态栏的细分割线
        QString borderPanel;      // TopBar、左右面板、聊天区共享的面板边框
        QString borderInput;      // 输入框、列表、菜单边框
        QString borderCard;       // 卡片边框
        QString borderBubbleOther;
        QString borderAvatarSelf;
        QString borderAvatarOther;
        QString borderRadio;

        // ---- 文本 ----
        QString textBody;             // QWidget 基础色
        QString textLabel;            // QLabel
        QString textTitle;            // 标题（标题栏、TopBar标题、CardTitle、用户列表标题、聊天标题）
        QString textSubtitle;         // TopBar 副标题
        QString textInput;            // 输入框内文字
        QString textInputPlaceholder; // 输入框占位
        QString textListItem;         // 列表项
        QString textButton;           // 通用按钮文字
        QString textMenuItem;         // 弹出菜单项
        QString textMenuBar;          // 菜单栏文字
        QString textStatusBar;
        QString textTooltip;
        QString textWinBtn;           // 窗口按钮（最小化等）
        QString textSideBtn;          // 侧栏按钮
        QString textBubbleOther;
        QString textNameSelf;
        QString textNameOther;
        QString textTimeLabel;
        QString textSystemMessage;
        QString textStatusLabel;      // 错误提示色

        // ---- 其它 ----
        QString avatarOtherBg;
        QString avatarOtherFg;
        QString scrollHandle;
        QString scrollHandleHover;
        QString splitterBg;
    };

    /// 获取所有主题（按索引顺序：0=素白, 1=森氧, 2=云海, 3=夜航）。
    static const QVector<ThemePalette> &palettes();

    /// 按索引获取主题（越界则取 0）。
    static const ThemePalette &paletteAt(int index);

    /// 主题总数。
    static int themeCount();

    /// 渲染：加载模板并替换占位符，得到可直接 setStyleSheet 的字符串。
    static QString render(const ThemePalette &palette);

    /// 便捷：按索引渲染。
    static QString renderForIndex(int index);

private:
    static QString loadTemplate();
};

#endif // THEME_MANAGER_H
