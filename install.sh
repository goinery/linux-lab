#!/usr/bin/env bash
set -Eeuo pipefail

readonly MIN_CMAKE_VERSION="3.16"
readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

readonly -a REQUIRED_PACKAGES=(
    build-essential
    cmake
    qtbase5-dev
    qtbase5-dev-tools
    qt5-qmake
    fontconfig
)
readonly -a FONT_PACKAGES=(
    fonts-noto-cjk
    fonts-wqy-microhei
)
readonly -a IBUS_PACKAGES=(
    ibus
    ibus-libpinyin
)
readonly -a FCITX5_PACKAGES=(
    fcitx5
    fcitx5-chinese-addons
    fcitx5-frontend-qt5
)

install_fonts=false
input_method="none"
run_apt_update=true
check_only=false

usage() {
    cat <<'EOF'
用法: bash install.sh [选项]

默认安装构建和运行 ChatRoom 所需的核心依赖。
脚本仅支持使用 apt-get 的 Debian/Ubuntu 系发行版。

选项:
  --check             仅检查当前环境，不更新软件源，不安装任何包
  --with-fonts        安装 Noto CJK 和文泉驿字体（可选系统回退字体）
  --with-ibus         安装 ibus 及中文拼音引擎
  --with-fcitx5       安装 fcitx5、中文扩展和 Qt5 前端
  --full              安装核心依赖、系统 CJK 字体和 ibus
  --no-update         跳过 apt-get update（仅在本地软件包索引可用时使用）
  -h, --help          显示本帮助

权限说明:
  - 推荐以普通用户运行 "bash install.sh"；脚本只会在 apt 操作时调用 sudo。
  - 普通用户需要 sudo 权限；root 用户可直接运行。
  - 安装需要可用的 Debian/Ubuntu 软件源和网络连接。
EOF
}

die() {
    printf '错误: %s\n' "$*" >&2
    exit 1
}

set_input_method() {
    local requested="$1"
    if [[ "$input_method" != "none" && "$input_method" != "$requested" ]]; then
        die "--with-ibus 和 --with-fcitx5 不能同时使用"
    fi
    input_method="$requested"
}

version_at_least() {
    local actual="$1"
    local required="$2"
    [[ "$(printf '%s\n' "$required" "$actual" | sort -V | sed -n '1p')" == "$required" ]]
}

verify_environment() {
    printf '\n=== 检查构建环境 ===\n'

    local -a missing_commands=()
    local command_name
    for command_name in cmake c++ make qmake fc-list; do
        if ! command -v "$command_name" >/dev/null 2>&1; then
            missing_commands+=("$command_name")
        fi
    done

    if ((${#missing_commands[@]} > 0)); then
        die "缺少命令: ${missing_commands[*]}"
    fi

    local cmake_version
    cmake_version="$(cmake --version | sed -n '1s/[^0-9]*\([0-9][0-9.]*\).*/\1/p')"
    [[ -n "$cmake_version" ]] || die "无法识别 CMake 版本"
    version_at_least "$cmake_version" "$MIN_CMAKE_VERSION" \
        || die "CMake $cmake_version 过旧，项目需要 >= $MIN_CMAKE_VERSION"

    local qt_version
    qt_version="$(qmake -query QT_VERSION 2>/dev/null || true)"
    [[ "$qt_version" == 5.* ]] || die "未检测到 Qt5 qmake（当前: ${qt_version:-未知}）"

    if ! printf '#if __cplusplus < 201703L\n#error C++17 required\n#endif\n' \
        | c++ -std=c++17 -x c++ -fsyntax-only - >/dev/null 2>&1; then
        die "当前 C++ 编译器不支持 C++17"
    fi

    local check_dir
    check_dir="$(mktemp -d "${TMPDIR:-/tmp}/chatroom-cmake-check.XXXXXX")"
    if ! cmake -S "$SCRIPT_DIR" -B "$check_dir" >/dev/null; then
        rm -rf -- "$check_dir"
        die "CMake 无法找到 Qt5 Core/Widgets/Network，请检查 Qt5 开发包"
    fi
    rm -rf -- "$check_dir"

    printf 'CMake: %s\n' "$cmake_version"
    printf 'Qt: %s\n' "$qt_version"
    printf 'C++: %s\n' "$(c++ --version | sed -n '1p')"
    printf 'Qt5 Core/Widgets/Network: 可用\n'

    if find "$SCRIPT_DIR/resources/fonts" -maxdepth 1 -type f \
        \( -name '*.ttf' -o -name '*.ttc' -o -name '*.otf' \) -print -quit \
        | grep -q .; then
        printf '内嵌字体资源: 已找到\n'
    else
        printf '内嵌字体资源: 未找到（建议使用 --with-fonts）\n'
    fi

    if [[ "$install_fonts" == true ]]; then
        local system_cjk_font
        system_cjk_font="$(fc-list :lang=zh family | sed -n '1p')"
        [[ -n "$system_cjk_font" ]] || die "未检测到系统 CJK 字体"
        printf '系统 CJK 字体: %s\n' "$system_cjk_font"
    fi

    if [[ "$input_method" == "ibus" ]]; then
        command -v ibus >/dev/null 2>&1 || die "ibus 未正确安装"
    elif [[ "$input_method" == "fcitx5" ]]; then
        command -v fcitx5 >/dev/null 2>&1 || die "fcitx5 未正确安装"
    fi
}

while (($# > 0)); do
    case "$1" in
    --check)
        check_only=true
        ;;
    --with-fonts)
        install_fonts=true
        ;;
    --with-ibus)
        set_input_method "ibus"
        ;;
    --with-fcitx5)
        set_input_method "fcitx5"
        ;;
    --full)
        install_fonts=true
        set_input_method "ibus"
        ;;
    --no-update)
        run_apt_update=false
        ;;
    -h | --help)
        usage
        exit 0
        ;;
    *)
        usage >&2
        die "未知选项: $1"
        ;;
    esac
    shift
done

if [[ "$check_only" == true ]]; then
    verify_environment
    exit 0
fi

command -v apt-get >/dev/null 2>&1 \
    || die "未找到 apt-get；请根据 README 手动安装对应发行版的依赖"

if [[ -r /etc/os-release ]]; then
    # shellcheck disable=SC1091
    source /etc/os-release
    printf '系统: %s\n' "${PRETTY_NAME:-${ID:-未知}}"
fi

declare -a apt_command
if ((EUID == 0)); then
    apt_command=(apt-get)
else
    command -v sudo >/dev/null 2>&1 \
        || die "当前用户不是 root，且系统未安装 sudo"
    printf '正在验证 sudo 权限（可能需要输入当前用户密码）...\n'
    sudo -v
    apt_command=(sudo apt-get)
fi

declare -a packages=("${REQUIRED_PACKAGES[@]}")
if [[ "$install_fonts" == true ]]; then
    packages+=("${FONT_PACKAGES[@]}")
fi
if [[ "$input_method" == "ibus" ]]; then
    packages+=("${IBUS_PACKAGES[@]}")
elif [[ "$input_method" == "fcitx5" ]]; then
    packages+=("${FCITX5_PACKAGES[@]}")
fi

printf '\n=== 将安装的软件包 ===\n'
printf '  %s\n' "${packages[@]}"

if [[ "$run_apt_update" == true ]]; then
    printf '\n=== 更新 apt 软件包索引 ===\n'
    "${apt_command[@]}" update
fi

printf '\n=== 安装项目依赖 ===\n'
"${apt_command[@]}" install -y --no-install-recommends "${packages[@]}"

verify_environment

printf '\n=== 依赖安装完成 ===\n'
printf '下一步:\n'
printf '  cmake -S . -B build\n'
printf '  cmake --build build --parallel\n'
if [[ "$input_method" != "none" ]]; then
    printf '\n输入法已安装，但脚本不会修改用户会话配置。\n'
    printf '请在桌面设置中选择对应输入法，并重新登录图形会话。\n'
fi
