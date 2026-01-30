/*
 * ST7789 SPI Driver for ESP-IDF (No TFT_eSPI dependency)
 * Direct SPI control using ESP-IDF API
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// T-Deck pin definitions
#define TDECK_TFT_CS    12
#define TDECK_TFT_DC    11
#define TDECK_TFT_MOSI  41
#define TDECK_TFT_MISO  38
#define TDECK_TFT_SCLK  40
#define TDECK_TFT_BL    42
#define TDECK_POWERON   10

// Other SPI devices CS pins (keep high)
#define TDECK_SDCARD_CS 39
#define TDECK_RADIO_CS  9

// Display dimensions
#define ST7789_WIDTH    240
#define ST7789_HEIGHT   320

// ST7789 Commands
#define ST7789_NOP      0x00
#define ST7789_SWRESET  0x01
#define ST7789_SLPIN    0x10
#define ST7789_SLPOUT   0x11
#define ST7789_PTLON    0x12
#define ST7789_NORON    0x13
#define ST7789_INVOFF   0x20
#define ST7789_INVON    0x21
#define ST7789_DISPOFF  0x28
#define ST7789_DISPON   0x29
#define ST7789_CASET    0x2A
#define ST7789_RASET    0x2B
#define ST7789_RAMWR    0x2C
#define ST7789_RAMRD    0x2E
#define ST7789_PTLAR    0x30
#define ST7789_COLMOD   0x3A
#define ST7789_MADCTL   0x36

// MADCTL flags
#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00
#define ST7789_MADCTL_BGR 0x08

// Color definitions (RGB565)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F

// Initialize ST7789 display
bool st7789_init(void);

// Basic drawing functions
void st7789_fill_screen(uint16_t color);
void st7789_draw_pixel(int16_t x, int16_t y, uint16_t color);
void st7789_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void st7789_set_rotation(uint8_t rotation);

// Display properties
int16_t st7789_width(void);
int16_t st7789_height(void);
uint8_t st7789_get_rotation(void);

// Text functions (basic)
void st7789_set_cursor(int16_t x, int16_t y);
void st7789_set_text_color(uint16_t color);
void st7789_set_text_size(uint8_t size);
uint8_t st7789_get_text_size(void);
void st7789_set_text_wrap(bool wrap);
bool st7789_get_text_wrap(void);
void st7789_print(const char* text);

// Color conversion
uint16_t rgb888_to_rgb565(uint32_t rgb888);

// Backlight control
void st7789_set_backlight(uint8_t level);

// Line drawing functions
void st7789_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint16_t color);
void st7789_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint16_t color);

// Rectangle functions
void st7789_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void st7789_draw_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void st7789_fill_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);

#ifdef __cplusplus
}
#endif
