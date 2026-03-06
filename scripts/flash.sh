#!/bin/bash
# ESP32 NanoBot - Linux/macOS 烧录脚本

echo "======================================="
echo "  ESP32 NanoBot - 烧录工具"
echo "======================================="
echo ""

# 检查 ESP-IDF 环境
if [ -z "$IDF_PATH" ]; then
    echo "[错误] 未检测到 ESP-IDF 环境!"
    echo ""
    echo "请先运行: source \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

echo "[信息] ESP-IDF 环境检测成功"
echo "[信息] IDF_PATH: $IDF_PATH"
echo ""

# 自动检测串口
detect_serial() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        ls /dev/cu.usbserial* 2>/dev/null || ls /dev/cu.wchusbserial* 2>/dev/null
    else
        # Linux
        ls /dev/ttyUSB* 2>/dev/null || ls /dev/ttyACM* 2>/dev/null
    fi
}

SERIAL_PORTS=$(detect_serial)

if [ -z "$SERIAL_PORTS" ]; then
    echo "[警告] 未检测到串口设备"
    echo "请连接 ESP32-S3 开发板后重试"
    echo ""
fi

echo "检测到的串口设备:"
echo "$SERIAL_PORTS"
echo ""

# 获取串口号
if [ -n "$SERIAL_PORTS" ]; then
    # 使用第一个检测到的串口
    COM_PORT=$(echo "$SERIAL_PORTS" | head -n 1)
    echo "使用串口：$COM_PORT"
    echo ""
    echo "如需使用其他串口，请输入串口号 (直接回车使用默认):"
    read -p "> " INPUT_PORT
    if [ -n "$INPUT_PORT" ]; then
        COM_PORT="$INPUT_PORT"
    fi
else
    read -p "请输入串口号 (如 /dev/ttyUSB0): " COM_PORT
fi

if [ -z "$COM_PORT" ]; then
    echo "[错误] 未输入串口号!"
    exit 1
fi

echo ""
echo "[步骤 1/3] 设置目标芯片为 ESP32-S3..."
idf.py set-target esp32s3
if [ $? -ne 0 ]; then
    echo "[错误] 设置目标芯片失败!"
    exit 1
fi
echo ""

echo "[步骤 2/3] 开始编译..."
idf.py build
if [ $? -ne 0 ]; then
    echo "[错误] 编译失败!"
    exit 1
fi
echo ""
echo "[信息] 编译成功!"
echo ""

echo "[步骤 3/3] 开始烧录到 $COM_PORT..."
echo "按 Ctrl+C 可取消"
echo ""

idf.py -p "$COM_PORT" flash monitor

echo ""
echo "======================================="
echo "  烧录完成!"
echo "======================================="
