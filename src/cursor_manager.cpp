#include "cursor_manager.h"

#include <QDebug>

CursorManager &CursorManager::instance() {
    static CursorManager mgr;
    return mgr;
}

CursorManager::CursorManager()
    : arrowCursor_(Qt::ArrowCursor)
    , handCursor_(Qt::PointingHandCursor)
    , sizeHorCursor_(Qt::SizeHorCursor)
    , sizeVerCursor_(Qt::SizeVerCursor)
    , sizeFDiagCursor_(Qt::SizeFDiagCursor)
    , sizeBDiagCursor_(Qt::SizeBDiagCursor)
    , moveCursor_(Qt::SizeAllCursor)
    , ibeamCursor_(Qt::IBeamCursor) {}

void CursorManager::initialize() {
    // ---- 加载自定义光标 PNG ----
    // hotspot 坐标需根据实际光标图调整，当前为常见默认值

    arrowCursor_     = loadCursor(":/cursors/cursor_arrow.png",      20, 14, Qt::ArrowCursor);
    handCursor_      = loadCursor(":/cursors/cursor_hand.png",       27, 11, Qt::PointingHandCursor);
    sizeHorCursor_   = loadCursor(":/cursors/cursor_size_hor.png",  32, 31, Qt::SizeHorCursor);
    sizeVerCursor_   = loadCursor(":/cursors/cursor_size_ver.png",  32, 31, Qt::SizeVerCursor);
    sizeFDiagCursor_ = loadCursor(":/cursors/cursor_size_fdiag.png", 32, 31, Qt::SizeFDiagCursor);
    sizeBDiagCursor_ = loadCursor(":/cursors/cursor_size_bdiag.png", 32, 31, Qt::SizeBDiagCursor);
    moveCursor_      = loadCursor(":/cursors/cursor_move..png",      32, 31, Qt::SizeAllCursor);
    ibeamCursor_     = loadCursor(":/cursors/cursor_ibeam.png",      32, 31, Qt::IBeamCursor);
}

QCursor CursorManager::loadCursor(const QString &resourcePath,
                                  int hotspotX, int hotspotY,
                                  Qt::CursorShape fallback) {
    QPixmap pix(resourcePath);
    if (pix.isNull()) {
        qWarning() << "CursorManager: failed to load" << resourcePath
                   << "— falling back to Qt built-in cursor";
        return QCursor(fallback);
    }
    return QCursor(pix, hotspotX, hotspotY);
}

const QCursor &CursorManager::arrow()      const { return arrowCursor_; }
const QCursor &CursorManager::hand()       const { return handCursor_; }
const QCursor &CursorManager::sizeHor()    const { return sizeHorCursor_; }
const QCursor &CursorManager::sizeVer()    const { return sizeVerCursor_; }
const QCursor &CursorManager::sizeFDiag()  const { return sizeFDiagCursor_; }
const QCursor &CursorManager::sizeBDiag()  const { return sizeBDiagCursor_; }
const QCursor &CursorManager::move()       const { return moveCursor_; }
const QCursor &CursorManager::ibeam()      const { return ibeamCursor_; }
