#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// T-Deck Plus pin definitions
#define SDCARD_CS_PIN       39
#define TFT_CS_PIN          12
#define RADIO_CS_PIN        9
#define SPI_MOSI_PIN        41
#define SPI_MISO_PIN        38
#define SPI_SCK_PIN         40

// Initialize SD card (uses existing SPI bus from TFT)
bool sdcard_init(void);

// Write string to file on SD card
// Returns true on success, false on failure
bool sdcard_write_file(const char *path, const char *data);

// Read file contents from SD card
// Returns allocated buffer (caller must free) or NULL on failure
// Sets *len to the number of bytes read
char* sdcard_read_file(const char *path, size_t *len);

// Check if SD card is mounted
bool sdcard_is_mounted(void);

#ifdef __cplusplus
}
#endif
