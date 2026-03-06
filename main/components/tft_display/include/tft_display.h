/**
 * @file tft_display.h
 * @brief TFT LCD Display Driver for 1.69 inch 8-pin ST7789 Display
 *
 * Pin connection:
 *   VCC  -> 3.3V
 *   GND  -> GND
 *   CLK  -> GPIO12 (SPI SCK)
 *   SDA  -> GPIO11 (SPI MOSI)
 *   CS   -> GPIO10 (SPI CS)
 *   DC   -> GPIO9  (Data/Command)
 *   RES  -> GPIO15 (Reset)
 *   BLK  -> GPIO14 (Backlight PWM)
 */

#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Display dimensions
#define TFT_WIDTH  240
#define TFT_HEIGHT 280

// Color definitions (RGB565 format)
#define TFT_COLOR_BLACK       0x0000
#define TFT_COLOR_WHITE       0xFFFF
#define TFT_COLOR_RED         0xF800
#define TFT_COLOR_GREEN       0x07E0
#define TFT_COLOR_BLUE        0x001F
#define TFT_COLOR_YELLOW      0xFFE0
#define TFT_COLOR_CYAN        0x07FF
#define TFT_COLOR_MAGENTA     0xF81F
#define TFT_COLOR_GRAY        0x8410
#define TFT_COLOR_DARKGRAY    0x4208
#define TFT_COLOR_NAVY        0x000F
#define TFT_COLOR_ORANGE      0xFD20
#define TFT_COLOR_PURPLE      0x780F

/**
 * @brief Initialize TFT display
 * @return ESP_OK on success
 */
esp_err_t tft_display_init(void);

/**
 * @brief Deinitialize TFT display
 * @return ESP_OK on success
 */
esp_err_t tft_display_deinit(void);

/**
 * @brief Clear screen with color
 * @param color RGB565 color
 */
void tft_fill_screen(uint16_t color);

/**
 * @brief Draw a pixel at specified position
 * @param x X coordinate
 * @param y Y coordinate
 * @param color RGB565 color
 */
void tft_draw_pixel(int16_t x, int16_t y, uint16_t color);

/**
 * @brief Draw a horizontal line
 * @param x Start X coordinate
 * @param y Y coordinate
 * @param w Line width
 * @param color RGB565 color
 */
void tft_draw_hline(int16_t x, int16_t y, int16_t w, uint16_t color);

/**
 * @brief Draw a vertical line
 * @param x X coordinate
 * @param y Start Y coordinate
 * @param h Line height
 * @param color RGB565 color
 */
void tft_draw_vline(int16_t x, int16_t y, int16_t h, uint16_t color);

/**
 * @brief Draw a rectangle outline
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param w Rectangle width
 * @param h Rectangle height
 * @param color RGB565 color
 */
void tft_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief Draw a filled rectangle
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param w Rectangle width
 * @param h Rectangle height
 * @param color RGB565 color
 */
void tft_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief Draw a circle
 * @param x0 Center X coordinate
 * @param y0 Center Y coordinate
 * @param r Radius
 * @param color RGB565 color
 */
void tft_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/**
 * @brief Draw a filled circle
 * @param x0 Center X coordinate
 * @param y0 Center Y coordinate
 * @param r Radius
 * @param color RGB565 color
 */
void tft_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/**
 * @brief Draw a character
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 * @param color Foreground color
 * @param bg Background color
 * @param size Character size multiplier
 */
void tft_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief Draw a string
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param color Foreground color
 * @param bg Background color
 * @param size Character size multiplier
 * @return Width of the string
 */
int16_t tft_draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief Draw an icon (bitmap)
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Icon width
 * @param height Icon height
 * @param data Icon data (RGB565 format)
 */
void tft_draw_icon(int16_t x, int16_t y, int16_t width, int16_t height, const uint16_t *data);

/**
 * @brief Set backlight brightness
 * @param brightness 0-100
 */
void tft_set_brightness(uint8_t brightness);

/**
 * @brief Set cursor position for text output
 * @param x X coordinate
 * @param y Y coordinate
 */
void tft_set_cursor(int16_t x, int16_t y);

/**
 * @brief Print text at cursor position
 * @param text Text to print
 * @param color Foreground color
 * @param size Character size multiplier
 */
void tft_print(const char *text, uint16_t color, uint8_t size);

/**
 * @brief Print text centered horizontally
 * @param y Y coordinate
 * @param text Text to print
 * @param color Foreground color
 * @param size Character size multiplier
 */
void tft_print_centered(int16_t y, const char *text, uint16_t color, uint8_t size);

/**
 * @brief Display status message
 * @param title Title text
 * @param message Message text
 * @param icon Icon type (0:none, 1:check, 2:warning, 3:error)
 */
void tft_show_status(const char *title, const char *message, uint8_t icon);

/**
 * @brief Display WiFi connection status
 * @param connected true if connected
 * @param ip_address IP address string
 */
void tft_show_wifi_status(bool connected, const char *ip_address);

/**
 * @brief Display AI provider status
 * @param ready true if AI provider is ready
 */
void tft_show_ai_status(bool ready);

#ifdef __cplusplus
}
#endif

#endif // TFT_DISPLAY_H
