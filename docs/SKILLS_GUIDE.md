# ESP32 NanoBot Skills Development Guide

## 什么是 Skills?

Skills 是 ESP32 NanoBot 的功能扩展模块，每个 Skill 完成一个特定的任务，例如：

- 📝 每日工作总结
- 🌤️ 天气预报
- 📧 邮件通知
- 📊 数据报表生成

## Skill 架构

每个 Skill 包含以下组件:

```
skills/
└── my_skill/
    ├── include/
    │   └── my_skill.h      # 头文件 (API 声明)
    ├── my_skill.c          # C 实现
    └── my_skill.cpp        # C++ 实现 (可选)
```

## 创建新 Skill

### 步骤 1: 创建目录结构

```bash
mkdir -p main/components/skills/my_skill/include
```

### 步骤 2: 创建头文件

```c
/**
 * @file my_skill.h
 * @brief My Custom Skill
 */

#ifndef MY_SKILL_H
#define MY_SKILL_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the skill
 * @return ESP_OK on success
 */
esp_err_t my_skill_init(void);

/**
 * @brief Main task function
 * @param pvParameters Task parameters
 */
void my_skill_task(void *pvParameters);

/**
 * @brief Execute the skill action
 * @param input Input data (optional)
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return ESP_OK on success
 */
esp_err_t my_skill_execute(const char *input, char *output, size_t output_size);

#ifdef __cplusplus
}
#endif

#endif // MY_SKILL_H
```

### 步骤 3: 创建实现文件

```cpp
/**
 * @file my_skill.cpp
 * @brief My Custom Skill Implementation
 */

#include "my_skill.h"
#include "ai_provider.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "my_skill";

esp_err_t my_skill_init(void)
{
    ESP_LOGI(TAG, "Initializing my skill...");
    // 初始化代码
    return ESP_OK;
}

void my_skill_task(void *pvParameters)
{
    ESP_LOGI(TAG, "My skill task started");

    while (1) {
        // 主循环逻辑
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

esp_err_t my_skill_execute(const char *input, char *output, size_t output_size)
{
    if (!output || output_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // 如果需要 AI 能力
    ai_message_t messages[] = {
        {AI_ROLE_SYSTEM, "你是一个有帮助的助手。"},
        {AI_ROLE_USER, input ? input : "你好"}
    };

    return ai_provider_chat(messages, 2, output, output_size);
}
```

### 步骤 4: 更新 CMakeLists.txt

在 `main/CMakeLists.txt` 中添加源文件:

```cmake
idf_component_register(
    SRCS
        # ... 其他文件
        "components/skills/my_skill/my_skill.cpp"
    INCLUDE_DIRS
        # ... 其他目录
        "components/skills/my_skill/include"
    # ...
)
```

### 步骤 5: 在 main.c 中注册

```c
#include "my_skill.h"

extern "C" void app_main(void)
{
    // ... 其他初始化

    // 初始化技能
    if (my_skill_init() == ESP_OK) {
        ESP_LOGI(TAG, "My Skill initialized");
        xTaskCreate(my_skill_task, "my_skill_task", 4096, NULL, 5, NULL);
    }
}
```

### 步骤 6: 添加配置选项 (可选)

在 `main/Kconfig.projbuild` 中添加:

```kconfig
menu "My Skill Settings"
    config MY_SKILL_ENABLED
        bool "Enable My Skill"
        default y
        help
            Enable my custom skill

    config MY_SKILL_PARAM
        string "My Skill Parameter"
        default "default_value"
        help
            Parameter for my skill
endmenu
```

## Skill 示例

### 示例 1: 天气预报 Skill

```c
esp_err_t weather_skill_get_forecast(const char *city, char *output, size_t size)
{
    // 调用天气 API
    // 格式化结果
    // 使用 AI 生成建议
}
```

### 示例 2: 定时提醒 Skill

```c
void reminder_skill_task(void *pvParameters)
{
    while (1) {
        // 检查提醒时间
        // 发送通知
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
```

### 示例 3: 邮件检查 Skill

```c
esp_err_t email_skill_check_unread(char *summary, size_t size)
{
    // 连接 IMAP 服务器
    // 获取未读邮件
    // 生成摘要
}
```

## 使用 AI Provider

所有 Skills 都可以使用 AI Provider:

```c
#include "ai_provider.h"

// 简单对话
ai_message_t messages[] = {
    {AI_ROLE_USER, "请帮我总结一下..."}
};
char response[4096];
ai_provider_chat(messages, 1, response, sizeof(response));

// 带系统提示的对话
ai_message_t messages_with_system[] = {
    {AI_ROLE_SYSTEM, "你是一个专业的翻译。"},
    {AI_ROLE_USER, "翻译成英文：你好"}
};
ai_provider_chat(messages_with_system, 2, response, sizeof(response));
```

## 发送通知

### QQ 通知

```c
#include "qq_bot.h"

qq_bot_send_text("user_openid", "提醒内容");
qq_bot_send_markdown("user_openid", "# 标题\n\n内容");
```

### 飞书通知

```c
#include "feishu_bot.h"

feishu_bot_send_text("user_open_id", "提醒内容");
```

## 最佳实践

### 1. 内存管理

```c
// 始终检查缓冲区大小
if (output_size < required_size) {
    return ESP_ERR_NO_MEM;
}

// 及时释放动态分配的内存
char *temp = malloc(1024);
// ... 使用 temp
free(temp);
```

### 2. 任务优先级

```c
// 低优先级任务 (如日志记录)
xTaskCreate(task_func, "task", 2048, NULL, 1, NULL);

// 中优先级任务 (如定时检查)
xTaskCreate(task_func, "task", 4096, NULL, 3, NULL);

// 高优先级任务 (如实时响应)
xTaskCreate(task_func, "task", 8192, NULL, 5, NULL);
```

### 3. 错误处理

```c
esp_err_t err = some_function();
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Function failed: %s", esp_err_to_name(err));
    // 适当的错误恢复
    return err;
}
```

### 4. 日志输出

```c
ESP_LOGI(TAG, "一般信息");
ESP_LOGD(TAG, "调试信息");
ESP_LOGW(TAG, "警告信息");
ESP_LOGE(TAG, "错误信息");
```

## 调试技巧

### 1. 启用详细日志

```bash
idf.py menuconfig
# Component config → Log output → Default log level: Debug
```

### 2. 监控堆内存

```c
ESP_LOGI(TAG, "Free heap: %lu", esp_get_free_heap_size());
ESP_LOGI(TAG, "Min heap: %lu", esp_get_minimum_free_heap_size());
```

### 3. 查看任务状态

```c
#include "freertos/task.h"

vTaskList(status_buffer);
printf("Task Status:\n%s", status_buffer);
```

## 分享你的 Skill

欢迎贡献 Skills!

1. 在 `skills/` 目录下创建你的 Skill
2. 编写 README 说明使用方法
3. 提交 Pull Request

---

## 常见问题

### Q: Skill 占用多少内存？

A: 简单 Skill 约需 2-4KB 栈空间，复杂 Skill 可能需要 8KB+

### Q: 如何调试 Skill?

A: 使用 `idf.py monitor` 查看串口输出，添加 `ESP_LOG*` 宏输出调试信息

### Q: Skill 可以互相调用吗？

A: 可以，但要注意避免循环依赖

---

祝你开发愉快！🚀
