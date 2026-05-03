# ChatRoom 完整测试说明

本文档用于验证 Linux 环境下 ChatRoom 的字体修复、功能可用性与打包结果。

## 1. 环境准备

在项目根目录执行：

sudo apt-get update
sudo apt-get install -y qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools fontconfig fonts-noto-cjk fonts-wqy-microhei cmake build-essential

或直接执行：

bash install.sh

预期：
- 依赖安装完成无报错。
- 末尾能看到中文字体列表输出（fc-list :lang=zh）。

输入法环境建议（Ubuntu/WSL）：
- 安装后重新登录图形会话。
- 若使用 ibus，可执行：
	- `ibus-daemon -drx`
- 检查环境变量至少包含：
	- `QT_IM_MODULE=ibus`
	- `XMODIFIERS=@im=ibus`

## 2. 字体资源确认

检查字体文件：

ls -lh resources/fonts/HuaweiSans-Regular.ttf

预期：
- 文件存在。
- 大小约 85KB（可能略有差异）。

检查字体 family：

fc-scan resources/fonts/HuaweiSans-Regular.ttf | grep -Ei "family|fullname|style"

预期：
- family 显示为 Huawei Sans。

## 3. 全量构建

建议清理后构建：

rm -rf build
cmake -S . -B build
cmake --build build -j$(nproc)

预期：
- CMake 配置阶段打印 Embedding chatroom fonts: ...HuaweiSans-Regular.ttf。
- 编译到 100%，无错误。

## 4. 字体加载链路验证（无界面模式）

执行：

QT_QPA_PLATFORM=offscreen ./build/bin/chatroom --help

预期日志包含：
- Loaded embedded font: :/fonts/HuaweiSans-Regular.ttf
- Selected embedded font family: Huawei Sans
- UI font configured from embedded-resource family: Huawei Sans

说明：
- 出现上述日志表示程序优先使用了内嵌字体，不再依赖系统字体才能显示中文。

## 5. 服务端冒烟测试（启动页流程）

执行：

./build/bin/chatroom

在启动页操作：
- 选择“作为服务端运行”。
- 端口填 8890，点击“下一步”。

预期：
- 成功进入“服务端状态”窗口。
- 页面显示本机地址与端口 8890。

## 6. 客户端/服务端联调（图形界面）

先在一个终端启动服务端（见第5节）。

再在另一个终端启动客户端：

./build/bin/chatroom

在启动页操作：
- 选择“作为客户端运行”。
- 输入服务端地址和端口。
- 点击“下一步”时会立即检查连接：
	- 若连接失败，会在页面状态区提示并停留在启动页。
	- 若连接成功，进入登录/注册页。

手工验收点：
- 登录界面中文标题、按钮、提示不再是方框。
- 登录成功后在同一个窗口内切换进入聊天页（无新弹窗）。
- 在聊天输入框中可切换中文输入法并正常输入中文。
- 新用户注册后提示“注册成功，请登录”，并能自动切回登录页。
- 使用刚注册的账号登录成功并进入主界面。
- 使用错误密码登录时提示“用户名或密码错误”。
- 主窗口菜单“文件/视图/帮助”中文正常。
- 在线用户区、聊天标题、输入提示中文正常。
- 发送中文、英文、混排、标点、emoji 后消息气泡显示正常。

## 7. 注册/登录协议专项回归（推荐）

在服务端运行时执行下面脚本（默认端口 8888，如你在第5节改了端口需同步修改脚本）：

python3 - <<'PY'
import socket, json, struct, time

def send(msg, timeout=3):
	s=socket.create_connection(('127.0.0.1',8888),timeout=timeout)
	body=json.dumps(msg,separators=(',',':')).encode()
	s.sendall(struct.pack('>I',len(body))+body)
	s.settimeout(timeout)
	hdr=s.recv(4)
	ln=struct.unpack('>I',hdr)[0]
	data=b''
	while len(data)<ln:
		data+=s.recv(ln-len(data))
	s.close()
	return data.decode('utf-8',errors='replace')

u='u'+str(int(time.time()))[-6:]
p='pass1234'
print('REGISTER', send({'type':'register','username':u,'password':p}))
print('LOGIN_OK', send({'type':'login','username':u,'password':p}))
print('LOGIN_BAD', send({'type':'login','username':u,'password':'badpass'}))
PY

预期：
- REGISTER 返回 register_response 且 success 为 true。
- LOGIN_OK 返回 login_response 且 success 为 true。
- LOGIN_BAD 返回 login_response 且 success 为 false。

## 8. 打包测试

执行：

cd build
cpack -G DEB
ls -lh *.deb

检查包元数据：

dpkg-deb -f chatroom-1.0.0-Linux.deb Package Version Depends Recommends

预期：
- 生成 chatroom-1.0.0-Linux.deb。
- Depends 包含 libqt5core5a, libqt5widgets5, libqt5network5, fontconfig。
- Recommends 包含 fonts-noto-cjk, fonts-wqy-microhei。

## 9. 常见问题排查

1) 仍显示方框
- 先看 offscreen 日志是否命中 Huawei Sans。
- 若未命中，确认 resources/fonts/HuaweiSans-Regular.ttf 是否存在并重新 cmake + build。

2) 能运行但字体不一致
- 程序会在加载 QSS 后追加最终字体覆盖规则，优先以选中的嵌入字体为准。

3) 注册成功但登录失败
- 检查服务端是否出现 QSocketNotifier: Invalid socket 错误；如果有，说明未使用修复后的二进制。
- 确认正在运行的程序是最新构建产物 ./build/bin/chatroom。

4) 换机器后显示异常
- 优先携带 resources/fonts 下字体重建。
- 或执行 install.sh 安装系统中文字体作为兜底。
