# Embedded font guide

Place a CJK font file in this directory to embed it into the `chatroom` executable at build time.

Supported file types:
- `.ttf`
- `.ttc`
- `.otf`

Recommended embedded font priority:
1. `HuaweiSans-Regular.ttf`
2. `NotoSansCJKsc-Regular.otf`
3. `NotoSansSC-Regular.otf`
4. `NotoSansCJK-Regular.ttc`
5. `SourceHanSansCN-Regular.otf`

Build behavior:
- If one or more font files exist here, CMake auto-generates `embedded_fonts.qrc` and embeds them.
- Runtime scans all `.ttf/.ttc/.otf` files under `:/fonts`, then selects a preferred CJK family (Huawei Sans first).
- If this directory is empty, the app falls back to system fonts and logs a warning at startup.
