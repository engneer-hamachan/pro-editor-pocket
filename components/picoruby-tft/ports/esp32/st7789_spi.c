/*
 * ST7789 SPI Driver for ESP-IDF
 */

#include "st7789_spi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "ST7789";

// SPI handle
static spi_device_handle_t spi_handle = NULL;

// Display state
static int16_t _width = ST7789_WIDTH;
static int16_t _height = ST7789_HEIGHT;
static uint8_t _rotation = 0;
static int16_t _cursor_x = 0;
static int16_t _cursor_y = 0;
static uint16_t _text_color = COLOR_WHITE;
static uint8_t _text_size = 1;
static bool _text_wrap = true;

// Pre/post transaction callbacks for DC pin
static void IRAM_ATTR spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(TDECK_TFT_DC, dc);
}

// Send command (DC=0)
static void st7789_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .user = (void*)0,  // DC = 0 for command
    };
    esp_err_t ret = spi_device_polling_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send command 0x%02x", cmd);
    }
}

// Send data (DC=1)
static void st7789_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
        .user = (void*)1,  // DC = 1 for data
    };
    esp_err_t ret = spi_device_polling_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send data");
    }
}

// Send single byte data
static void st7789_data8(uint8_t data)
{
    st7789_data(&data, 1);
}

// Set address window for drawing
static void st7789_set_addr_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t data[4];

    st7789_cmd(ST7789_CASET);
    data[0] = (x0 >> 8) & 0xFF;
    data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF;
    data[3] = x1 & 0xFF;
    st7789_data(data, 4);

    st7789_cmd(ST7789_RASET);
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    st7789_data(data, 4);

    st7789_cmd(ST7789_RAMWR);
}

bool st7789_init(void)
{
    // Already initialized - just re-add SPI device if needed
    if (spi_handle != NULL) {
        ESP_LOGI(TAG, "ST7789 already initialized, re-adding SPI device");
        spi_bus_remove_device(spi_handle);
        spi_handle = NULL;
    }

    ESP_LOGI(TAG, "Initializing ST7789 display...");

    // Initialize power pin
    gpio_config_t pwr_conf = {
        .pin_bit_mask = (1ULL << TDECK_POWERON),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pwr_conf);
    gpio_set_level(TDECK_POWERON, 1);
    ESP_LOGI(TAG, "Power ON");

    // Initialize CS pins for all SPI devices (keep high)
    gpio_config_t cs_conf = {
        .pin_bit_mask = (1ULL << TDECK_TFT_CS) | (1ULL << TDECK_SDCARD_CS) | (1ULL << TDECK_RADIO_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cs_conf);
    gpio_set_level(TDECK_SDCARD_CS, 1);
    gpio_set_level(TDECK_RADIO_CS, 1);
    gpio_set_level(TDECK_TFT_CS, 1);

    // Initialize DC pin
    gpio_config_t dc_conf = {
        .pin_bit_mask = (1ULL << TDECK_TFT_DC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&dc_conf);
    gpio_set_level(TDECK_TFT_DC, 1);

    // Initialize backlight pin
    gpio_config_t bl_conf = {
        .pin_bit_mask = (1ULL << TDECK_TFT_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl_conf);
    gpio_set_level(TDECK_TFT_BL, 0);  // Start with backlight off

    // Configure SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = TDECK_TFT_MOSI,
        .miso_io_num = TDECK_TFT_MISO,
        .sclk_io_num = TDECK_TFT_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ST7789_WIDTH * ST7789_HEIGHT * 2,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret == ESP_ERR_INVALID_STATE) {
        // SPI bus already initialized, this is OK
        ESP_LOGI(TAG, "SPI bus already initialized");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    } else {
        ESP_LOGI(TAG, "SPI bus initialized");
    }

    // Configure SPI device
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 40 * 1000 * 1000,  // 40 MHz
        .mode = 0,
        .spics_io_num = TDECK_TFT_CS,
        .queue_size = 7,
        .pre_cb = spi_pre_transfer_callback,
    };

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(TAG, "SPI device added");

    // Software reset
    st7789_cmd(ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Exit sleep mode
    st7789_cmd(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Set color mode to 16-bit (RGB565)
    st7789_cmd(ST7789_COLMOD);
    st7789_data8(0x55);  // 16-bit color
    vTaskDelay(pdMS_TO_TICKS(10));

    // Memory Data Access Control (rotation)
    st7789_cmd(ST7789_MADCTL);
    st7789_data8(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);  // Rotation 1 (landscape)

    // Inversion on (for T-Deck display)
    st7789_cmd(ST7789_INVON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Normal display mode
    st7789_cmd(ST7789_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Display on
    st7789_cmd(ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set initial rotation (landscape)
    _rotation = 1;
    _width = ST7789_HEIGHT;  // 320
    _height = ST7789_WIDTH;  // 240

    ESP_LOGI(TAG, "ST7789 initialized successfully (%dx%d)", _width, _height);

    // Turn on backlight
    st7789_set_backlight(16);

    return true;
}

void st7789_fill_screen(uint16_t color)
{
    st7789_fill_rect(0, 0, _width, _height, color);
}

void st7789_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;

    st7789_set_addr_window(x, y, x, y);
    uint8_t data[2] = { (color >> 8) & 0xFF, color & 0xFF };
    st7789_data(data, 2);
}

void st7789_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (x >= _width || y >= _height || w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;

    st7789_set_addr_window(x, y, x + w - 1, y + h - 1);

    uint32_t total_pixels = w * h;
    uint8_t color_hi = (color >> 8) & 0xFF;
    uint8_t color_lo = color & 0xFF;

    // Use a buffer for faster transfer
    #define FILL_BUF_SIZE 512
    uint8_t buf[FILL_BUF_SIZE];
    for (int i = 0; i < FILL_BUF_SIZE; i += 2) {
        buf[i] = color_hi;
        buf[i + 1] = color_lo;
    }

    uint32_t bytes_remaining = total_pixels * 2;
    while (bytes_remaining > 0) {
        uint32_t chunk = (bytes_remaining > FILL_BUF_SIZE) ? FILL_BUF_SIZE : bytes_remaining;
        st7789_data(buf, chunk);
        bytes_remaining -= chunk;
    }
}

void st7789_set_rotation(uint8_t rotation)
{
    _rotation = rotation % 4;
    uint8_t madctl = 0;

    switch (_rotation) {
        case 0:
            madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB;
            _width = ST7789_WIDTH;
            _height = ST7789_HEIGHT;
            break;
        case 1:
            madctl = ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB;
            _width = ST7789_HEIGHT;
            _height = ST7789_WIDTH;
            break;
        case 2:
            madctl = ST7789_MADCTL_RGB;
            _width = ST7789_WIDTH;
            _height = ST7789_HEIGHT;
            break;
        case 3:
            madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB;
            _width = ST7789_HEIGHT;
            _height = ST7789_WIDTH;
            break;
    }

    st7789_cmd(ST7789_MADCTL);
    st7789_data8(madctl);
}

int16_t st7789_width(void)
{
    return _width;
}

int16_t st7789_height(void)
{
    return _height;
}

uint8_t st7789_get_rotation(void)
{
    return _rotation;
}

void st7789_set_cursor(int16_t x, int16_t y)
{
    _cursor_x = x;
    _cursor_y = y;
}

void st7789_set_text_color(uint16_t color)
{
    _text_color = color;
}

// Simple 5x7 font (ASCII 32-127)
static const uint8_t font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // 32 space
    0x00, 0x00, 0x5F, 0x00, 0x00, // 33 !
    0x00, 0x07, 0x00, 0x07, 0x00, // 34 "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // 35 #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // 36 $
    0x23, 0x13, 0x08, 0x64, 0x62, // 37 %
    0x36, 0x49, 0x55, 0x22, 0x50, // 38 &
    0x00, 0x05, 0x03, 0x00, 0x00, // 39 '
    0x00, 0x1C, 0x22, 0x41, 0x00, // 40 (
    0x00, 0x41, 0x22, 0x1C, 0x00, // 41 )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // 42 *
    0x08, 0x08, 0x3E, 0x08, 0x08, // 43 +
    0x00, 0x50, 0x30, 0x00, 0x00, // 44 ,
    0x08, 0x08, 0x08, 0x08, 0x08, // 45 -
    0x00, 0x60, 0x60, 0x00, 0x00, // 46 .
    0x20, 0x10, 0x08, 0x04, 0x02, // 47 /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 48 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 49 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 50 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 51 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 52 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 53 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 54 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 55 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 56 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 57 9
    0x00, 0x36, 0x36, 0x00, 0x00, // 58 :
    0x00, 0x56, 0x36, 0x00, 0x00, // 59 ;
    0x08, 0x14, 0x22, 0x41, 0x00, // 60 <
    0x14, 0x14, 0x14, 0x14, 0x14, // 61 =
    0x00, 0x41, 0x22, 0x14, 0x08, // 62 >
    0x02, 0x01, 0x51, 0x09, 0x06, // 63 ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // 64 @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // 65 A
    0x7F, 0x49, 0x49, 0x49, 0x36, // 66 B
    0x3E, 0x41, 0x41, 0x41, 0x22, // 67 C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // 68 D
    0x7F, 0x49, 0x49, 0x49, 0x41, // 69 E
    0x7F, 0x09, 0x09, 0x09, 0x01, // 70 F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // 71 G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // 72 H
    0x00, 0x41, 0x7F, 0x41, 0x00, // 73 I
    0x20, 0x40, 0x41, 0x3F, 0x01, // 74 J
    0x7F, 0x08, 0x14, 0x22, 0x41, // 75 K
    0x7F, 0x40, 0x40, 0x40, 0x40, // 76 L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // 77 M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // 78 N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // 79 O
    0x7F, 0x09, 0x09, 0x09, 0x06, // 80 P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // 81 Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // 82 R
    0x46, 0x49, 0x49, 0x49, 0x31, // 83 S
    0x01, 0x01, 0x7F, 0x01, 0x01, // 84 T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // 85 U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // 86 V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // 87 W
    0x63, 0x14, 0x08, 0x14, 0x63, // 88 X
    0x07, 0x08, 0x70, 0x08, 0x07, // 89 Y
    0x61, 0x51, 0x49, 0x45, 0x43, // 90 Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // 91 [
    0x02, 0x04, 0x08, 0x10, 0x20, // 92 backslash
    0x00, 0x41, 0x41, 0x7F, 0x00, // 93 ]
    0x04, 0x02, 0x01, 0x02, 0x04, // 94 ^
    0x40, 0x40, 0x40, 0x40, 0x40, // 95 _
    0x00, 0x01, 0x02, 0x04, 0x00, // 96 `
    0x20, 0x54, 0x54, 0x54, 0x78, // 97 a
    0x7F, 0x48, 0x44, 0x44, 0x38, // 98 b
    0x38, 0x44, 0x44, 0x44, 0x20, // 99 c
    0x38, 0x44, 0x44, 0x48, 0x7F, // 100 d
    0x38, 0x54, 0x54, 0x54, 0x18, // 101 e
    0x08, 0x7E, 0x09, 0x01, 0x02, // 102 f
    0x0C, 0x52, 0x52, 0x52, 0x3E, // 103 g
    0x7F, 0x08, 0x04, 0x04, 0x78, // 104 h
    0x00, 0x44, 0x7D, 0x40, 0x00, // 105 i
    0x20, 0x40, 0x44, 0x3D, 0x00, // 106 j
    0x7F, 0x10, 0x28, 0x44, 0x00, // 107 k
    0x00, 0x41, 0x7F, 0x40, 0x00, // 108 l
    0x7C, 0x04, 0x18, 0x04, 0x78, // 109 m
    0x7C, 0x08, 0x04, 0x04, 0x78, // 110 n
    0x38, 0x44, 0x44, 0x44, 0x38, // 111 o
    0x7C, 0x14, 0x14, 0x14, 0x08, // 112 p
    0x08, 0x14, 0x14, 0x18, 0x7C, // 113 q
    0x7C, 0x08, 0x04, 0x04, 0x08, // 114 r
    0x48, 0x54, 0x54, 0x54, 0x20, // 115 s
    0x04, 0x3F, 0x44, 0x40, 0x20, // 116 t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // 117 u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // 118 v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // 119 w
    0x44, 0x28, 0x10, 0x28, 0x44, // 120 x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // 121 y
    0x44, 0x64, 0x54, 0x4C, 0x44, // 122 z
    0x00, 0x08, 0x36, 0x41, 0x00, // 123 {
    0x00, 0x00, 0x7F, 0x00, 0x00, // 124 |
    0x00, 0x41, 0x36, 0x08, 0x00, // 125 }
    0x10, 0x08, 0x08, 0x10, 0x08, // 126 ~
};

static void st7789_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint8_t size)
{
    if (c < ' ' || c > '~') return;
    int idx = (c - ' ') * 5;

    for (int i = 0; i < 5; i++) {
        uint8_t line = font5x7[idx + i];
        for (int j = 0; j < 7; j++) {
            if (line & (1 << j)) {
                if (size == 1) {
                    st7789_draw_pixel(x + i, y + j, color);
                } else {
                    st7789_fill_rect(x + i * size, y + j * size, size, size, color);
                }
            }
        }
    }
}

void st7789_set_text_size(uint8_t size)
{
    _text_size = (size > 0) ? size : 1;
}

uint8_t st7789_get_text_size(void)
{
    return _text_size;
}

void st7789_print(const char* text)
{
    int16_t char_width = 6 * _text_size;
    int16_t char_height = 8 * _text_size;

    while (*text) {
        st7789_draw_char(_cursor_x, _cursor_y, *text, _text_color, _text_size);
        _cursor_x += char_width;
        if (_text_wrap && _cursor_x + (5 * _text_size) > _width) {
            _cursor_x = 0;
            _cursor_y += char_height;
        }
        text++;
    }
}

void st7789_set_text_wrap(bool wrap)
{
    _text_wrap = wrap;
}

bool st7789_get_text_wrap(void)
{
    return _text_wrap;
}

uint16_t rgb888_to_rgb565(uint32_t rgb888)
{
    uint8_t r = (rgb888 >> 16) & 0xFF;
    uint8_t g = (rgb888 >> 8) & 0xFF;
    uint8_t b = rgb888 & 0xFF;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void st7789_set_backlight(uint8_t level)
{
    // T-Deck backlight has 16 levels (0-15)
    // Simple on/off for now
    if (level > 0) {
        gpio_set_level(TDECK_TFT_BL, 1);
    } else {
        gpio_set_level(TDECK_TFT_BL, 0);
    }
}

// Fast horizontal line (optimized fill_rect)
void st7789_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if (y < 0 || y >= _height || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > _width) w = _width - x;
    if (w <= 0) return;

    st7789_set_addr_window(x, y, x + w - 1, y);

    uint8_t color_hi = (color >> 8) & 0xFF;
    uint8_t color_lo = color & 0xFF;
    uint8_t buf[64];
    for (int i = 0; i < 64; i += 2) {
        buf[i] = color_hi;
        buf[i + 1] = color_lo;
    }

    int remaining = w;
    while (remaining > 0) {
        int chunk = (remaining > 32) ? 32 : remaining;
        st7789_data(buf, chunk * 2);
        remaining -= chunk;
    }
}

// Fast vertical line (optimized fill_rect)
void st7789_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if (x < 0 || x >= _width || h <= 0) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > _height) h = _height - y;
    if (h <= 0) return;

    st7789_set_addr_window(x, y, x, y + h - 1);

    uint8_t color_hi = (color >> 8) & 0xFF;
    uint8_t color_lo = color & 0xFF;
    uint8_t buf[64];
    for (int i = 0; i < 64; i += 2) {
        buf[i] = color_hi;
        buf[i + 1] = color_lo;
    }

    int remaining = h;
    while (remaining > 0) {
        int chunk = (remaining > 32) ? 32 : remaining;
        st7789_data(buf, chunk * 2);
        remaining -= chunk;
    }
}

// Rectangle outline
void st7789_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    st7789_draw_fast_h_line(x, y, w, color);           // Top
    st7789_draw_fast_h_line(x, y + h - 1, w, color);   // Bottom
    st7789_draw_fast_v_line(x, y, h, color);           // Left
    st7789_draw_fast_v_line(x + w - 1, y, h, color);   // Right
}

// Helper function for drawing circle corners (used by rounded rect)
static void st7789_draw_circle_helper(int16_t x0, int16_t y0, int16_t r,
                                       uint8_t cornername, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
            st7789_draw_pixel(x0 + x, y0 + y, color);
            st7789_draw_pixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            st7789_draw_pixel(x0 + x, y0 - y, color);
            st7789_draw_pixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            st7789_draw_pixel(x0 - y, y0 + x, color);
            st7789_draw_pixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            st7789_draw_pixel(x0 - y, y0 - x, color);
            st7789_draw_pixel(x0 - x, y0 - y, color);
        }
    }
}

// Rounded rectangle outline
void st7789_draw_round_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                            int16_t r, uint16_t color)
{
    // Draw straight lines
    st7789_draw_fast_h_line(x + r, y, w - 2 * r, color);         // Top
    st7789_draw_fast_h_line(x + r, y + h - 1, w - 2 * r, color); // Bottom
    st7789_draw_fast_v_line(x, y + r, h - 2 * r, color);         // Left
    st7789_draw_fast_v_line(x + w - 1, y + r, h - 2 * r, color); // Right
    // Draw corners
    st7789_draw_circle_helper(x + r, y + r, r, 1, color);
    st7789_draw_circle_helper(x + w - r - 1, y + r, r, 2, color);
    st7789_draw_circle_helper(x + w - r - 1, y + h - r - 1, r, 4, color);
    st7789_draw_circle_helper(x + r, y + h - r - 1, r, 8, color);
}

// Helper function for filling circle corners (used by rounded rect fill)
static void st7789_fill_circle_helper(int16_t x0, int16_t y0, int16_t r,
                                       uint8_t corners, int16_t delta, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;

    delta++;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (x < (y + 1)) {
            if (corners & 1)
                st7789_draw_fast_v_line(x0 + x, y0 - y, 2 * y + delta, color);
            if (corners & 2)
                st7789_draw_fast_v_line(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (corners & 1)
                st7789_draw_fast_v_line(x0 + py, y0 - px, 2 * px + delta, color);
            if (corners & 2)
                st7789_draw_fast_v_line(x0 - py, y0 - px, 2 * px + delta, color);
            py = y;
        }
        px = x;
    }
}

// Rounded rectangle filled
void st7789_fill_round_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                            int16_t r, uint16_t color)
{
    // Center rectangle
    st7789_fill_rect(x + r, y, w - 2 * r, h, color);
    // Corner fills
    st7789_fill_circle_helper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    st7789_fill_circle_helper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}
