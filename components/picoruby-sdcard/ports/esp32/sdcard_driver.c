#include "sdcard_driver.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"

static const char *TAG = "SDCard";

#define CODE_FILE_START_SECTOR 1024  // Start writing from sector 1024
#define MAX_CODE_SIZE (64 * 1024)    // Max 64KB code

// Set other SPI devices CS to HIGH before SD operation
static void prepare_spi_for_sd(void)
{
    gpio_set_level(TFT_CS_PIN, 1);
    gpio_set_level(RADIO_CS_PIN, 1);
}

// Initialize SD card for single operation, returns card handle
static sdmmc_card_t* sdcard_begin(sdspi_dev_handle_t *out_handle)
{
    ESP_LOGI(TAG, "Opening SD card...");

    // Set CS pins as output and HIGH
    gpio_set_direction(TFT_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RADIO_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(SDCARD_CS_PIN, GPIO_MODE_OUTPUT);
    prepare_spi_for_sd();

    // Allocate card structure
    sdmmc_card_t *card = (sdmmc_card_t *)malloc(sizeof(sdmmc_card_t));
    if (card == NULL) {
        ESP_LOGE(TAG, "Failed to allocate card structure");
        return NULL;
    }

    // Configure SD SPI device
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SDCARD_CS_PIN;
    slot_config.host_id = SPI2_HOST;

    // Initialize SD SPI device (bus already initialized by TFT)
    sdspi_dev_handle_t handle;
    esp_err_t ret = sdspi_host_init_device(&slot_config, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init SD SPI device: %s", esp_err_to_name(ret));
        free(card);
        return NULL;
    }

    // Configure host
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    // Initialize card
    ret = sdmmc_card_init(&host, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
        sdspi_host_remove_device(handle);
        free(card);
        return NULL;
    }

    ESP_LOGI(TAG, "SD card opened successfully");
    *out_handle = handle;
    return card;
}

// Close SD card after operation
static void sdcard_end(sdmmc_card_t *card, sdspi_dev_handle_t handle)
{
    if (card != NULL) {
        free(card);
    }
    sdspi_host_remove_device(handle);

    // Restore CS pins
    gpio_set_level(SDCARD_CS_PIN, 1);
    gpio_set_level(TFT_CS_PIN, 1);

    ESP_LOGI(TAG, "SD card closed");
}

bool sdcard_init(void)
{
    // Just check if we can open the card
    sdspi_dev_handle_t handle;
    sdmmc_card_t *card = sdcard_begin(&handle);
    if (card == NULL) {
        return false;
    }
    sdmmc_card_print_info(stdout, card);
    sdcard_end(card, handle);
    return true;
}

bool sdcard_write_file(const char *path, const char *data)
{
    sdspi_dev_handle_t handle;
    sdmmc_card_t *card = sdcard_begin(&handle);
    if (card == NULL) {
        return false;
    }

    size_t data_len = strlen(data);
    if (data_len > MAX_CODE_SIZE - 4) {
        ESP_LOGE(TAG, "Data too large: %zu bytes (max %d)", data_len, MAX_CODE_SIZE - 4);
        sdcard_end(card, handle);
        return false;
    }

    // Calculate sectors needed (512 bytes per sector)
    size_t total_size = data_len + 4;  // 4 bytes for length header
    size_t sectors_needed = (total_size + 511) / 512;

    // Allocate buffer aligned to sector size
    uint8_t *buffer = (uint8_t *)heap_caps_malloc(sectors_needed * 512, MALLOC_CAP_DMA);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate write buffer");
        sdcard_end(card, handle);
        return false;
    }

    memset(buffer, 0, sectors_needed * 512);

    // Write length as first 4 bytes (little endian)
    buffer[0] = (data_len >> 0) & 0xFF;
    buffer[1] = (data_len >> 8) & 0xFF;
    buffer[2] = (data_len >> 16) & 0xFF;
    buffer[3] = (data_len >> 24) & 0xFF;

    // Copy data after length
    memcpy(buffer + 4, data, data_len);

    ESP_LOGI(TAG, "Writing %zu bytes to SD card...", data_len);

    esp_err_t ret = sdmmc_write_sectors(card, buffer, CODE_FILE_START_SECTOR, sectors_needed);
    free(buffer);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to SD card: %s", esp_err_to_name(ret));
        sdcard_end(card, handle);
        return false;
    }

    ESP_LOGI(TAG, "Write successful");
    sdcard_end(card, handle);
    return true;
}

char* sdcard_read_file(const char *path, size_t *len)
{
    sdspi_dev_handle_t handle;
    sdmmc_card_t *card = sdcard_begin(&handle);
    if (card == NULL) {
        *len = 0;
        return NULL;
    }

    // Read first sector to get length
    uint8_t *header = (uint8_t *)heap_caps_malloc(512, MALLOC_CAP_DMA);
    if (header == NULL) {
        ESP_LOGE(TAG, "Failed to allocate header buffer");
        sdcard_end(card, handle);
        *len = 0;
        return NULL;
    }

    esp_err_t ret = sdmmc_read_sectors(card, header, CODE_FILE_START_SECTOR, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read header: %s", esp_err_to_name(ret));
        free(header);
        sdcard_end(card, handle);
        *len = 0;
        return NULL;
    }

    // Read length from first 4 bytes
    size_t data_len = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);

    if (data_len == 0 || data_len > MAX_CODE_SIZE) {
        ESP_LOGW(TAG, "Invalid data length: %zu", data_len);
        free(header);
        sdcard_end(card, handle);
        *len = 0;
        return NULL;
    }

    // Calculate sectors needed
    size_t total_size = data_len + 4;
    size_t sectors_needed = (total_size + 511) / 512;

    // If we need more than 1 sector, read all
    uint8_t *buffer;
    if (sectors_needed > 1) {
        free(header);
        buffer = (uint8_t *)heap_caps_malloc(sectors_needed * 512, MALLOC_CAP_DMA);
        if (buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate read buffer");
            sdcard_end(card, handle);
            *len = 0;
            return NULL;
        }

        ret = sdmmc_read_sectors(card, buffer, CODE_FILE_START_SECTOR, sectors_needed);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read data: %s", esp_err_to_name(ret));
            free(buffer);
            sdcard_end(card, handle);
            *len = 0;
            return NULL;
        }
    } else {
        buffer = header;
    }

    // Allocate result string
    char *content = (char *)malloc(data_len + 1);
    if (content == NULL) {
        ESP_LOGE(TAG, "Failed to allocate result buffer");
        free(buffer);
        sdcard_end(card, handle);
        *len = 0;
        return NULL;
    }

    memcpy(content, buffer + 4, data_len);
    content[data_len] = '\0';
    free(buffer);

    *len = data_len;
    ESP_LOGI(TAG, "Read %zu bytes from SD card", data_len);
    sdcard_end(card, handle);
    return content;
}

bool sdcard_is_mounted(void)
{
    // Always return false since we don't keep it mounted
    return false;
}
