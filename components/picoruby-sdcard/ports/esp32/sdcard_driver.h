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

// Slot-based storage constants
#define SLOT_START_SECTOR   1024    // First slot starts at sector 1024
#define SLOT_SIZE_SECTORS   16      // 16 sectors = 8KB per slot
#define MAX_SLOTS           8       // 8 slots available (0-7)
#define MAX_SLOT_DATA_SIZE  (SLOT_SIZE_SECTORS * 512 - 4)  // Max data per slot (minus 4-byte header)

// Write data to a specific slot (0-7)
// Returns true on success, false on failure
bool sdcard_write_slot(int slot, const char *data);

// Read data from a specific slot (0-7)
// Returns allocated buffer (caller must free) or NULL on failure
// Sets *len to the number of bytes read
char* sdcard_read_slot(int slot, size_t *len);

#ifdef __cplusplus
}
#endif
