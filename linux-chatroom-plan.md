# Linux环境程序与设计课设 — 现代化图形界面多线程聊天室

## 一、项目概述

在 WSL Ubuntu 22.04 环境下，从零开始设计并实现一个基于 C/S 架构的现代化图形界面多线程聊天室程序。项目使用 CMake 构建，最终可打包为单个可执行文件。

## 二、技术选型

| 模块 | 技术方案 | 理由 |
|------|---------|------|
| GUI 框架 | **Qt5** (qtbase5-dev) | 原生支持 Ubuntu 22.04 仓库，丰富的控件体系，内置网络/线程模块，QSS 样式表可实现现代化 UI |
| 网络通信 | **QTcpSocket / QTcpServer** | 与 Qt 事件循环无缝集成，跨平台，信号槽机制天然异步 |
| 多线程 | **QThread + 线程池** | 服务端使用线程池处理多客户端，客户端网络层独立线程避免阻塞 UI |
| 通信协议 | **JSON 格式自定义协议** | 可读性好，Qt 自带 QJson 解析，便于调试和扩展 |
| 构建系统 | **CMake 3.22+** | 已安装，支持 CPack 打包 |
| 打包方案 | **CPack DEB + 单可执行文件** | 通过静态链接或资源嵌入实现单文件分发 |

## 三、系统架构

```
┌─────────────────────────────────────────────────┐
│                  Chat Server                     │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ QTcpServer│  │ ThreadPool│  │  UserManager  │  │
│  │ (监听端口) │  │ (线程池)  │  │  (用户管理)   │  │
│  └─────┬─────┘  └─────┬────┘  └───────┬───────┘  │
│        │              │                │          │
│  ┌─────▼──────────────▼────────────────▼───────┐  │
│  │           MessageRouter (消息路由)            │  │
│  │    私聊转发 / 群聊广播 / 系统通知              │  │
│  └─────────────────────────────────────────────┘  │
└───────────────────────┬─────────────────────────┘
                        │ TCP (JSON Protocol)
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
   ┌────────────┐ ┌────────────┐ ┌────────────┐
   │  Client A  │ │  Client B  │ │  Client C  │
   │ ┌────────┐ │ │ ┌────────┐ │ │ ┌────────┐ │
   │ │GUI线程 │ │ │ │GUI线程 │ │ │ │GUI线程 │ │
   │ │(主线程)│ │ │ │(主线程)│ │ │ │(主线程)│ │
   │ └───┬────┘ │ │ └───┬────┘ │ │ └───┬────┘ │
   │ ┌───▼────┐ │ │ ┌───▼────┐ │ │ ┌───▼────┐ │
   │ │网络线程│ │ │ │网络线程│ │ │ │网络线程│ │
   │ └────────┘ │ │ └────────┘ │ │ └────────┘ │
   └────────────┘ └────────────┘ └────────────┘
```

## 四、通信协议设计

所有消息使用 JSON 格式，通过 TCP 长连接传输，每条消息前4字节为消息体长度（大端序）。

### 消息类型

```json
// 注册请求
{"type": "register", "username": "alice", "password": "123456"}

// 登录请求
{"type": "login", "username": "alice", "password": "123456"}

// 聊天消息
{"type": "chat", "from": "alice", "to": "bob", "content": "Hello!", "timestamp": 1713600000}

// 群聊消息
{"type": "group_chat", "from": "alice", "room": "general", "content": "Hi all!", "timestamp": 1713600000}

// 系统通知
{"type": "system", "content": "alice 加入了聊天室", "timestamp": 1713600000}

// 在线用户列表
{"type": "user_list", "users": ["alice", "bob", "charlie"]}

// 心跳包
{"type": "heartbeat", "timestamp": 1713600000}
```

## 五、功能清单

### 核心功能
1. **用户注册/登录** — 账号密码认证，本地文件持久化
2. **私聊** — 一对一实时消息
3. **群聊** — 公共聊天室，所有人可见
4. **在线用户列表** — 实时显示在线用户
5. **消息时间戳** — 每条消息附带时间显示
6. **系统通知** — 用户上线/下线提醒

### 增强功能
7. **聊天记录** — 本地保存聊天历史
8. **消息提示音** — 新消息到达提示
9. **窗口托盘** — 最小化到系统托盘
10. **用户头像** — 基于用户名生成随机头像

## 六、项目目录结构

```
linux-lab/
├── CMakeLists.txt                  # 顶层 CMake 配置
├── src/
│   ├── CMakeLists.txt              # 源码 CMake
│   ├── main.cpp                    # 程序入口（含 server/client 模式切换）
│   ├── common/
│   │   ├── protocol.h              # 协议定义（消息类型枚举、JSON结构）
│   │   └── constants.h             # 常量定义（端口、缓冲区大小等）
│   ├── server/
│   │   ├── chat_server.h           # 聊天服务器类声明
│   │   ├── chat_server.cpp         # 聊天服务器实现
│   │   ├── client_handler.h        # 客户端连接处理类声明
│   │   ├── client_handler.cpp      # 客户端连接处理实现
│   │   ├── thread_pool.h           # 线程池实现（头文件 only）
│   │   ├── user_manager.h          # 用户管理类声明
│   │   └── user_manager.cpp        # 用户管理实现（注册/登录/持久化）
│   └── client/
│       ├── chat_client.h           # 聊天客户端网络层声明
│       ├── chat_client.cpp         # 聊天客户端网络层实现
│       ├── mainwindow.h            # 主窗口声明
│       ├── mainwindow.cpp          # 主窗口实现
│       ├── login_dialog.h          # 登录/注册对话框声明
│       ├── login_dialog.cpp        # 登录/注册对话框实现
│       ├── message_widget.h        # 消息气泡组件声明
│       ├── message_widget.cpp      # 消息气泡组件实现
│       └── resources.qrc           # Qt 资源文件（图标、QSS样式）
├── resources/
│   ├── style.qss                   # 现代化 QSS 样式表
│   └── icons/                      # 图标资源
├── install.sh                      # 一键安装依赖脚本
└── README.md                       # 项目说明
```

## 七、UI 设计

### 登录界面
- 居中对话框，应用图标 + 标题
- 用户名/密码输入框，登录/注册按钮切换
- 现代化圆角卡片式设计

### 主界面布局
```
┌──────────────────────────────────────────────┐
│  聊天室 v1.0                        ─ □ ✕   │
├──────────┬───────────────────────────────────┤
│          │  群聊 / 与 Bob 的对话              │
│ 在线用户  │───────────────────────────────────│
│          │                                   │
│ ● Alice  │  ┌─────────────────────┐          │
│ ● Bob    │  │ Bob: Hello!         │  10:30   │
│ ● Charlie│  └─────────────────────┘          │
│          │         ┌─────────────────────┐    │
│          │         │ Hi! I'm Alice       │    │
│          │         └─────────────────────┘    │
│          │                                   │
│          │───────────────────────────────────│
│          │  [输入消息...              ] [发送] │
└──────────┴───────────────────────────────────┘
```

### QSS 样式要点
- 深色/浅色主题切换
- 消息气泡：自己的消息右对齐蓝色，他人消息左对齐灰色
- 圆角按钮和输入框
- 平滑过渡动画

## 八、实现步骤（共 12 步）

### 第1步：环境搭建
- 编写 `install.sh` 安装 Qt5 开发依赖（qtbase5-dev, qtchooser, qt5-qmake 等）
- 创建顶层 `CMakeLists.txt`，配置 Qt5 和 C++17 标准
- 验证构建环境可用

### 第2步：协议与公共模块
- 实现 `common/protocol.h`：定义消息类型枚举、JSON 序列化/反序列化辅助函数
- 实现 `common/constants.h`：定义默认端口、缓冲区大小等常量
- 编写消息帧封装（4字节长度头 + JSON body）

### 第3步：线程池实现
- 实现 `server/thread_pool.h`：基于 `std::thread` + `std::mutex` + `std::condition_variable` 的线程池
- 支持提交任务、优雅关闭

### 第4步：用户管理模块
- 实现 `server/user_manager.h/cpp`：用户注册、登录验证
- 本地 JSON 文件持久化用户数据（users.json）
- 线程安全的用户操作接口

### 第5步：服务端核心 — 客户端连接处理
- 实现 `server/client_handler.h/cpp`：封装单个客户端的 socket 读写
- 处理消息帧的拆包/粘包
- 解析 JSON 消息并路由到对应处理器

### 第6步：服务端核心 — 聊天服务器
- 实现 `server/chat_server.h/cpp`：基于 QTcpServer 的服务器
- 新连接到来时分配 ClientHandler 并提交到线程池
- 消息路由：私聊转发、群聊广播、系统通知
- 在线用户管理：维护连接映射表

### 第7步：客户端网络层
- 实现 `client/chat_client.h/cpp`：基于 QTcpSocket 的客户端
- 独立 QThread 运行网络事件循环
- 信号槽机制与 UI 线程通信
- 自动重连机制

### 第8步：登录/注册界面
- 实现 `client/login_dialog.h/cpp`：QDialog 登录对话框
- 用户名/密码输入，登录/注册模式切换
- 输入校验与错误提示
- 连接服务器并发送认证请求

### 第9步：主界面框架
- 实现 `client/mainwindow.h/cpp`：QMainWindow 主窗口
- 左侧用户列表（QListWidget）+ 右侧聊天区域（QStackedWidget）
- 消息输入框 + 发送按钮
- 与 ChatClient 信号槽连接

### 第10步：消息气泡组件与聊天逻辑
- 实现 `client/message_widget.h/cpp`：自定义消息气泡 QWidget
- 区分自己/他人消息的样式和布局
- 聊天消息的发送和接收显示
- 自动滚动到最新消息

### 第11步：QSS 样式与资源
- 编写 `resources/style.qss`：现代化样式表
- 配色方案：主色调 #2196F3（蓝色系），背景 #F5F5F5
- 圆角、阴影、过渡效果
- Qt 资源文件 `resources.qrc` 嵌入样式和图标

### 第12步：程序入口与打包
- 实现 `main.cpp`：命令行参数解析，`--server` 启动服务端模式，默认启动客户端
- CPack 配置：打包为 DEB 安装包
- 编写启动脚本，可同时启动 server + client 进行测试
- 完整构建测试与验证

## 九、依赖安装

```bash
sudo apt-get update
sudo apt-get install -y \
    qtbase5-dev \
    qtchooser \
    qt5-qmake \
    qtbase5-dev-tools \
    cmake \
    build-essential
```

## 十、构建与运行

```bash
# 构建
mkdir build && cd build
cmake ..
make -j$(nproc)

# 启动服务端
./chatroom --server --port 8888

# 启动客户端（另一个终端）
./chatroom --host 127.0.0.1 --port 8888
```

## 十一、课设报告覆盖的知识点

| Linux/系统知识点 | 本项目中的体现 |
|-----------------|--------------|
| 多线程编程 | 线程池、QThread、mutex/condition_variable |
| 网络编程 | TCP Socket、自定义应用层协议 |
| 进程间通信 | 客户端-服务端 C/S 架构 |
| 文件 I/O | 用户数据持久化、聊天记录保存 |
| 信号机制 | Qt 信号槽、Unix 信号处理 |
| GUI 编程 | Qt5 控件、事件循环、自定义绘制 |
| CMake 构建 | 跨平台构建系统、CPack 打包 |
