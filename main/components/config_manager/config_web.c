/**
 * @file config_web.c
 * @brief Web Server Implementation for WiFi AP Configuration
 *
 * When device starts in AP mode, connect to "NanoBot-Setup" WiFi network
 * and open http://192.168.4.1 in browser to configure.
 */

#include "config_web.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_idf_version.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "config_web";

static httpd_handle_t s_server = NULL;
static bool s_running = false;

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static bool json_get_string(const char *json, const char *key, char *out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) {
        return false;
    }

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

    const char *p = strstr(json, pattern);
    if (!p) {
        return false;
    }

    p += strlen(pattern);
    const char *end = strchr(p, '"');
    if (!end) {
        return false;
    }

    size_t len = MIN((size_t)(end - p), out_size - 1);
    memcpy(out, p, len);
    out[len] = '\0';
    return true;
}

static bool json_get_checkbox_enabled(const char *json, const char *key)
{
    char value[8] = {0};
    if (!json_get_string(json, key, value, sizeof(value))) {
        return false;
    }

    return strcmp(value, "on") == 0;
}

// HTML configuration page (stored in flash)
static const char INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 NanoBot Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px}
.container{max-width:600px;margin:0 auto;background:#fff;border-radius:16px;box-shadow:0 10px 40px rgba(0,0,0,0.2);overflow:hidden}
.header{background:linear-gradient(135deg,#11998e,#38ef7d);padding:30px;text-align:center;color:#fff}
.header h1{font-size:24px;margin-bottom:10px}
.header p{opacity:0.9}
.content{padding:30px}
.section{margin-bottom:25px;padding:20px;background:#f8f9fa;border-radius:12px}
.section h2{font-size:18px;color:#333;margin-bottom:15px;display:flex;align-items:center}
.section h2::before{content:"";width:4px;height:20px;background:linear-gradient(135deg,#11998e,#38ef7d);margin-right:10px;border-radius:2px}
.form-group{margin-bottom:15px}
.form-group label{display:block;margin-bottom:5px;color:#555;font-weight:500}
.form-group input,.form-group select{width:100%;padding:12px;border:2px solid #e0e0e0;border-radius:8px;font-size:14px;transition:border-color 0.3s}
.form-group input:focus,.form-group select:focus{outline:none;border-color:#11998e}
.form-group small{color:#888;font-size:12px}
.btn{display:inline-block;width:100%;padding:15px;background:linear-gradient(135deg,#11998e,#38ef7d);color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;transition:transform 0.2s}
.btn:hover{transform:translateY(-2px)}
.btn:active{transform:translateY(0)}
.btn-secondary{background:#6c757d;margin-top:10px}
.status{padding:15px;background:#e8f5e9;border-radius:8px;margin-bottom:20px;text-align:center;color:#2e7d32}
.error{background:#ffebee;color:#c62828}
.tabs{display:flex;border-bottom:2px solid #e0e0e0;margin-bottom:20px}
.tab{flex:1;padding:12px;text-align:center;cursor:pointer;border-bottom:3px solid transparent;transition:all 0.3s}
.tab:hover{background:#f0f0f0}
.tab.active{border-bottom-color:#11998e;color:#11998e;font-weight:600}
.tab-content{display:none}
.tab-content.active{display:block}
</style>
</head>
<body>
<div class="container">
<div class="header"><h1>🤖 ESP32 NanoBot</h1><p>初始配置向导</p></div>
<div class="content">
<div id="status" class="status" style="display:none"></div>

<div class="tabs">
<div class="tab active" onclick="showTab('wifi')">📡 WiFi</div>
<div class="tab" onclick="showTab('ai')">🤖 AI</div>
<div class="tab" onclick="showTab('bot')">💬 机器人</div>
</div>

<form id="configForm">
<div class="tab-content active" id="wifi-tab">
<div class="section">
<h2>WiFi 设置</h2>
<div class="form-group">
<label>WiFi 名称 (SSID)</label>
<input type="text" name="wifi_ssid" required placeholder="请输入 WiFi 名称">
</div>
<div class="form-group">
<label>WiFi 密码</label>
<input type="password" name="wifi_password" required placeholder="请输入 WiFi 密码">
<small>⚠️ 仅支持 2.4GHz WiFi</small>
</div>
</div>
</div>

<div class="tab-content" id="ai-tab">
<div class="section">
<h2>AI 服务配置</h2>
<div class="form-group">
<label>AI 服务商</label>
<select name="ai_type" id="ai_type" onchange="updateAiUrl()">
<option value="deepseek">DeepSeek (深智 AI)</option>
<option value="moonshot">Moonshot (Kimi/月之暗面)</option>
<option value="dashscope">DashScope (通义千问)</option>
<option value="siliconflow">SiliconFlow (硅基流动)</option>
<option value="volcengine">VolcEngine (火山引擎)</option>
<option value="zhipu">Zhipu (智谱 AI)</option>
</select>
</div>
<div class="form-group">
<label>API Base URL</label>
<input type="url" name="ai_url" id="ai_url" value="https://api.deepseek.com">
</div>
<div class="form-group">
<label>API Key</label>
<input type="password" name="ai_key" required placeholder="请输入 API 密钥">
</div>
<div class="form-group">
<label>模型名称</label>
<input type="text" name="ai_model" id="ai_model" value="deepseek-chat">
<small>留空使用默认模型</small>
</div>
</div>
</div>

<div class="tab-content" id="bot-tab">
<div class="section">
<h2>QQ 机器人</h2>
<div class="form-group">
<label><input type="checkbox" name="qq_enabled" id="qq_enabled"> 启用 QQ 机器人</label>
</div>
<div class="form-group">
<label>App ID</label>
<input type="text" name="qq_id" placeholder="QQ App ID">
</div>
<div class="form-group">
<label>App Secret</label>
<input type="password" name="qq_secret" placeholder="QQ App Secret">
</div>
<div class="form-group">
<label>接收者 OpenID</label>
<input type="text" name="qq_oid" placeholder="消息接收者 OpenID (可选)">
</div>
</div>

<div class="section">
<h2>飞书机器人</h2>
<div class="form-group">
<label><input type="checkbox" name="fs_enabled" id="fs_enabled"> 启用飞书机器人</label>
</div>
<div class="form-group">
<label>App ID</label>
<input type="text" name="fs_id" placeholder="Feishu App ID">
</div>
<div class="form-group">
<label>App Secret</label>
<input type="password" name="fs_secret" placeholder="Feishu App Secret">
</div>
<div class="form-group">
<label>接收者 OpenID</label>
<input type="text" name="fs_oid" placeholder="消息接收者 OpenID (可选)">
</div>
</div>

<div class="section">
<h2>每日工作总结</h2>
<div class="form-group">
<label>发送时间</label>
<input type="time" name="summary_time" value="18:00">
</div>
<div class="form-group">
<label>发送渠道</label>
<select name="summary_channel">
<option value="qq">QQ</option>
<option value="feishu">飞书</option>
</select>
</div>
</div>
</div>

<button type="submit" class="btn">💾 保存配置并重启</button>
<button type="button" class="btn btn-secondary" onclick="scanWifi()">🔍 扫描 WiFi</button>
</form>
</div>
</div>

<script>
const AI_URLS={deepseek:"https://api.deepseek.com",moonshot:"https://api.moonshot.cn",dashscope:"https://dashscope.aliyuncs.com",siliconflow:"https://api.siliconflow.cn",volcengine:"https://ark.cn-beijing.volces.com",zhipu:"https://open.bigmodel.cn"};
const AI_MODELS={deepseek:"deepseek-chat",moonshot:"moonshot-v1-8k",dashscope:"qwen-plus",siliconflow:"Qwen/Qwen2.5-72B-Instruct",volcengine:"Doubao-pro-4k",zhipu:"glm-4"};
function showTab(t){document.querySelectorAll('.tab').forEach(e=>e.classList.remove('active'));document.querySelectorAll('.tab-content').forEach(e=>e.classList.remove('active'));document.querySelector('.tab[onclick="showTab(\''+t+'\')"]').classList.add('active');document.getElementById(t+'-tab').classList.add('active')}
function updateAiUrl(){const t=document.getElementById('ai_type').value;document.getElementById('ai_url').value=AI_URLS[t];document.getElementById('ai_model').value=AI_MODELS[t]}
async function scanWifi(){const s=document.getElementById('status');s.className='status';s.textContent='🔍 扫描中...';s.style.display='block';try{const r=await fetch('/scan'),d=await r.json();let m='📡 可用 WiFi:\n';d.forEach(w=>m+=`• ${w.ssid} (${w.rssi}dBm)\n`);s.textContent=m;s.classList.add('error')}catch(e){s.textContent='❌ 扫描失败：'+e.message;s.classList.add('error')}}
document.getElementById('configForm').onsubmit=async(e=>{e.preventDefault();const s=document.getElementById('status');s.className='status';s.textContent='⏳ 保存中...';s.style.display='block';const f=new FormData(e.target);const d={wifi_ssid:f.get('wifi_ssid'),wifi_password:f.get('wifi_password'),ai_type:f.get('ai_type'),ai_url:f.get('ai_url'),ai_key:f.get('ai_key'),ai_model:f.get('ai_model'),qq_enabled:f.get('qq_enabled')?'on':'',qq_id:f.get('qq_id'),qq_secret:f.get('qq_secret'),qq_oid:f.get('qq_oid'),fs_enabled:f.get('fs_enabled')?'on':'',fs_id:f.get('fs_id'),fs_secret:f.get('fs_secret'),fs_oid:f.get('fs_oid'),summary_time:f.get('summary_time'),summary_channel:f.get('summary_channel')};try{const r=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)});const t=await r.json();if(t.success){s.className='status';s.textContent='✅ 配置已保存！设备将在 3 秒后重启...';setTimeout(()=>window.location.href='/reboot',3000)}else{s.className='status error';s.textContent='❌ 保存失败：'+t.error}}catch(e){s.className='status error';s.textContent='❌ 保存失败：'+e.message}});
</script>
</body></html>
)rawliteral";

/**
 * @brief WiFi AP start
 */
static esp_err_t start_ap(void)
{
    // Create AP network interface
    esp_netif_create_default_wifi_ap();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure AP with hardcoded SSID and password
    wifi_config_t wifi_config = {0};  // Zero initialize first
    strncpy((char *)wifi_config.ap.ssid, "NanoBot-Setup", sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
    strncpy((char *)wifi_config.ap.password, "12345678", sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP started: SSID=%s, Password=%s", wifi_config.ap.ssid, wifi_config.ap.password);
    return ESP_OK;
}

/**
 * @brief HTTP GET handler for index page
 */
static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    return httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
}

/**
 * @brief HTTP GET handler for status
 */
static esp_err_t status_handler(httpd_req_t *req)
{
    device_config_t *config = config_manager_get_current();

    char json[512];
    snprintf(json, sizeof(json),
        "{\"wifi_configured\":%s,\"ai_configured\":%s,\"ai_type\":\"%s\",\"ai_model\":\"%s\"}",
        config->wifi_configured ? "true" : "false",
        config->ai_configured ? "true" : "false",
        config->ai_provider_type,
        config->ai_provider_model);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, strlen(json));
}

/**
 * @brief HTTP GET handler for WiFi scan
 */
static esp_err_t scan_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Starting WiFi scan...");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 200,
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    wifi_ap_record_t *ap_records = malloc(ap_count * sizeof(wifi_ap_record_t));
    if (!ap_records) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));

    // Build JSON response
    char *json = malloc(4096);
    if (!json) {
        free(ap_records);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    strcpy(json, "[");
    for (int i = 0; i < ap_count; i++) {
        if (i > 0) strcat(json, ",");
        char entry[256];
        snprintf(entry, sizeof(entry),
            "{\"ssid\":\"%s\",\"rssi\":%d,\"channel\":%d}",
            ap_records[i].ssid, ap_records[i].rssi, ap_records[i].primary);
        strcat(json, entry);
    }
    strcat(json, "]");

    free(ap_records);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);

    return ESP_OK;
}

/**
 * @brief HTTP POST handler for save configuration
 */
static esp_err_t save_handler(httpd_req_t *req)
{
    char buf[2048];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    ESP_LOGI(TAG, "Received config: %s", buf);

    device_config_t config = *config_manager_get_current();

    // Parse JSON fields
    if (json_get_string(buf, "wifi_ssid", config.wifi_ssid, sizeof(config.wifi_ssid))) {
        config.wifi_configured = strlen(config.wifi_ssid) > 0;
    }
    json_get_string(buf, "wifi_password", config.wifi_password, sizeof(config.wifi_password));

    if (json_get_string(buf, "ai_key", config.ai_provider_api_key, sizeof(config.ai_provider_api_key))) {
        config.ai_configured = strlen(config.ai_provider_api_key) > 0;
    }
    json_get_string(buf, "ai_type", config.ai_provider_type, sizeof(config.ai_provider_type));
    json_get_string(buf, "ai_url", config.ai_provider_base_url, sizeof(config.ai_provider_base_url));
    json_get_string(buf, "ai_model", config.ai_provider_model, sizeof(config.ai_provider_model));

    config.qq_bot_enabled = json_get_checkbox_enabled(buf, "qq_enabled");
    json_get_string(buf, "qq_id", config.qq_app_id, sizeof(config.qq_app_id));
    json_get_string(buf, "qq_secret", config.qq_app_secret, sizeof(config.qq_app_secret));
    json_get_string(buf, "qq_oid", config.qq_recipient_openid, sizeof(config.qq_recipient_openid));

    config.feishu_bot_enabled = json_get_checkbox_enabled(buf, "fs_enabled");
    json_get_string(buf, "fs_id", config.feishu_app_id, sizeof(config.feishu_app_id));
    json_get_string(buf, "fs_secret", config.feishu_app_secret, sizeof(config.feishu_app_secret));
    json_get_string(buf, "fs_oid", config.feishu_recipient_openid, sizeof(config.feishu_recipient_openid));

    json_get_string(buf, "summary_time", config.summary_send_time, sizeof(config.summary_send_time));
    json_get_string(buf, "summary_channel", config.summary_channel, sizeof(config.summary_channel));

    // Save configuration
    esp_err_t err = config_manager_save(&config);

    char response[64];
    if (err == ESP_OK) {
        snprintf(response, sizeof(response), "{\"success\":true}");
    } else {
        snprintf(response, sizeof(response), "{\"success\":false,\"error\":\"%s\"}", esp_err_to_name(err));
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response, strlen(response));
}

/**
 * @brief HTTP GET handler for reboot
 */
static esp_err_t reboot_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "Rebooting...", 12);

    ESP_LOGI(TAG, "Rebooting in 1 second...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

esp_err_t config_web_server_start(void)
{
    if (s_running) {
        return ESP_OK;
    }

    // Start WiFi AP
    start_ap();

    // Configure HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.stack_size = 8192;

    ESP_LOGI(TAG, "Starting web server on port 80...");

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(err));
        return err;
    }

    // Register handlers
    httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = index_handler};
    httpd_uri_t status_uri = {.uri = "/status", .method = HTTP_GET, .handler = status_handler};
    httpd_uri_t scan_uri = {.uri = "/scan", .method = HTTP_GET, .handler = scan_handler};
    httpd_uri_t save_uri = {.uri = "/save", .method = HTTP_POST, .handler = save_handler};
    httpd_uri_t reboot_uri = {.uri = "/reboot", .method = HTTP_GET, .handler = reboot_handler};

    httpd_register_uri_handler(s_server, &index_uri);
    httpd_register_uri_handler(s_server, &status_uri);
    httpd_register_uri_handler(s_server, &scan_uri);
    httpd_register_uri_handler(s_server, &save_uri);
    httpd_register_uri_handler(s_server, &reboot_uri);

    s_running = true;
    ESP_LOGI(TAG, "Web server started! Connect to http://192.168.4.1");

    return ESP_OK;
}

esp_err_t config_web_server_stop(void)
{
    if (!s_running) {
        return ESP_OK;
    }

    httpd_stop(s_server);
    s_server = NULL;
    s_running = false;

    esp_wifi_stop();

    ESP_LOGI(TAG, "Web server stopped");
    return ESP_OK;
}

bool config_web_server_is_running(void)
{
    return s_running;
}
