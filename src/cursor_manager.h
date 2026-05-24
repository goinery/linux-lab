#ifndef CURSOR_MANAGER_H
#define CURSOR_MANAGER_H

#include <QCursor>
#include <QPixmap>

/**
 * @brief 自定义光标管理器：从 PNG 资源加载各状态光标。
 *
 * 光标 PNG 文件位于 resources/cursors/ 目录，通过 Qt 资源系统
 * 在运行时加载。如果 PNG 加载失败，自动回退为 Qt 内置光标。
 *
 * 当前占位光标为 32×32 全透明 PNG —— 替换为实际光标图后重新编译即可生效。
 * 替换时请注意 hotspot（热点）坐标是否需要调整，可在 loadCursor() 调用中修改。
 *
 * 光标文件清单：
 *   cursor_arrow.png      → 默认箭头光标    (hotspot: 20,14)
 *   cursor_hand.png       → 手型光标(可点击) (hotspot: 27,11)
 *   cursor_size_hor.png   → 水平调整大小     (hotspot: 32,31)
 *   cursor_size_ver.png   → 垂直调整大小     (hotspot: 32,31)
 *   cursor_size_fdiag.png → ↘对角调整大小   (hotspot: 32,31)
 *   cursor_size_bdiag.png → ↗对角调整大小   (hotspot: 32,31)
 *   cursor_move..png      → 移动窗口光标     (hotspot: 32,31)
 *   cursor_ibeam.png      → 文本选择光标     (hotspot: 32,31)
 */
class CursorManager {
public:
    /// 获取单例
    static CursorManager &instance();

    /// 初始化：从 Qt 资源加载所有自定义光标 PNG
    void initialize();

    // ---- 各状态光标访问 ----
    const QCursor &arrow()      const;
    const QCursor &hand()       const;
    const QCursor &sizeHor()    const;
    const QCursor &sizeVer()    const;
    const QCursor &sizeFDiag()  const;
    const QCursor &sizeBDiag()  const;
    const QCursor &move()       const;
    const QCursor &ibeam()      const;

private:
    CursorManager();

    /**
     * 加载单个光标：若 PNG 加载成功则创建自定义 QCursor，
     * 否则回退到 Qt 内置光标 shape。
     */
    QCursor loadCursor(const QString &resourcePath,
                       int hotspotX, int hotspotY,
                       Qt::CursorShape fallback);

    QCursor arrowCursor_;
    QCursor handCursor_;
    QCursor sizeHorCursor_;
    QCursor sizeVerCursor_;
    QCursor sizeFDiagCursor_;
    QCursor sizeBDiagCursor_;
    QCursor moveCursor_;
    QCursor ibeamCursor_;
};

#endif // CURSOR_MANAGER_H
