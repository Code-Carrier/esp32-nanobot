# ESP32 NanoBot - 快速入门指南

## 5 分钟快速上手

### 第一步：安装 ESP-IDF（仅限首次）

**Windows 用户:**
1. 下载离线安装器：https://dl.espressif.com/dl/esp-idf/
2. 运行 `esp-idf-tools-setup-offline.exe`
3. 安装完成后，打开 "ESP-IDF CMD" 命令行工具

**macOS/Linux 用户:**
```bash
mkdir -p ~/esp && cd ~/esp
git clone -b v5.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32s3
. ./export.sh
```

### 第二步：配置项目

```bash
# 进入项目目录
cd esp32-nanobot

# 打开配置菜单
idf.py menuconfig
```

在配置菜单中设置：

1. **WiFi 配置** (Network Settings)
   - WiFi SSID: 你的 WiFi 名称
   - WiFi Password: 你的 WiFi 密码

2. **AI Provider 配置** (AI Provider Settings)
   - AI Provider API Key: 你的 API 密钥
   - AI Provider Type: 选择服务商 (deepseek/moonshot 等)

3. **QQ/飞书配置** (可选)
   - QQ App ID / App Secret
   - Feishu App ID / App Secret

保存配置（按 S）并退出（按 Q 多次）

### 第三步：编译烧录

```bash
# 设置目标芯片为 ESP32-S3
idf.py set-target esp32s3

# 烧录并监控串口输出
idf.py -p [端口号] flash monitor
```

**端口号示例:**
- Windows: `COM3`, `COM4`
- macOS: `/dev/cu.usbserial-*`
- Linux: `/dev/ttyUSB0`

### 第四步：观察输出

烧录成功后，你应该看到：

```
=======================================
   ESP32 NanoBot v1.0.0
   Ultra-Lightweight AI Assistant
=======================================
I (321) nvs: NVS initialized
I (341) wifi_manager: WiFi manager initialized
I (1521) wifi_manager: WiFi got IP: 192.168.1.100
I (1521) NanoBot: WiFi connected successfully
I (1531) ai_provider: AI Provider initialized: deepseek
I (1601) NanoBot: NanoBot started successfully!
```

---

## 获取 API 密钥

### DeepSeek (推荐)

1. 访问：https://platform.deepseek.com
2. 注册/登录账号
3. 进入「API 服务」→「API 密钥」
4. 创建新密钥并复制

### Moonshot (Kimi)

1. 访问：https://platform.moonshot.cn
2. 注册/登录账号
3. 进入控制台 → API 密钥
4. 创建新密钥

### 通义千问 (DashScope)

1. 访问：https://dashscope.console.aliyun.com
2. 使用阿里云账号登录
3. 进入 API-KEY 管理
4. 创建新密钥

---

## 配置 QQ 机器人

### 1. 注册开发者

访问 https://q.qq.com 并注册开发者账号

### 2. 创建应用

1. 进入「应用管理」→「创建应用」
2. 填写应用信息
3. 选择「机器人」类型

### 3. 获取凭证

1. 进入应用 → 「开发设置」
2. 复制 AppID 和 AppSecret

### 4. 配置沙箱

1. 进入「沙箱配置」
2. 添加你的 QQ 号为测试成员
3. 扫码绑定机器人

### 5. 填入配置

```bash
idf.py menuconfig
# ESP32 NanoBot Configuration → QQ Bot Settings
# - QQ App ID: [填入 AppID]
# - QQ App Secret: [填入 AppSecret]
```

---

## 配置飞书机器人

### 1. 创建应用

1. 访问 https://open.feishu.cn
2. 进入「企业管理」→「应用开发」
3. 创建企业自建应用

### 2. 添加权限

在「权限管理」中添加：
- `im:message` (发送消息)
- `im:message.p2p_msg:readonly` (接收消息)

### 3. 配置事件订阅

1. 进入「事件订阅」
2. 添加事件：`im.message.receive_v1`
3. 选择「长连接」模式

### 4. 获取凭证

1. 进入「凭证与基础信息」
2. 复制 App ID 和 App Secret

### 5. 发布应用

点击「版本管理与发布」→ 发布应用

---

## 常用命令

```bash
# 编译项目
idf.py build

# 烧录固件
idf.py -p [端口] flash

# 监控串口
idf.py -p [端口] monitor

# 编译并烧录
idf.py -p [端口] flash monitor

# 清除编译缓存
idf.py fullclean

# 查看项目大小
idf.py size-components
```

---

## 故障排除

### 无法识别串口

**Windows:**
- 安装 CP210x 驱动：https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- 安装 CH34x 驱动：https://www.wch.cn/downloads/CH34XSER_ZIP.html

**检查设备管理器:**
- Windows: 设备管理器 → 端口 (COM 和 LPT)
- macOS: `ls /dev/cu.*`
- Linux: `ls /dev/ttyUSB*`

### 烧录失败

1. 按住 BOOT 键不放
2. 按下 RST 键后松开
3. 松开 BOOT 键
4. 重新运行烧录命令

### WiFi 连接失败

- 确认使用 2.4GHz WiFi（不支持 5GHz）
- 检查 SSID 和密码是否正确
- 确认路由器未设置 MAC 过滤

### 内存不足

如果编译时提示内存不足：

```bash
# 降低日志级别
idf.py menuconfig
# Component config → Log output → Default log level: Warning
```

---

## 下一步

- 📖 阅读 [完整文档](docs/BUILD_AND_FLASH.md)
- 🔧 学习如何 [自定义技能](README.md#自定义技能)
- 💡 查看 [示例代码](main/components/)
- 🤝 加入 [社区讨论](https://github.com/your-repo/esp32-nanobot/discussions)

---

## 技术支持

遇到问题？

1. 查看 [常见问题](README.md#故障排除)
2. 搜索 [现有 Issues](https://github.com/your-repo/esp32-nanobot/issues)
3. 提交 [新 Issue](https://github.com/your-repo/esp32-nanobot/issues/new)

---

<div align="center">

**祝你使用愉快！**

如有问题欢迎反馈 🐛

</div>
