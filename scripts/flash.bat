@echo off
REM ESP32 NanoBot - Windows 一键烧录脚本

echo =======================================
echo   ESP32 NanoBot - 烧录工具
echo =======================================
echo.

REM 检查 ESP-IDF 环境
if not defined IDF_PATH (
    echo [错误] 未检测到 ESP-IDF 环境!
    echo.
    echo 请先打开 "ESP-IDF CMD" 命令行工具
    echo 或者运行：call %IDF_TOOLS_PATH%\idf_cmd_init.bat
    echo.
    pause
    exit /b 1
)

echo [信息] ESP-IDF 环境检测成功
echo [信息] IDF_PATH: %IDF_PATH%
echo.

REM 设置目标芯片
echo [步骤 1/3] 设置目标芯片为 ESP32-S3...
idf.py set-target esp32s3
if errorlevel 1 (
    echo [错误] 设置目标芯片失败!
    pause
    exit /b 1
)
echo.

REM 编译
echo [步骤 2/3] 开始编译...
idf.py build
if errorlevel 1 (
    echo [错误] 编译失败!
    pause
    exit /b 1
)
echo.
echo [信息] 编译成功!
echo.

REM 列出可用串口
echo [步骤 3/3] 检测串口...
echo.
mode com

echo.
echo 请连接 ESP32-S3 开发板
echo 如果已连接，请输入串口号 (如 COM3):
set /p COM_PORT=

if "%COM_PORT%"=="" (
    echo [错误] 未输入串口号!
    pause
    exit /b 1
)

echo.
echo 开始烧录到 %COM_PORT%...
echo 按 Ctrl+C 可取消
echo.

idf.py -p %COM_PORT% flash monitor

echo.
echo =======================================
echo   烧录完成!
echo =======================================
pause
