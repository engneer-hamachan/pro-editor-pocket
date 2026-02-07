/*
 * SDCard Native mrubyc bindings
 */

#include "sdcard_driver.h"
#include <mrubyc.h>
#include <stdlib.h>

// mrubyc class pointer
mrbc_class *mrbc_class_SDCard = NULL;

/* ==============================================
 * Method: SDCard.init
 * Initialize SD card
 * Returns: true on success, false on failure
 * ============================================== */
static void c_sdcard_init(mrbc_vm *vm, mrbc_value *v, int argc)
{
    bool success = sdcard_init();
    if (success) {
        SET_TRUE_RETURN();
    } else {
        SET_FALSE_RETURN();
    }
}

/* ==============================================
 * Method: SDCard.save(code_string) or SDCard.save(slot, code_string)
 * Save code to SD card
 * Args:
 *   1 arg:  code_string - Ruby string to save (uses slot 0)
 *   2 args: slot (0-7), code_string - save to specific slot
 * Returns: true on success, false on failure
 * ============================================== */
static void c_sdcard_save(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc < 1) {
        SET_FALSE_RETURN();
        return;
    }

    int slot = 0;
    const char *code = NULL;

    if (argc == 1) {
        // SDCard.save(code) - save to slot 0
        if (mrbc_type(v[1]) != MRBC_TT_STRING) {
            SET_FALSE_RETURN();
            return;
        }
        code = (const char *)GET_STRING_ARG(1);
    } else {
        // SDCard.save(slot, code) - save to specified slot
        if (mrbc_type(v[1]) != MRBC_TT_INTEGER) {
            SET_FALSE_RETURN();
            return;
        }
        if (mrbc_type(v[2]) != MRBC_TT_STRING) {
            SET_FALSE_RETURN();
            return;
        }
        slot = GET_INT_ARG(1);
        code = (const char *)GET_STRING_ARG(2);
    }

    bool success = sdcard_write_slot(slot, code);

    if (success) {
        SET_TRUE_RETURN();
    } else {
        SET_FALSE_RETURN();
    }
}

/* ==============================================
 * Method: SDCard.load or SDCard.load(slot)
 * Load code from SD card
 * Args:
 *   0 args: load from slot 0
 *   1 arg:  slot (0-7) - load from specific slot
 * Returns: String content or nil on failure
 * ============================================== */
static void c_sdcard_load(mrbc_vm *vm, mrbc_value *v, int argc)
{
    int slot = 0;

    if (argc >= 1) {
        // SDCard.load(slot) - load from specified slot
        if (mrbc_type(v[1]) != MRBC_TT_INTEGER) {
            SET_NIL_RETURN();
            return;
        }
        slot = GET_INT_ARG(1);
    }

    size_t len = 0;
    char *content = sdcard_read_slot(slot, &len);

    if (content == NULL || len == 0) {
        SET_NIL_RETURN();
        return;
    }

    // Create mrubyc string from the content
    mrbc_value str = mrbc_string_new(vm, content, len);

    // Free the allocated buffer
    free(content);

    SET_RETURN(str);
}

/* ==============================================
 * Method: SDCard.mounted?
 * Check if SD card is mounted
 * Returns: true if mounted, false otherwise
 * ============================================== */
static void c_sdcard_mounted(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (sdcard_is_mounted()) {
        SET_TRUE_RETURN();
    } else {
        SET_FALSE_RETURN();
    }
}

/* ==============================================
 * Initialize SDCard class
 * ============================================== */
void mrbc_sdcard_init(mrbc_vm *vm)
{
    mrbc_class_SDCard = mrbc_define_class(vm, "SDCard", mrbc_class_object);

    mrbc_define_method(vm, mrbc_class_SDCard, "init", c_sdcard_init);
    mrbc_define_method(vm, mrbc_class_SDCard, "save", c_sdcard_save);
    mrbc_define_method(vm, mrbc_class_SDCard, "load", c_sdcard_load);
    mrbc_define_method(vm, mrbc_class_SDCard, "mounted?", c_sdcard_mounted);
}
