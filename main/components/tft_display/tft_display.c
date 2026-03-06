/**
 * @file tft_display.c
 * @brief ST7789 TFT LCD Driver Implementation
 */

#include "tft_display.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "tft_display";

// Pin definitions
#define TFT_PIN_CLK   GPIO_NUM_12
#define TFT_PIN_MOSI  GPIO_NUM_11
#define TFT_PIN_CS    GPIO_NUM_10
#define TFT_PIN_DC    GPIO_NUM_9
#define TFT_PIN_RST   GPIO_NUM_15
#define TFT_PIN_BLK   GPIO_NUM_14

// SPI configuration
#define TFT_SPI_HOST SPI2_HOST
#define TFT_SPI_CLOCK_SPEED_HZ (40 * 1000 * 1000)  // 40MHz

// ST7789 commands
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09
#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_NORON   0x13
#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36
#define ST7789_VSCSAD  0x37

// MADCTL bits
#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00

// SPI handle
static spi_device_handle_t s_spi_device = NULL;
static bool s_initialized = false;

// Font data (5x7 font, simplified)
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x30, 0x40, 0x40, 0x3F, 0x00}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x09, 0x19, 0x66}, // R
    {0x26, 0x49, 0x49, 0x49, 0x32}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x20, 0x1F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 0 (same as O for simplicity)
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 1 (same as I)
    {0x70, 0x48, 0x48, 0x48, 0x30}, // 2
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 3
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 4
    {0x18, 0x14, 0x14, 0x54, 0x78}, // 5
    {0x78, 0x54, 0x54, 0x54, 0x18}, // 6
    {0x44, 0x44, 0x44, 0x44, 0x38}, // 7
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 8
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 9
};

// Cursor position
static int16_t s_cursor_x = 0;
static int16_t s_cursor_y = 0;

/**
 * @brief Send command to display
 */
static void tft_send_command(uint8_t cmd)
{
    gpio_set_level(TFT_PIN_DC, 0);  // Command mode
    gpio_set_level(TFT_PIN_CS, 0);  // Select

    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TX_DATA,
        .length = 8,
        .tx_data[0] = cmd,
    };
    spi_device_transmit(s_spi_device, &t);

    gpio_set_level(TFT_PIN_CS, 1);  // Deselect
}

/**
 * @brief Send data to display
 */
static void tft_send_data(uint8_t data)
{
    gpio_set_level(TFT_PIN_DC, 1);  // Data mode
    gpio_set_level(TFT_PIN_CS, 0);  // Select

    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TX_DATA,
        .length = 8,
        .tx_data[0] = data,
    };
    spi_device_transmit(s_spi_device, &t);

    gpio_set_level(TFT_PIN_CS, 1);  // Deselect
}

/**
 * @brief Send multiple data bytes
 */
static void tft_send_data_multi(const uint8_t *data, int len)
{
    gpio_set_level(TFT_PIN_DC, 1);  // Data mode
    gpio_set_level(TFT_PIN_CS, 0);  // Select

    spi_transaction_t t = {
        .tx_buffer = data,
        .length = len * 8,
    };
    spi_device_transmit(s_spi_device, &t);

    gpio_set_level(TFT_PIN_CS, 1);  // Deselect
}

/**
 * @brief Set address window for RAM write
 */
static void tft_set_addr_window(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    uint8_t data[4];

    // Column address set
    tft_send_command(ST7789_CASET);
    data[0] = (x1 >> 8) & 0xFF;
    data[1] = x1 & 0xFF;
    data[2] = (x2 >> 8) & 0xFF;
    data[3] = x2 & 0xFF;
    tft_send_data_multi(data, 4);

    // Row address set
    tft_send_command(ST7789_RASET);
    data[0] = (y1 >> 8) & 0xFF;
    data[1] = y1 & 0xFF;
    data[2] = (y2 >> 8) & 0xFF;
    data[3] = y2 & 0xFF;
    tft_send_data_multi(data, 4);

    // RAM write
    tft_send_command(ST7789_RAMWR);
}

/**
 * @brief Initialize SPI
 */
static esp_err_t tft_spi_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = TFT_PIN_MOSI,
        .miso_io_num = -1,  // Not used
        .sclk_io_num = TFT_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32000,
    };

    esp_err_t ret = spi_bus_initialize(TFT_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = TFT_SPI_CLOCK_SPEED_HZ,
        .mode = 3,  // ST7789 uses SPI mode 3
        .spics_io_num = -1,  // Manual CS control
        .queue_size = 7,
    };

    ret = spi_bus_add_device(TFT_SPI_HOST, &dev_cfg, &s_spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(TFT_SPI_HOST);
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Initialize backlight PWM
 */
static void tft_backlight_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = TFT_PIN_BLK,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ledc_channel);
}

esp_err_t tft_display_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing TFT display...");

    // Initialize GPIO
    gpio_reset_pin(TFT_PIN_CS);
    gpio_reset_pin(TFT_PIN_DC);
    gpio_reset_pin(TFT_PIN_RST);
    gpio_reset_pin(TFT_PIN_BLK);

    gpio_set_direction(TFT_PIN_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(TFT_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(TFT_PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(TFT_PIN_BLK, GPIO_MODE_OUTPUT);

    gpio_set_level(TFT_PIN_CS, 1);
    gpio_set_level(TFT_PIN_DC, 0);
    gpio_set_level(TFT_PIN_RST, 1);
    gpio_set_level(TFT_PIN_BLK, 0);

    // Initialize SPI
    esp_err_t ret = tft_spi_init();
    if (ret != ESP_OK) {
        return ret;
    }

    // Hardware reset
    gpio_set_level(TFT_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TFT_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // ST7789 initialization sequence
    tft_send_command(ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    tft_send_command(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Memory data access control (rotation)
    tft_send_command(ST7789_MADCTL);
    tft_send_data(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);

    // Pixel format
    tft_send_command(ST7789_COLMOD);
    tft_send_data(0x05);  // 16-bit/pixel

    // Frame rate
    tft_send_command(0xB2);
    tft_send_data(0x0C);
    tft_send_data(0x0C);
    tft_send_data(0x00);
    tft_send_data(0x33);
    tft_send_data(0x33);

    // Gate control
    tft_send_command(0xB7);
    tft_send_data(0x72);

    // VCOM setting
    tft_send_command(0xBB);
    tft_send_data(0x3D);

    // LCM control
    tft_send_command(0xC0);
    tft_send_data(0x2C);

    // VRH set
    tft_send_command(0xC2);
    tft_send_data(0x01);
    tft_send_data(0xFF);

    // VDVR set
    tft_send_command(0xC3);
    tft_send_data(0x10);

    // VCI set
    tft_send_command(0xC4);
    tft_send_data(0x20);

    // VDV set
    tft_send_command(0xC6);
    tft_send_data(0x0F);

    // VCMOFSET
    tft_send_command(0xC8);
    tft_send_data(0x08);

    // FS option
    tft_send_command(0xD0);
    tft_send_data(0xA4);
    tft_send_data(0xA1);

    // Gamma
    tft_send_command(0xE0);
    tft_send_data(0xD0);
    tft_send_data(0x00);
    tft_send_data(0x02);
    tft_send_data(0x07);
    tft_send_data(0x0A);
    tft_send_data(0x28);
    tft_send_data(0x32);
    tft_send_data(0x44);
    tft_send_data(0x42);
    tft_send_data(0x06);
    tft_send_data(0x0E);
    tft_send_data(0x12);
    tft_send_data(0x14);
    tft_send_data(0x17);

    tft_send_command(0xE1);
    tft_send_data(0xD0);
    tft_send_data(0x00);
    tft_send_data(0x02);
    tft_send_data(0x07);
    tft_send_data(0x0A);
    tft_send_data(0x28);
    tft_send_data(0x31);
    tft_send_data(0x54);
    tft_send_data(0x47);
    tft_send_data(0x0E);
    tft_send_data(0x1C);
    tft_send_data(0x17);
    tft_send_data(0x1B);
    tft_send_data(0x1E);

    tft_send_command(ST7789_INVON);
    tft_send_command(ST7789_NORON);

    // Turn on display
    tft_send_command(ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Initialize backlight
    tft_backlight_init();
    tft_set_brightness(80);

    s_initialized = true;
    s_cursor_x = 0;
    s_cursor_y = 0;

    ESP_LOGI(TAG, "TFT display initialized successfully");
    return ESP_OK;
}

esp_err_t tft_display_deinit(void)
{
    if (!s_initialized) {
        return ESP_OK;
    }

    tft_set_brightness(0);

    if (s_spi_device) {
        spi_bus_remove_device(s_spi_device);
        spi_bus_free(TFT_SPI_HOST);
    }

    s_initialized = false;
    ESP_LOGI(TAG, "TFT display deinitialized");
    return ESP_OK;
}

void tft_fill_screen(uint16_t color)
{
    tft_set_addr_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);

    gpio_set_level(TFT_PIN_DC, 1);
    gpio_set_level(TFT_PIN_CS, 0);

    uint8_t color_bytes[2];
    color_bytes[0] = (color >> 8) & 0xFF;
    color_bytes[1] = color & 0xFF;

    for (int i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
        spi_transaction_t t = {
            .tx_buffer = color_bytes,
            .length = 16,
        };
        spi_device_transmit(s_spi_device, &t);
    }

    gpio_set_level(TFT_PIN_CS, 1);
}

void tft_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= TFT_WIDTH || y < 0 || y >= TFT_HEIGHT) {
        return;
    }

    tft_set_addr_window(x, y, x, y);

    gpio_set_level(TFT_PIN_DC, 1);
    gpio_set_level(TFT_PIN_CS, 0);

    uint8_t color_bytes[2];
    color_bytes[0] = (color >> 8) & 0xFF;
    color_bytes[1] = color & 0xFF;

    spi_transaction_t t = {
        .tx_buffer = color_bytes,
        .length = 16,
    };
    spi_device_transmit(s_spi_device, &t);

    gpio_set_level(TFT_PIN_CS, 1);
}

void tft_draw_hline(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if (y < 0 || y >= TFT_HEIGHT || x >= TFT_WIDTH || x + w <= 0) {
        return;
    }

    int16_t x2 = x + w - 1;
    if (x < 0) { x = 0; }
    if (x2 >= TFT_WIDTH) { x2 = TFT_WIDTH - 1; }

    tft_set_addr_window(x, y, x2, y);

    gpio_set_level(TFT_PIN_DC, 1);
    gpio_set_level(TFT_PIN_CS, 0);

    uint8_t color_bytes[2];
    color_bytes[0] = (color >> 8) & 0xFF;
    color_bytes[1] = color & 0xFF;

    for (int i = x; i <= x2; i++) {
        spi_transaction_t t = {
            .tx_buffer = color_bytes,
            .length = 16,
        };
        spi_device_transmit(s_spi_device, &t);
    }

    gpio_set_level(TFT_PIN_CS, 1);
}

void tft_draw_vline(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if (x < 0 || x >= TFT_WIDTH || y >= TFT_HEIGHT || y + h <= 0) {
        return;
    }

    int16_t y2 = y + h - 1;
    if (y < 0) { y = 0; }
    if (y2 >= TFT_HEIGHT) { y2 = TFT_HEIGHT - 1; }

    tft_set_addr_window(x, y, x, y2);

    gpio_set_level(TFT_PIN_DC, 1);
    gpio_set_level(TFT_PIN_CS, 0);

    uint8_t color_bytes[2];
    color_bytes[0] = (color >> 8) & 0xFF;
    color_bytes[1] = color & 0xFF;

    for (int i = y; i <= y2; i++) {
        spi_transaction_t t = {
            .tx_buffer = color_bytes,
            .length = 16,
        };
        spi_device_transmit(s_spi_device, &t);
    }

    gpio_set_level(TFT_PIN_CS, 1);
}

void tft_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    tft_draw_hline(x, y, w, color);
    tft_draw_hline(x, y + h - 1, w, color);
    tft_draw_vline(x, y, h, color);
    tft_draw_vline(x + w - 1, y, h, color);
}

void tft_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (x < 0 || y < 0 || x + w <= 0 || y + h <= 0) {
        return;
    }
    if (x + w > TFT_WIDTH) w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    tft_set_addr_window(x, y, x + w - 1, y + h - 1);

    gpio_set_level(TFT_PIN_DC, 1);
    gpio_set_level(TFT_PIN_CS, 0);

    uint8_t color_bytes[2];
    color_bytes[0] = (color >> 8) & 0xFF;
    color_bytes[1] = color & 0xFF;

    for (int i = 0; i < w * h; i++) {
        spi_transaction_t t = {
            .tx_buffer = color_bytes,
            .length = 16,
        };
        spi_device_transmit(s_spi_device, &t);
    }

    gpio_set_level(TFT_PIN_CS, 1);
}

void tft_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    tft_draw_pixel(x0, y0 + r, color);
    tft_draw_pixel(x0, y0 - r, color);
    tft_draw_pixel(x0 + r, y0, color);
    tft_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        tft_draw_pixel(x0 + x, y0 + y, color);
        tft_draw_pixel(x0 - x, y0 + y, color);
        tft_draw_pixel(x0 + x, y0 - y, color);
        tft_draw_pixel(x0 - x, y0 - y, color);
        tft_draw_pixel(x0 + y, y0 + x, color);
        tft_draw_pixel(x0 - y, y0 + x, color);
        tft_draw_pixel(x0 + y, y0 - x, color);
        tft_draw_pixel(x0 - y, y0 - x, color);
    }
}

void tft_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    tft_draw_vline(x0, y0 - r, 2 * r + 1, color);
    tft_fill_circle_helper(x0, y0, r, 3, 0, color);
}

void tft_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size)
{
    if (x > TFT_WIDTH - 1 || y > TFT_HEIGHT - 1 || c < ' ' || c > 'z') {
        return;
    }

    for (int8_t i = 0; i < 6; i++) {
        uint8_t line;
        if (i == 5) {
            line = 0x0;
        } else {
            line = font5x7[c - ' '][i];
        }

        for (int8_t j = 0; j < 8; j++) {
            if (line & 0x1) {
                if (size == 1) {
                    tft_draw_pixel(x + i, y + j, color);
                } else {
                    tft_fill_rect(x + (i * size), y + (j * size), size, size, color);
                }
            } else if (bg != color) {
                if (size == 1) {
                    tft_draw_pixel(x + i, y + j, bg);
                } else {
                    tft_fill_rect(x + i * size, y + j * size, size, size, bg);
                }
            }
            line >>= 1;
        }
    }
}

int16_t tft_draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size)
{
    int16_t start_x = x;
    while (*str) {
        if (*str == '\n') {
            x = start_x;
            y += 8 * size;
        } else {
            tft_draw_char(x, y, *str, color, bg, size);
            x += 6 * size;
        }
        str++;
    }
    return x - start_x;
}

void tft_set_brightness(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    uint32_t duty = (255 * brightness) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void tft_set_cursor(int16_t x, int16_t y)
{
    s_cursor_x = x;
    s_cursor_y = y;
}

void tft_print(const char *text, uint16_t color, uint8_t size)
{
    int16_t x = s_cursor_x;
    int16_t y = s_cursor_y;

    while (*text) {
        if (*text == '\n') {
            x = 0;
            y += 8 * size;
        } else {
            tft_draw_char(x, y, *text, color, TFT_COLOR_BLACK, size);
            x += 6 * size;
        }
        text++;
    }

    s_cursor_x = x;
    s_cursor_y = y;
}

void tft_print_centered(int16_t y, const char *text, uint16_t color, uint8_t size)
{
    int16_t len = strlen(text) * 6 * size;
    int16_t x = (TFT_WIDTH - len) / 2;
    if (x < 0) x = 0;
    tft_draw_string(x, y, text, color, TFT_COLOR_BLACK, size);
}

void tft_show_status(const char *title, const char *message, uint8_t icon)
{
    tft_fill_screen(TFT_COLOR_WHITE);

    // Draw title bar
    tft_fill_rect(0, 0, TFT_WIDTH, 50, TFT_COLOR_NAVY);
    tft_print_centered(15, title, TFT_COLOR_WHITE, 2);

    // Draw icon
    if (icon > 0) {
        uint16_t icon_color;
        char icon_char;
        switch (icon) {
            case 1: icon_color = TFT_COLOR_GREEN; icon_char = '✓'; break;
            case 2: icon_color = TFT_COLOR_YELLOW; icon_char = '!'; break;
            case 3: icon_color = TFT_COLOR_RED; icon_char = 'X'; break;
            default: icon_color = TFT_COLOR_BLUE; icon_char = 'i'; break;
        }
        tft_fill_circle(TFT_WIDTH / 2, 100, 30, icon_color);
        tft_draw_char(TFT_WIDTH / 2 - 3, 88, icon_char, TFT_COLOR_WHITE, TFT_COLOR_NAVY, 3);
    }

    // Draw message
    if (message) {
        tft_set_cursor(10, 150);
        tft_print(message, TFT_COLOR_BLACK, 2);
    }
}

void tft_show_wifi_status(bool connected, const char *ip_address)
{
    tft_fill_screen(TFT_COLOR_WHITE);

    // Title
    tft_fill_rect(0, 0, TFT_WIDTH, 50, TFT_COLOR_NAVY);
    tft_print_centered(15, "WiFi Status", TFT_COLOR_WHITE, 2);

    // Status
    if (connected) {
        tft_fill_circle(TFT_WIDTH / 2, 120, 40, TFT_COLOR_GREEN);
        tft_print_centered(108, "Connected", TFT_COLOR_GREEN, 2);
        if (ip_address) {
            tft_print_centered(150, ip_address, TFT_COLOR_BLACK, 2);
        }
    } else {
        tft_fill_circle(TFT_WIDTH / 2, 120, 40, TFT_COLOR_RED);
        tft_draw_char(TFT_WIDTH / 2 - 5, 108, 'X', TFT_COLOR_WHITE, TFT_COLOR_RED, 3);
        tft_print_centered(150, "Disconnected", TFT_COLOR_RED, 2);
    }
}

void tft_show_ai_status(bool ready)
{
    tft_fill_screen(TFT_COLOR_WHITE);

    // Title
    tft_fill_rect(0, 0, TFT_WIDTH, 50, TFT_COLOR_NAVY);
    tft_print_centered(15, "AI Status", TFT_COLOR_WHITE, 2);

    // Status
    if (ready) {
        tft_fill_circle(TFT_WIDTH / 2, 120, 40, TFT_COLOR_GREEN);
        tft_print_centered(108, "Ready", TFT_COLOR_GREEN, 2);
        tft_print_centered(180, "AI Provider OK", TFT_COLOR_BLACK, 1);
    } else {
        tft_fill_circle(TFT_WIDTH / 2, 120, 40, TFT_COLOR_RED);
        tft_draw_char(TFT_WIDTH / 2 - 5, 108, 'X', TFT_COLOR_WHITE, TFT_COLOR_RED, 3);
        tft_print_centered(150, "Not Configured", TFT_COLOR_RED, 2);
        tft_print_centered(200, "Type 'ai' to setup", TFT_COLOR_GRAY, 1);
    }
}
