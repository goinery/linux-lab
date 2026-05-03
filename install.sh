#!/bin/bash
set -e

echo "=== 安装聊天室项目依赖 ==="

sudo apt-get update
sudo apt-get install -y \
    qtbase5-dev \
    qtchooser \
    qt5-qmake \
    qtbase5-dev-tools \
    ibus \
    ibus-libpinyin \
    ibus-gtk \
    ibus-gtk3 \
    fontconfig \
    fonts-noto-cjk \
    fonts-wqy-microhei \
    cmake \
    build-essential

echo "=== 依赖安装完成 ==="
echo "Qt5 版本: $(qmake --version 2>&1)"
echo "CMake 版本: $(cmake --version | head -1)"
echo "g++ 版本: $(g++ --version | head -1)"
echo "中文字体检查:"
fc-list :lang=zh family | head -5 || true
echo "输入法环境变量检查:"
env | grep -E '^(QT_IM_MODULE|GTK_IM_MODULE|XMODIFIERS)=' || true
