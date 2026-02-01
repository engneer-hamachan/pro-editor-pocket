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
 * Method: SDCard.save(code_string)
 * Save code to SD card
 * Args: code_string - Ruby string to save
 * Returns: true on success, false on failure
 * ============================================== */
static void c_sdcard_save(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc < 1) {
        SET_FALSE_RETURN();
        return;
    }

    // Check if argument is a string
    if (mrbc_type(v[1]) != MRBC_TT_STRING) {
        SET_FALSE_RETURN();
        return;
    }

    const char *code = (const char *)GET_STRING_ARG(1);
    bool success = sdcard_write_file("/sdcard/code.rb", code);

    if (success) {
        SET_TRUE_RETURN();
    } else {
        SET_FALSE_RETURN();
    }
}

/* ==============================================
 * Method: SDCard.load
 * Load code from SD card
 * Returns: String content or nil on failure
 * ============================================== */
static void c_sdcard_load(mrbc_vm *vm, mrbc_value *v, int argc)
{
    size_t len = 0;
    char *content = sdcard_read_file("/sdcard/code.rb", &len);

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
