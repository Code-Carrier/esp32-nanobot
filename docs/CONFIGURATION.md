# ESP32 NanoBot - 配置指南

## 📋 目录

1. [配置方式概述](#配置方式概述)
2. [方式一：Web 配置 (推荐)](#方式一 web-配置-推荐)
3. [方式二：串口命令行配置](#方式二串口命令行配置)
4. [方式三：编译时配置](#方式三编译时配置)
5. [TFT 屏幕连接](#tft 屏幕连接)
6. [常见问题](#常见问题)

---

## 配置方式概述

| 方式 | 适用场景 | 难度 |
|------|----------|------|
| **Web 配置** | 首次使用，无电脑在身边 | ⭐ 简单 |
| **串口命令行** | 开发调试，快速修改 | ⭐⭐ 中等 |
| **编译时配置** | 批量部署，固定配置 | ⭐⭐⭐ 较复杂 |

---

## 方式一：Web 配置（推荐）

### 首次使用 - 自动进入配网模式

设备检测到没有保存的配置时，会**自动进入 AP 配置模式**。

#### 步骤 1：连接 WiFi

1. 烧录固件后，设备自动启动
2. 打开手机 WiFi 列表
3. 连接热点：`NanoBot-Setup`
4. 密码：`12345678`

#### 步骤 2：打开配置页面

1. 手机浏览器打开：`http://192.168.4.1`
2. 看到配置页面

#### 步骤 3：填写配置

**WiFi 设置**
```
WiFi 名称：你的 WiFi SSID
WiFi 密码：你的 WiFi 密码
⚠️ 仅支持 2.4GHz WiFi
```

**AI 服务配置**
```
AI 服务商：选择 DeepSeek/Moonshot/通义千问等
API Base URL: 自动填充（可修改）
API Key: 输入你的 API 密钥
模型名称：留空使用默认
```

**机器人配置**（可选）
```
QQ 机器人：
  ☑ 启用 QQ 机器人
  App ID: 你的 QQ AppID
  App Secret: 你的 QQ 密钥
  接收者 OpenID: 消息接收者 ID

飞书机器人：
  ☑ 启用飞书机器人
  App ID: 你的飞书 AppID
  App Secret: 你的飞书密钥
```

**每日工作总结**
```
发送时间：18:00
发送渠道：QQ / 飞书
```

#### 步骤 4：保存并重启

1. 点击 "保存配置并重启"
2. 设备重启后自动连接 WiFi
3. 配置完成！

---

### 手动进入配网模式

如果需要重新进入配网模式：

1. **按住 BOOT 键**（GPIO0）
2. **按下 RST 键**重启设备
3. **继续按住 BOOT 键 3 秒**
4. 设备进入配网模式

---

## 方式二：串口命令行配置

### 连接串口

```bash
# Windows (使用 PuTTY 或 idf.py monitor)
idf.py -p COM3 monitor

# macOS/Linux
idf.py -p /dev/ttyUSB0 monitor
```

### 配置命令

设备启动后，在串口输入 `help` 查看可用命令：

```
╔═══════════════════════════════════════════╗
║   ESP32 NanoBot Configuration CLI v1.0    ║
║   Type 'help' for available commands      ║
╚═══════════════════════════════════════════╝

nanobot> help

Available commands:
  help                              - Show this help message
  status                            - Show current configuration
  wifi <ssid> <password>            - Set WiFi credentials
  ai <provider> <api_key> [model]   - Set AI provider
  qq <appid> <secret> [openid]      - Set QQ bot
  feishu <appid> <secret> [openid]  - Set Feishu bot
  summary <HH:MM> <channel>         - Set daily summary
  scan                              - Scan available WiFi networks
  reset                             - Reset all configuration
  save                              - Save configuration to NVS
  reboot                            - Reboot device
```

### 配置示例

#### 1. 配置 WiFi

```bash
nanobot> wifi MyWiFi MyPassword123
WiFi credentials set. Type 'save' to save.

nanobot> scan
Scanning WiFi networks...

Found 5 networks:

  SSID                             BSSID            RSSI
  -------------------------------- ---------------- ----
  MyWiFi                           xx:xx:xx:xx:xx   -45
  Neighbor-WiFi                    yy:yy:yy:yy:yy   -67
  ...
```

#### 2. 配置 AI Provider

```bash
# DeepSeek
nanobot> ai deepseek sk-xxxxxxxxx deepseek-chat
AI provider set. Type 'save' to save.

# Moonshot (Kimi)
nanobot> ai moonshot sk-xxxxxxxxx moonshot-v1-8k
AI provider set. Type 'save' to save.

# 通义千问
nanobot> ai dashscope sk-xxxxxxxxx qwen-plus
AI provider set. Type 'save' to save.
```

#### 3. 配置 QQ 机器人

```bash
nanobot> qq 12345678 abcdefghijk
QQ bot set. Type 'save' to save.
```

#### 4. 配置飞书机器人

```bash
nanobot> feishu cli_xxxxx xxxxxxxxxxxx
Feishu bot set. Type 'save' to save.
```

#### 5. 配置每日总结

```bash
nanobot> summary 18:00 qq
Daily summary set. Type 'save' to save.
```

#### 6. 保存配置

```bash
nanobot> save
Configuration saved to NVS.

nanobot> reboot
Rebooting...
```

### 查看状态

```bash
nanobot> status

=== Current Configuration ===

WiFi:
  SSID: MyWiFi
  Status: configured

AI Provider:
  Type: deepseek
  Model: deepseek-chat
  URL: https://api.deepseek.com
  API Key: ******
  Status: configured

QQ Bot:
  Enabled: yes
  App ID: 12345678

Feishu Bot:
  Enabled: no

Daily Summary:
  Time: 18:00
  Channel: qq

================================
```

---

## 方式三：编译时配置

### 使用 menuconfig

```bash
idf.py menuconfig
```

#### 1. 配置 WiFi

```
ESP32 NanoBot Configuration → Network Settings
  → WiFi SSID: [输入你的 WiFi 名称]
  → WiFi Password: [输入你的 WiFi 密码]
```

#### 2. 配置 AI Provider

```
ESP32 NanoBot Configuration → AI Provider Settings
  → AI Provider Type: deepseek
  → AI Provider API Key: sk-xxxxxxxxx
  → AI Model Name: deepseek-chat
  → Provider Base URL: https://api.deepseek.com
```

#### 3. 配置机器人

```
ESP32 NanoBot Configuration → QQ Bot Settings
  → Enable QQ Bot: [*]
  → QQ App ID: xxxxxxxx
  → QQ App Secret: xxxxxxxxxxxx

ESP32 NanoBot Configuration → Feishu Bot Settings
  → Enable Feishu Bot: [*]
  → Feishu App ID: cli_xxxxxxxxx
  → Feishu App Secret: xxxxxxxxxxxx
```


#### 4. 配置每日总结 GitHub 提交来源（可选）

> 如果你主要把代码上传到 GitHub，建议开启该项，这样每日总结会拉取**真实提交**而不是示例数据。

```
ESP32 NanoBot Configuration → Daily Summary Skill
  → Enable GitHub API for real commits: [*]
  → GitHub owner: [你的 GitHub 用户名或组织]
  → GitHub repository: [仓库名]
  → GitHub author (optional): [你的 GitHub 登录名，可选]
  → GitHub token (optional): [PAT，可选，私有仓库建议配置]
```

### 保存并编译

1. 按 `S` 保存配置
2. 按 `Q` 退出菜单
3. 编译烧录：

```bash
idf.py -p [端口] flash monitor
```

---

## TFT 屏幕连接

### 1.69 寸 TFT 8 针屏幕引脚定义

```
┌─────────────────────────────────────────────────────────────┐
│  TFT 屏幕 (8 针 SPI)  →  ESP32-S3-N16R8  引脚连接              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
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
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 接线图

```
        ┌─────────────────┐
        │   TFT 屏幕      │
        │   (正面)        │
        │                 │
        │  ○ VCC  ────────┼──→ 3.3V
        │  ○ GND  ────────┼──→ GND
        │  ○ CLK  ────────┼──→ GPIO12
        │  ○ SDA  ────────┼──→ GPIO11
        │  ○ CS   ────────┼──→ GPIO10
        │  ○ DC   ────────┼──→ GPIO9
        │  ○ RES  ────────┼──→ GPIO15
        │  ○ BLK  ────────┼──→ GPIO14
        └─────────────────┘
```

### 启用/禁用显示

在配置中设置：

```bash
# 串口命令
nanobot> display enable 1
nanobot> display brightness 80
nanobot> save
```

或在 `menuconfig` 中：

```
ESP32 NanoBot Configuration → Display Settings
  → Enable TFT Display: [*]
  → Brightness: 80
```

### 屏幕显示内容

| 状态 | 显示内容 |
|------|----------|
| 启动 | ESP32 NanoBot v1.0.0 |
| WiFi 连接中 | 正在连接 WiFi... |
| WiFi 已连接 | IP 地址 |
| AI 初始化 | AI 状态（Ready/Not Configured） |
| 正常运行 | 系统状态（每 5 秒更新） |

---

## 获取 API 密钥

### DeepSeek (推荐)

1. 访问：https://platform.deepseek.com
2. 注册/登录
3. 进入「API 服务」→「API 密钥」
4. 创建密钥，复制保存

**价格参考：**
- DeepSeek-V3: ¥0.002 / 1K tokens (输入)
- DeepSeek-R1: ¥0.004 / 1K tokens (输入)

### Moonshot (Kimi)

1. 访问：https://platform.moonshot.cn
2. 注册/登录
3. 控制台 → API 密钥
4. 创建密钥

**价格参考：**
- moonshot-v1-8k: ¥0.012 / 1K tokens

### 通义千问 (DashScope)

1. 访问：https://dashscope.console.aliyun.com
2. 阿里云账号登录
3. API-KEY 管理 → 创建密钥

**价格参考：**
- qwen-plus: ¥0.004 / 1K tokens (输入)

---

## 常见问题

### Q1: 忘记 API 密钥怎么办？

**A:** 在对应平台的控制台重新生成即可。旧的密钥会自动失效。

### Q2: 配置保存后无法连接 WiFi？

**A:**
1. 确认使用 2.4GHz WiFi
2. 检查 SSID 和密码是否正确
3. 尝试重置配置：`nanobot> reset`
4. 重新配置

### Q3: 如何查看当前配置？

**A:** 使用 `nanobot> status` 命令

### Q4: 如何恢复出厂设置？

**A:**
```bash
nanobot> reset
nanobot> reboot
```

### Q5: 屏幕不亮怎么办？

**A:**
1. 检查接线是否正确
2. 确认供电充足（使用 5V/2A 电源）
3. 检查背光 GPIO 是否正确（GPIO14）
4. 调整亮度：`nanobot> display brightness 100`

### Q6: 如何切换配置方式？

**A:** 三种配置方式可以混合使用，后保存的配置会覆盖之前的。建议首次使用 Web 配置，后续微调使用串口命令行。

---

## 配置优先级

```
1. Web 配置（最后保存的优先）
2. 串口命令行配置
3. 编译时配置（最低优先级）
```

---

## 下一步

配置完成后，你的 ESP32 NanoBot 就可以：

✅ 通过 AI 进行智能对话
✅ 接收 QQ/飞书消息
✅ 自动发送每日工作总结
✅ 在 TFT 屏幕上显示状态

享受你的 AI 助手吧！🎉
