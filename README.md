# ChatRoom

基于 Qt5（C++17）的多线程局域网聊天室。单个二进制 `chatroom` 既可作服务端，也可作客户端，甚至可在同一窗口内完成「启动配置 → 服务端监控 / 客户端登录 → 聊天」的完整流程。

![platform](https://img.shields.io/badge/platform-Linux-blue)
![qt](https://img.shields.io/badge/Qt-5-green)
![lang](https://img.shields.io/badge/C%2B%2B-17-orange)
![version](https://img.shields.io/badge/version-1.0.0-brightgreen)

---

## 目录

- [功能特性](#功能特性)
- [快速开始](#快速开始)
- [命令行参数](#命令行参数)
- [使用指南](#使用指南)
- [架构设计](#架构设计)
- [通信协议](#通信协议)
- [主题与字体](#主题与字体)
- [打包发布](#打包发布)
- [测试](#测试)
- [常见问题](#常见问题)
- [项目结构](#项目结构)

---

## 功能特性

### 服务端

- **多线程处理**：基于 `std::thread` 的线程池（线程数 = `std::thread::hardware_concurrency()`），每个客户端连接由独立 `ClientHandler` 包装。
- **用户管理**：JSON 文件持久化（`users.json`），密码以 SHA-256 哈希存储，支持注册 / 登录鉴权。
- **消息路由**：私聊（`chat`）定向投递，群聊（`group_chat`）广播至全员，系统消息（`system`）广播。
- **管理能力**：禁言 / 解禁指定用户、向全员广播、实时在线用户列表、连接数 / 消息数 / 运行时长统计、操作日志。
- **状态监控页**：展示本机 IP 与端口、在线用户、统计信息、广播输入框与日志列表。

### 客户端

- **连接管理**：自动重连（间隔 3s，最多 5 次）、心跳保活（30s）。
- **聊天界面**：左侧用户列表 + 右侧多会话分页（`QStackedWidget`），私聊与群聊各占一页。
- **消息气泡**：自绘 `MessageWidget`，区分自己 / 他人左右气泡，支持长文本自动换行与宽度自适应。
- **未读提醒**：会话切换前累计未读消息数，以徽章显示在用户列表项上。
- **快捷操作**：全屏切换、菜单栏（文件 / 视图 / 帮助）、状态栏。

### 通用

- **单窗口流程**：`QMainWindow` + `QStackedWidget`，启动页 → 服务端监控 / 客户端登录 → 聊天页，全程无新弹窗。
- **自绘标题栏**：自定义最小化 / 最大化 / 关闭按钮，支持拖拽移动与边缘缩放（8 向）。
- **四套主题**：素白、森氧、云海、夜航，一键切换。
- **内嵌中文字体**：构建时自动把 `resources/fonts/` 下的字体打包进二进制，运行时优先使用内嵌字体，无需依赖系统字体即可正常显示中文。
- **输入法自适应**：自动检测并配置 `QT_IM_MODULE` / `XMODIFIERS`，兼容 ibus 与 fcitx。
- **自定义鼠标光标**：从 PNG 资源加载，回退到 Qt 内置光标。

---

## 快速开始

### 前置需求

| 类别 | 要求 | 说明 |
| --- | --- | --- |
| 操作系统 | Linux | 构建可在无界面环境完成；运行 GUI 需要 X11、Wayland 或 WSLg/X Server |
| 构建工具 | CMake ≥ 3.16、Make | `install.sh` 默认使用 Debian/Ubuntu 的 `build-essential` 和 `cmake` |
| 编译器 | 支持 C++17 的 g++ / clang++ | 当前主要验证 g++ |
| Qt 开发包 | Qt5 Core / Widgets / Network | Debian/Ubuntu 对应 `qtbase5-dev` 和 `qtbase5-dev-tools` |
| 运行时 | fontconfig | 用于系统字体查找；项目已内嵌中文字体 |
| 可选增强 | 系统 CJK 字体、ibus 或 fcitx5 | 仅在需要系统字体回退或中文输入时安装 |

`install.sh` 仅支持具有 `apt-get` 的 Debian/Ubuntu 系发行版。Fedora、Arch Linux 等系统需手动安装上表对应组件。

> **WSL/WSLg 兼容性提示**：Windows 与原生 Ubuntu 在窗口装饰、菜单栏尺寸、显示缩放和窗口状态管理方面存在差异。本项目使用自定义无边框窗口和全屏切换，在 WSL/WSLg 中可能出现全屏尺寸、位置或菜单栏布局异常。WSL 适合构建与基础联调；如需完整、稳定的图形界面和全屏体验，建议在原生 Ubuntu 桌面环境中运行。

### 安装依赖

```bash
# 仅安装必需的构建/运行依赖
bash install.sh

# 查看可用选项
bash install.sh --help

# 安装系统 CJK 回退字体
bash install.sh --with-fonts

# 二选一：安装中文输入法
bash install.sh --with-ibus
bash install.sh --with-fcitx5

# 核心依赖 + 系统字体 + ibus
bash install.sh --full

# 仅检查环境，不访问网络、不需要 root/sudo
bash install.sh --check
```

默认核心包为 `build-essential`、`cmake`、`qtbase5-dev`、`qtbase5-dev-tools`、`qt5-qmake` 和 `fontconfig`。脚本不再强制安装某一种输入法，避免覆盖或干扰用户现有的 ibus/fcitx 配置。

### 权限与运行注意事项

- 推荐以普通用户执行 `bash install.sh`，不要直接使用 `sudo bash install.sh`。脚本只在 `apt-get update/install` 时调用 `sudo`，并会先执行 `sudo -v` 验证权限。
- 安装依赖需要 sudo/root 权限、可用的软件源与网络连接。`--check` 模式不修改系统，也不需要 sudo。
- 依赖安装完成后，构建和运行程序都应使用普通用户。默认端口 `8888` 不需要特权；不建议使用小于 `1024` 的端口。
- 服务端会在当前工作目录读写 `users.json`，因此请从普通用户可写目录启动，不要在 `/usr/bin` 等系统目录中运行服务端。
- 局域网客户端需连接服务端的 TCP 端口；如系统启用防火墙，需由管理员放行实际使用的端口。脚本不会自动修改防火墙。
- 安装输入法包后，仍需在桌面环境中选择 ibus/fcitx5 并重新登录图形会话；脚本不会修改用户的会话环境变量。

### 构建与运行

```bash
# 配置
cmake -S . -B build

# 编译（输出到 build/bin/chatroom）
cmake --build build -j$(nproc)

# 运行
./build/bin/chatroom
```

或在 VS Code 中使用预设的 `wsl-debug` CMake Preset（Debug 构建）。

### 一键联调

打开两个终端：

```bash
# 终端 1：启动服务端
./build/bin/chatroom -s -p 8890
# 启动页会预选「作为服务端运行」，端口预填 8890，点「下一步」进入监控页

# 终端 2：启动客户端
./build/bin/chatroom -p 8890
# 启动页选择「作为客户端运行」，填写地址与端口，点「下一步」会先检测连接
```

---

## 命令行参数

```
chatroom [-H host] [-p port] [-s] [--help] [--version]

  -H, --host <host>      服务端地址（默认 127.0.0.1），预填到启动页
  -p, --port <port>      服务端端口（默认 8888），预填到启动页
  -s, --server           启动时预选「作为服务端运行」
      --help             显示帮助
      --version          显示版本号
```

> `-H` / `-p` / `-s` 仅作为启动页的默认值，最终角色与连接参数仍以页面上的选择为准。

---

## 使用指南

### 服务端流程

1. 启动后在启动页选择「作为服务端运行」，填入端口，点击「下一步」。
2. 进入服务端监控页：可看到本机 IP、端口、在线用户、连接 / 消息统计与运行时长。
3. 在广播输入框输入内容可向全员发送系统消息。
4. 选中在线用户后点击「禁言 / 解禁」可控制其发言权限。

### 客户端流程

1. 启动后在启动页选择「作为客户端运行」，填入服务端地址与端口，点击「下一步」。
2. 连接成功后进入登录 / 注册页：
   - **登录**：输入用户名 + 密码。
   - **注册**：输入用户名 + 密码 + 确认密码；注册成功后提示并自动切回登录。
3. 登录成功后同一窗口切换进入聊天页（无新弹窗）。
4. 左侧选择用户发起私聊，或选择群聊会话发送群消息；他人发来的消息会在列表项上显示未读徽章。

### 主题切换

聊天页右上角的主题按钮可在「素白 / 森氧 / 云海 / 夜航」之间循环切换。

---

## 架构设计

```
┌──────────────────────────────────────────────────────────────┐
│                       UnifiedFlowWindow                       │
│                   (QMainWindow + QStackedWidget)              │
│  ┌──────────┐   ┌──────────────┐   ┌──────────┐   ┌────────┐ │
│  │ 启动页    │ → │ 服务端监控页  │   │ 客户端    │ → │ 聊天页  │ │
│  │ 角色选择  │   │ (ChatServer) │   │ 登录页    │   │(MainWin)│ │
│  └──────────┘   └──────────────┘   └──────────┘   └────────┘ │
└──────────────────────────────────────────────────────────────┘
        │                                   │
        ▼                                   ▼
┌─────────────────┐               ┌──────────────────┐
│   ChatServer    │   TCP / JSON  │    ChatClient    │
│  QTcpServer     │ ◄──────────► │   QTcpSocket      │
│  ThreadPool     │   4B 长度前缀  │  Heartbeat 30s   │
│  ClientHandler  │   + 紧凑 JSON  │  Auto-Reconnect  │
│  UserManager    │               │                   │
└─────────────────┘               └──────────────────┘
        │
        ▼
┌─────────────────┐
│   UserManager   │
│  users.json     │
│  SHA-256 哈希    │
└─────────────────┘
```

### 服务端

- `ChatServer`：持有 `QTcpServer`，接收新连接并为每个连接创建 `ClientHandler`；维护在线用户映射、禁言集合、统计数据；通过信号把状态推送给 UI。
- `ClientHandler`：每个 TCP 连接的包装器，负责按协议分帧、解析 JSON，向上发出 `messageReceived` / `disconnected` 信号。
- `ThreadPool`：`std::thread` 实现的工作线程池，连接派发的消息处理任务入队后由工作线程执行，避免阻塞 I/O。
- `UserManager`：单例，`users.json` 读写 + SHA-256 校验，`QMutex` 保证线程安全。

### 客户端

- `ChatClient`：`QTcpSocket` 封装，负责连接 / 断开 / 收发、心跳定时器、断线自动重连。
- `MainWindow`：聊天主界面，用户列表 + 会话分页 + 消息气泡 + 未读徽章 + 菜单栏 / 状态栏 / 快捷键。
- `MessageWidget`：单条消息气泡，`QFrame` + `QTextEdit` 容器，按会话宽度自适应。

### UI 基础设施

- `unified_flow_window_builder.cpp`：拆分出的 UI 构建逻辑，保持主窗口类可读。
- `theme_manager`：主题调色板 + 模板渲染，详见 [主题与字体](#主题与字体)。
- `cursor_manager`：自定义鼠标光标加载。
- `app_setup`：字体嵌入加载、输入法环境配置、`QApplication` 启动与命令行解析。

---

## 通信协议

**二进制分帧**：4 字节大端 `uint32` 长度前缀 + 紧凑 JSON 正文。

```
┌──────────────┬──────────────────────────────┐
│ length (4B)  │       JSON body (length B)    │
│  big-endian  │   UTF-8, compact (no spaces)  │
└──────────────┴──────────────────────────────┘
```

### 消息类型

| type 字符串        | 方向       | 说明                                   |
| ------------------ | ---------- | -------------------------------------- |
| `register`         | C → S      | 注册请求                               |
| `register_response`| S → C      | 注册结果（`success` / `message`）       |
| `login`            | C → S      | 登录请求                               |
| `login_response`   | S → C      | 登录结果（`success` / `message`）       |
| `chat`             | C → S → C  | 私聊消息（`from` / `to` / `content`）   |
| `group_chat`       | C → S → C  | 群聊消息（`room: "general"`）           |
| `system`           | S → C      | 系统消息（可选 `target_chat` 定向）     |
| `user_list_request`| C → S      | 请求在线用户列表                       |
| `user_list`        | S → C      | 在线用户列表（`users` 数组）            |
| `heartbeat`        | C → S      | 心跳保活                               |
| `logout`           | C → S      | 登出                                   |

协议实现见 [`src/common/protocol.h`](src/common/protocol.h)（`serializeMessage` / `deserializeMessage` 与各 `create*` 工厂函数）。

---

## 主题与字体

### 主题

共 4 套主题，按索引循环切换：

| 索引 | id     | 名称 | 风格       |
| ---- | ------ | ---- | ---------- |
| 0    | gray   | 素白 | 浅色 / 灰蓝 |
| 1    | mint   | 森氧 | 浅色 / 绿意 |
| 2    | sky    | 云海 | 浅色 / 海蓝 |
| 3    | night  | 夜航 | 深色       |

**设计**：只有一份模板 [`resources/themes/template.qss`](resources/themes/template.qss) 描述结构与选择器，每个主题通过一组颜色（`ThemePalette`）参数化渲染，避免每套主题重复书写整套 QSS。

- 修改某主题配色 → 编辑 [`src/theme_manager.cpp`](src/theme_manager.cpp) 中对应 `*Palette()` 工厂的字段值。
- 新增主题 → 复制一个工厂函数，调整配色，加入 `palettes()` 列表。
- 修改样式结构 → 编辑 `template.qss`。

### 字体

- 构建时由 CMake 扫描 `resources/fonts/*.ttf|*.ttc|*.otf`，生成 `embedded_fonts.qrc` 打包进二进制。
- 运行时优先加载内嵌字体，找不到时回退到系统 CJK 字体。
- 字体优选顺序：LXGW WenKai → Huawei Sans → Noto Sans CJK SC → Noto Sans SC → WenQuanYi Micro Hei → …（见 [`src/app_setup.cpp`](src/app_setup.cpp)）。

> 当前内嵌字体为 `HuaweiSans-Regular.ttf`，因此即便目标机器无任何中文字体，UI 也能正常显示中文。

---

## 打包发布

使用 CPack 生成 `.deb` 包：

```bash
cd build
cpack -G DEB
ls -lh *.deb        # 产出 chatroom-1.0.0-Linux.deb
```

包元数据（定义于 [`CMakeLists.txt`](CMakeLists.txt)）：

- **Depends**：`libqt5core5a, libqt5widgets5, libqt5network5, fontconfig`
- **Recommends**：`fonts-noto-cjk, fonts-wqy-microhei, fcitx5-frontend-qt5 | fcitx-frontend-qt5 | ibus`

安装后可执行文件位于 `/usr/bin/chatroom`。

---

## 测试

常用冒烟测试：

```bash
# 1. 无界面字体检查（确认命中内嵌字体）
QT_QPA_PLATFORM=offscreen ./build/bin/chatroom --help 2>&1 | grep "embedded font"

# 2. 注册 / 登录协议回归（需先启动服务端，默认端口 8888）
python3 - <<'PY'
import socket, json, struct, time

def send(msg, timeout=3):
    s = socket.create_connection(('127.0.0.1', 8888), timeout=timeout)
    body = json.dumps(msg, separators=(',', ':')).encode()
    s.sendall(struct.pack('>I', len(body)) + body)
    s.settimeout(timeout)
    ln = struct.unpack('>I', s.recv(4))[0]
    data = b''
    while len(data) < ln:
        data += s.recv(ln - len(data))
    s.close()
    return data.decode('utf-8', errors='replace')

u = 'u' + str(int(time.time()))[-6:]
print('REGISTER ', send({'type': 'register', 'username': u, 'password': 'pass1234'}))
print('LOGIN_OK ', send({'type': 'login', 'username': u, 'password': 'pass1234'}))
print('LOGIN_BAD', send({'type': 'login', 'username': u, 'password': 'badpass'}))
PY
```

预期：`REGISTER` / `LOGIN_OK` 返回 `success: true`，`LOGIN_BAD` 返回 `success: false`。

---

## 常见问题

**Q：界面显示方框（tofu）？**
确认 `resources/fonts/HuaweiSans-Regular.ttf` 存在并重新 `cmake + build`；或在目标机器执行 `bash install.sh --with-fonts` 安装系统 CJK 字体作为兜底。可用 `QT_QPA_PLATFORM=offscreen ./build/bin/chatroom --help 2>&1 | grep "embedded font"` 检查是否命中内嵌字体。

**Q：能运行但字体不一致？**
程序会优先使用内嵌字体。如需统一为系统字体，安装对应字体包即可。

**Q：注册成功但登录失败？**
确认运行的是最新构建产物 `./build/bin/chatroom`，并检查服务端日志是否出现 `QSocketNotifier: Invalid socket` 等异常。

**Q：中文输入法无法使用？**
先根据桌面环境选择一种输入法：`bash install.sh --with-ibus` 或 `bash install.sh --with-fcitx5`。安装后在系统设置中启用对应输入法并重新登录图形会话，再检查 `QT_IM_MODULE` / `XMODIFIERS`。不要同时强制设置 ibus 和 fcitx5。

**Q：换机器后显示异常？**
优先携带 `resources/fonts/` 字体重建；或执行 `bash install.sh --with-fonts` 安装系统中文字体兜底。

---

## 项目结构

```
.
├── CMakeLists.txt              # 顶层构建配置 + CPack 打包
├── CMakePresets.json            # WSL Debug 预设
├── install.sh                   # 依赖安装脚本
├── users.json                   # 用户数据（运行时生成，Git 忽略）
├── resources/
│   ├── style.qss                # 基础样式
│   ├── themes/template.qss      # 主题模板（结构 + 选择器）
│   ├── fonts/                   # 内嵌字体（构建时自动打包）
│   └── cursors/                 # 自定义光标 PNG
└── src/
    ├── main.cpp                 # 入口，委托给 runApp()
    ├── app_setup.cpp/h          # 字体嵌入、IME 环境、QApplication 启动、命令行解析
    ├── unified_flow_window.cpp/h        # 单窗口主框架（QStackedWidget）
    ├── unified_flow_window_builder.cpp  # UI 构建逻辑（拆分自主窗口）
    ├── theme_manager.cpp/h      # 主题调色板 + 模板渲染
    ├── cursor_manager.cpp/h     # 自定义光标
    ├── common/
    │   ├── constants.h          # 默认端口、缓冲区、超时等常量
    │   └── protocol.h           # 线协议：4B 长度前缀 + JSON
    ├── server/
    │   ├── chat_server.cpp/h    # QTcpServer，消息路由
    │   ├── client_handler.cpp/h # 每连接包装器
    │   ├── thread_pool.h        # std::thread 线程池
    │   └── user_manager.cpp/h   # JSON 文件用户存储（SHA-256）
    └── client/
        ├── chat_client.cpp/h    # QTcpSocket 封装、心跳、自动重连
        ├── mainwindow.cpp/h     # 聊天 UI：用户列表、会话分页、气泡
        ├── mainwindow_userlist.cpp # 用户列表逻辑
        ├── message_widget.cpp/h # 消息气泡组件
        ├── login_dialog.cpp/h   # （遗留，已被统一流程窗口取代）
        └── resources.qrc        # QSS / 主题模板 / 光标资源
```

---

## 许可

本项目暂未声明开源许可证，默认版权所有。如需使用，请联系作者。
