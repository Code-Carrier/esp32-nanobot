# ESP32 NanoBot

<div align="center">

**适用于 ESP32-S3 的超轻量级 AI 助手**

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.0+-blue)](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange)](https://www.espressif.com.cn/products/socs/esp32-s3)

</div>

## 📖 项目简介

ESP32 NanoBot 是一个受 [nanobot](https://github.com/HKUDS/nanobot) 启发，为 ESP32-S3 微控制器设计的超轻量级 AI 助手。它可以在资源受限的嵌入式设备上实现 AI 对话、消息推送和自动化任务。

### ✨ 核心特性

- 🤖 **多 AI 模型支持** - 支持 DeepSeek、月之暗面 (Kimi)、通义千问、智谱 AI 等国产大模型
- 💬 **QQ 机器人** - 通过 QQ 开放平台实现消息收发
- 📱 **飞书集成** - 支持飞书 (Feishu) 企业协作平台
- 📝 **每日工作总结** - 自动获取 git commit 并生成工作总结
- 🌐 **Web 配置** - 手机扫码即可配置，无需电脑
- 🔌 **TFT 屏幕支持** - 支持 1.69 寸 SPI TFT 显示屏
- 🔧 **串口命令行** - 便捷的串口配置界面
- 📦 **模块化设计** - 易于扩展新的技能和功能

---

## 🎯 配置方式

### 三种配置方式供选择

| 方式 | 适用场景 | 说明 |
|------|----------|------|
| **🌐 Web 配置** | 首次使用 | 手机连接热点，打开网页即可配置 |
| **💻 串口命令行** | 开发调试 | 通过串口输入命令配置 |
| **⚙️ menuconfig** | 批量部署 | 编译时固化配置 |

**推荐使用 Web 配置** - 最简单！首次烧录后设备会自动创建热点 `NanoBot-Setup`，手机连接后打开 `http://192.168.4.1` 即可配置。

详细配置方法请参考 [配置指南](docs/CONFIGURATION.md)

---

## 🛠️ 硬件要求

| 组件 | 规格 |
|------|------|
| 开发板 | ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM) |
| USB 线 | USB-C 数据线（支持数据传输） |
| 网络 | 2.4GHz WiFi |
| 屏幕（可选） | 1.69 寸 TFT 8 针 SPI 显示屏 |

**推荐开发板：**
- ESP32-S3-DevKitC-1-N16R8
- ESP32-S3-WROOM-1-N16R8

---

## 📦 快速开始

### 步骤 1：克隆项目

```bash
git clone https://github.com/your-username/esp32-nanobot.git
cd esp32-nanobot
```

### 步骤 2：安装 ESP-IDF

参考 [ESP-IDF 安装指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/)

### 步骤 3：编译烧录

```bash
# 设置目标芯片
idf.py set-target esp32s3

# 编译并烧录
idf.py -p [端口号] flash monitor
```

### 步骤 4：配置设备

**首次烧录后，设备会自动进入配网模式：**

1. 打开手机 WiFi 列表
2. 连接热点：`NanoBot-Setup`，密码：`12345678`
3. 浏览器打开：`http://192.168.4.1`
4. 填写 WiFi、AI Provider 等配置
5. 点击"保存配置并重启"

**详细步骤请参考：**
- 📖 [配置指南](docs/CONFIGURATION.md) - Web/串口/menuconfig 三种配置方式
- 🔧 [编译烧录指南](docs/BUILD_AND_FLASH.md) - 详细编译步骤
- ⚡ [快速入门](docs/QUICKSTART.md) - 5 分钟上手

---

## 🖥️ TFT 屏幕连接

支持 1.69 寸 TFT 8 针 SPI 显示屏（ST7789 驱动）

### 引脚连接图

```
┌─────────────────────────────────────────────────────────────┐
│  TFT 屏幕 (8 针 SPI)  →  ESP32-S3-N16R8  引脚连接              │
├─────────────────────────────────────────────────────────────┤
│   TFT 引脚              ESP32-S3 GPIO                       │
│   ───────              ──────────────                        │
│   VCC       ────────→  3.3V                                 │
│   GND       ────────→  GND                                  │
│   CLK/SCK   ────────→  GPIO12 (SPI SCK)                     │
│   SDA/MOSI  ────────→  GPIO11 (SPI MOSI)                    │
│   CS        ────────→  GPIO10 (SPI CS)                      │
│   DC/RS     ────────→  GPIO9  (数据/命令选择)                │
│   RES/RST   ────────→  GPIO15 (复位)                        │
│   BLK/BLED  ────────→  GPIO14 (背光 PWM)                    │
└─────────────────────────────────────────────────────────────┘
```

### 屏幕显示内容

| 状态 | 显示内容 |
|------|----------|
| 启动 | ESP32 NanoBot v1.0.0 |
| WiFi 连接中 | 正在连接 WiFi... |
| WiFi 已连接 | IP 地址 |
| AI 状态 | Ready / Not Configured |

---

## ⚙️ AI Provider 配置

支持以下国产 AI 模型：

| Provider | 模型 | API 地址 | 价格参考 |
|----------|------|----------|----------|
| **DeepSeek** | deepseek-chat | https://api.deepseek.com | ¥0.002/1K tokens |
| **Moonshot (Kimi)** | moonshot-v1-8k | https://api.moonshot.cn | ¥0.012/1K tokens |
| **DashScope (通义千问)** | qwen-plus | https://dashscope.aliyuncs.com | ¥0.004/1K tokens |
| **SiliconFlow** | Qwen2.5-72B | https://api.siliconflow.cn | 免费额度 |
| **VolcEngine (豆包)** | Doubao-pro | https://ark.cn-beijing.volces.com | 免费额度 |
| **Zhipu (智谱)** | glm-4 | https://open.bigmodel.cn | 免费额度 |

**获取 API Key 步骤：**
1. 访问对应平台官网
2. 注册/登录账号
3. 进入控制台 → API 密钥
4. 创建并复制密钥

---

## 💬 机器人配置

### QQ 机器人

1. 访问 [QQ 开放平台](https://q.qq.com)
2. 创建机器人应用
3. 获取 AppID 和 AppSecret
4. 沙箱配置中添加测试成员

### 飞书机器人

1. 访问 [飞书开放平台](https://open.feishu.cn)
2. 创建企业自建应用
3. 启用机器人权限
4. 获取 App ID 和 App Secret

---

## 🏗️ 项目结构

```
esp32-nanobot/
├── main/                          # 主程序目录
│   ├── CMakeLists.txt             # 组件构建配置
│   ├── main.cpp                   # 程序入口
│   └── components/                # 功能组件
│       ├── config_manager/        # 配置管理器 (NVS + Web + CLI)
│       ├── wifi_manager/          # WiFi 连接管理
│       ├── ai_provider/           # AI 服务提供者
│       ├── qq_bot/                # QQ 机器人
│       ├── feishu_bot/            # 飞书机器人
│       ├── skills/                # 技能模块
│       ├── tft_display/           # TFT 显示屏驱动
│       └── utils/                 # 工具函数
├── docs/                          # 文档目录
│   ├── CONFIGURATION.md           # 配置指南 (Web/串口/menuconfig)
│   ├── BUILD_AND_FLASH.md         # 编译烧录指南
│   ├── QUICKSTART.md              # 快速入门
│   └── SKILLS_GUIDE.md            # Skills 开发指南
├── scripts/                       # 脚本工具
│   ├── flash.bat                  # Windows 烧录脚本
│   └── flash.sh                   # Linux/macOS 烧录脚本
├── CMakeLists.txt                 # 项目构建配置
├── sdkconfig.defaults             # 默认配置
├── partitions.csv                 # 分区表
└── README.md                      # 项目说明
```

---

## 📚 使用示例

### 串口命令行配置

烧录后通过串口输入命令：

```bash
# 查看帮助
nanobot> help

# 配置 WiFi
nanobot> wifi MyWiFi MyPassword123

# 配置 AI Provider
nanobot> ai deepseek sk-xxxxxxxxx deepseek-chat

# 配置 QQ 机器人
nanobot> qq 12345678 abcdefghijk

# 保存并重启
nanobot> save
nanobot> reboot
```

### 代码示例

```c
// 发送 AI 请求
#include "ai_provider.h"

ai_message_t messages[] = {
    {AI_ROLE_SYSTEM, "你是一个有帮助的助手。"},
    {AI_ROLE_USER, "你好，请介绍一下自己。"}
};

char response[4096];
esp_err_t err = ai_provider_chat(messages, 2, response, sizeof(response));

// 发送 QQ 消息
#include "qq_bot.h"
qq_bot_send_text("user_openid", "Hello from ESP32!");

// 发送飞书消息
#include "feishu_bot.h"
feishu_bot_send_text("user_open_id", "Hello from ESP32!");
```

---

## 📊 资源占用

| 项目 | 占用 |
|------|------|
| Flash | ~3.5 MB |
| PSRAM | ~1.5 MB |
| 堆内存 | ~200 KB |
| 任务栈 | ~50 KB |

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

### 待开发功能

- [x] Web 配置界面
- [x] 串口命令行配置
- [x] TFT 屏幕支持
- [ ] WebSocket 长连接
- [ ] 语音交互功能
- [ ] SD 卡存储 git 仓库
- [ ] OTA 远程升级

---

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 🔗 相关链接

| 文档 | 链接 |
|------|------|
| 📖 [配置指南](docs/CONFIGURATION.md) | Web/串口/menuconfig 三种配置方式 |
| 🔧 [编译烧录指南](docs/BUILD_AND_FLASH.md) | 详细编译步骤 |
| ⚡ [快速入门](docs/QUICKSTART.md) | 5 分钟上手 |
| 🛠️ [Skills 开发指南](docs/SKILLS_GUIDE.md) | 自定义技能教程 |
| ESP-IDF 编程指南 | https://docs.espressif.com/projects/esp-idf/zh_CN/latest/ |
| ESP32-S3 技术参考手册 | https://www.espressif.com.cn/sites/default/files/documentation/esp32-s3_technical_reference_manual_cn.pdf |
| DeepSeek API 文档 | https://platform.deepseek.com/api-docs/ |
| 飞书开放平台 | https://open.feishu.cn/document/ukTMukTMukTM/uEjNwUjLxYDM14SM2ATN |
| QQ 开放平台 | https://bot.q.qq.com/wiki/ |

---

<div align="center">

**感谢使用 ESP32 NanoBot!**

Made with ❤️ for the ESP32 community

如有问题，请查看 [配置指南](docs/CONFIGURATION.md) 或提交 Issue

</div>
