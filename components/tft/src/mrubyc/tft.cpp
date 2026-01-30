#include <mrubyc.h>
#include "../../ports/esp32/tft_espi_impl.h"

// Global class pointer for TFT class
mrbc_class *mrbc_class_TFT = NULL;

// Gem initialization function for mruby/c
extern "C" void
mrbc_tft_init(mrbc_vm *vm)
{
    /* Define TFT class */
    mrbc_class_TFT = mrbc_define_class(vm, "TFT", mrbc_class_object);

    /* Initialize TFT_eSPI method groups */
    mrbc_tft_espi_core_init(vm, mrbc_class_TFT);
    mrbc_tft_espi_text_init(vm, mrbc_class_TFT);

    /* Note: TFT_eSPI instance is created globally in tft_espi_core.cpp */
    /* User will call TFT.init or TFT.begin to initialize the display */
}
