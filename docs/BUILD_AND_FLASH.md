# ESP32 NanoBot - 编译和烧录指南

## 📋 目录

- [系统要求](#系统要求)
- [安装 ESP-IDF](#安装 esp-idf)
- [项目配置](#项目配置)
- [编译固件](#编译固件)
- [烧录固件](#烧录固件)
- [监控串口输出](#监控串口输出)
- [故障排除](#故障排除)

---

## 系统要求

### 硬件要求

| 组件 | 规格要求 |
|------|----------|
| 开发板 | ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM) |
| USB 线 | USB-C 数据线（支持数据传输） |
| 电脑 | Windows 10/11, macOS 10.14+, 或 Linux |

### 软件要求

| 软件 | 版本要求 |
|------|----------|
| ESP-IDF | v5.0 或更高版本 |
| Python | 3.8 - 3.11 |
| CMake | 3.16 或更高 |
| 串口驱动 | CP210x 或 CH34x（取决于开发板） |

---

## 安装 ESP-IDF

### Windows 安装

1. **下载离线安装器**
   ```
   https://dl.espressif.com/dl/esp-idf/?idf=5.2
   ```
   下载 `esp-idf-tools-setup-offline-5.2.exe`

2. **运行安装器**
   - 双击运行下载的安装程序
   - 选择安装路径（推荐：`C:\Espressif\frameworks\esp-idf-v5.2`）
   - 勾选所有组件
   - 点击 "Install"

3. **打开 ESP-IDF 命令行**
   - 安装完成后，桌面会出现 "ESP-IDF 5.2 CMD" 快捷方式
   - 双击打开，会自动配置所有环境变量

### macOS 安装

```bash
# 1. 安装依赖
brew install cmake ninja dfu-util

# 2. 创建安装目录
mkdir -p ~/esp
cd ~/esp

# 3. 克隆 ESP-IDF
git clone -b v5.2 --recursive https://github.com/espressif/esp-idf.git

# 4. 运行安装脚本
cd esp-idf
./install.sh esp32s3

# 5. 加载环境变量
. ./export.sh
```

### Linux 安装

```bash
# 1. 安装依赖 (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y git wget flex bison gperf cmake ninja-build \
    ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 python3-pip

# 2. 创建安装目录
mkdir -p ~/esp
cd ~/esp

# 3. 克隆 ESP-IDF
git clone -b v5.2 --recursive https://github.com/espressif/esp-idf.git

# 4. 运行安装脚本
cd esp-idf
./install.sh esp32s3

# 5. 加载环境变量
. ./export.sh
```

---

## 项目配置

### 1. 进入项目目录

```bash
cd esp32-nanobot
```

### 2. 打开配置菜单

```bash
idf.py menuconfig
```

### 3. 配置 WiFi 连接

```
ESP32 NanoBot Configuration →
    Network Settings →
        WiFi SSID: [你的 WiFi 名称]
        WiFi Password: [你的 WiFi 密码]
```

### 4. 配置 AI Provider

```
ESP32 NanoBot Configuration →
    AI Provider Settings →
        AI Provider API Key: [你的 API 密钥]
        AI Provider Type: deepseek (或 moonshot/dashscope 等)
        AI Model Name: deepseek-chat
        Provider Base URL: https://api.deepseek.com
```

### 5. 配置 QQ Bot（可选）

```
ESP32 NanoBot Configuration →
    QQ Bot Settings →
        Enable QQ Bot: [*]
        QQ App ID: [你的 QQ AppID]
        QQ App Secret: [你的 QQ AppSecret]
```

### 6. 配置飞书 Bot（可选）

```
ESP32 NanoBot Configuration →
    Feishu Bot Settings →
        Enable Feishu Bot: [*]
        Feishu App ID: [你的飞书 AppID]
        Feishu App Secret: [你的飞书 AppSecret]
```

### 7. 配置每日总结

```
ESP32 NanoBot Configuration →
    Daily Summary Skill →
        Daily Summary Send Time (HH:MM): 18:00
```

### 8. 保存并退出

- 按 `S` 保存配置
- 按 `Q` 退出（可能需要多次按）

---

## 编译固件

### 完整编译

```bash
# 设置目标芯片为 ESP32-S3
idf.py set-target esp32s3

# 编译项目
idf.py build
```

编译完成后，会在 `build/` 目录下生成以下文件：
- `bootloader/bootloader.bin` - 引导加载程序
- `partition_table/partition-table.bin` - 分区表
- `esp32_nanobot.bin` - 应用程序固件

### 编译时间参考

| 电脑配置 | 首次编译 | 增量编译 |
|----------|----------|----------|
| i7/16GB RAM | ~3-5 分钟 | ~30 秒 |
| i5/8GB RAM | ~5-8 分钟 | ~1 分钟 |
| M1 Mac | ~2-3 分钟 | ~20 秒 |

---

## 烧录固件

### 方法一：自动烧录（推荐）

```bash
# 连接开发板后，直接烧录
idf.py -p [端口号] flash monitor
```

**端口号说明：**
- Windows: `COM3`, `COM4`, `COM5` 等
- macOS: `/dev/cu.usbserial-*` 或 `/dev/cu.wchusbserial*`
- Linux: `/dev/ttyUSB0` 或 `/dev/ttyACM0`

**示例：**
```bash
# Windows
idf.py -p COM3 flash monitor

# macOS
idf.py -p /dev/cu.usbserial-140 flash monitor

# Linux
idf.py -p /dev/ttyUSB0 flash monitor
```

### 方法二：手动烧录

```bash
# 1. 进入下载模式（按住 BOOT 键，按 RST 键）

# 2. 烧录所有固件
esptool.py -p [端口号] write_flash 0x0 build/bootloader/bootloader.bin \
                                           0x8000 build/partition_table/partition-table.bin \
                                           0x10000 build/esp32_nanobot.bin

# 3. 重启开发板（按 EN/RST 键）
```

### 方法三：使用 Flash 下载工具（Windows）

1. 下载 "Flash Download Tool" from Espressif
2. 选择以下文件：
   - 0x0000: `build/bootloader/bootloader.bin`
   - 0x8000: `build/partition_table/partition-table.bin`
   - 0x10000: `build/esp32_nanobot.bin`
3. 选择正确的 COM 端口
4. 点击 "START"

---

## 监控串口输出

### 使用 idf.py 监控

```bash
# 单独启动监控（如果已经烧录）
idf.py -p [端口号] monitor
```

### 监控时的快捷命令

| 按键 | 功能 |
|------|------|
| `Ctrl+]` | 退出监控模式 |
| `Ctrl+R` | 重启设备 |
| `Ctrl+T` | 重启监控 |

### 使用其他串口工具

**PuTTY (Windows):**
- Host: COM 端口（如 COM3）
- Port: 115200
- Connection type: Serial

**screen (macOS/Linux):**
```bash
screen /dev/cu.usbserial-140 115200
# 退出：Ctrl+A, 然后 D
```

---

## 预期输出

烧录成功后，串口输出应显示：

```
=======================================
   ESP32 NanoBot v1.0.0
   Ultra-Lightweight AI Assistant
=======================================
I (321) nvs: NVS initialized
I (331) wifi_manager: WiFi manager initialized
I (341) wifi: WiFi STA started, connecting to MyWiFi...
I (1521) wifi_manager: WiFi got IP: 192.168.1.100
I (1521) NanoBot: WiFi connected successfully
I (1531) ai_provider: AI Provider initialized: deepseek
I (1531) ai_provider:   Base URL: https://api.deepseek.com
I (1541) ai_provider:   Model: deepseek-chat
I (1551) qq_bot: QQ Bot initialized (AppID: 1234****)
I (1561) feishu_bot: Feishu Bot initialized (AppID: cli_xxx****)
I (1571) daily_summary: Daily Summary initialized
I (1581) daily_summary:   Send time: 18:00
I (1591) daily_summary:   Channel: qq
I (1601) NanoBot: NanoBot started successfully!
```

---

## 故障排除

### 问题 1: 无法识别串口

**症状:** `No Serial device found`

**解决方案:**
1. 检查 USB 线是否为数据线（有些线仅支持充电）
2. 安装串口驱动：
   - CP210x: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
   - CH34x: https://www.wch.cn/downloads/CH34XSER_ZIP.html
3. 尝试不同的 USB 端口

### 问题 2: 烧录失败

**症状:** `Failed to connect to ESP32-S3`

**解决方案:**
1. 按住 BOOT 键，然后按 RST 键，松开 BOOT 键（进入下载模式）
2. 检查端口号是否正确
3. 尝试降低烧录速度：
   ```bash
   idf.py -p [端口号] --baud 921600 flash
   ```

### 问题 3: WiFi 连接失败

**症状:** `WiFi disconnected` 或超时

**解决方案:**
1. 检查 SSID 和密码是否正确
2. 确保 WiFi 是 2.4GHz（ESP32 不支持 5GHz）
3. 检查路由器是否设置了 MAC 地址过滤

### 问题 4: API 请求失败

**症状:** `HTTP request failed` 或状态码非 200

**解决方案:**
1. 检查 API Key 是否正确配置
2. 检查网络连接
3. 查看 API 提供商的状态页面
4. 增加超时时间：
   ```
   AI Provider Settings →
       Timeout (ms): 60000
   ```

### 问题 5: 内存不足

**症状:** `Heap overflow` 或 `Malloc failed`

**解决方案:**
1. 降低日志级别：
   ```
   Component config → Log output → Default log level: Warning
   ```
2. 减少任务栈大小
3. 启用 PSRAM 支持

---

## 高级配置

### 更改分区表

编辑 `partitions.csv`:

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000
phy_init, data, phy,     0xf000,  0x1000
factory,  app,  factory, 0x10000, 4M
otadata,  data, ota,     ,        0x2000
ota_0,    app,  ota_0,   ,        4M
ota_1,    app,  ota_1,   ,        4M
```

### 启用 OTA 更新

```
Bootloader config → OTA support → Enable OTA updates
```

### 启用 JTAG 调试

```
Serial flasher config → UART for console output → Use USB CDC for console
Component config → ESP32S3-specific → Enable JTAG
```

---

## 获取帮助

- ESP-IDF 官方文档：https://docs.espressif.com/projects/esp-idf/zh_CN/latest/
- ESP32-S3 技术参考手册：https://www.espressif.com.cn/sites/default/files/documentation/esp32-s3_technical_reference_manual_cn.pdf
- 项目 Issues: https://github.com/your-repo/esp32-nanobot/issues
