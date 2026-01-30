/*
 * TFT Native mrubyc bindings
 * Uses ESP-IDF SPI API directly (no TFT_eSPI)
 */

#include "st7789_spi.h"
#include <mrubyc.h>

// mrubyc class pointer
mrbc_class *mrbc_class_TFT = NULL;

/* ==============================================
 * Method: TFT.init
 * ============================================== */
static void c_tft_init(mrbc_vm *vm, mrbc_value *v, int argc)
{
    bool success = st7789_init();
    if (success) {
        st7789_fill_screen(COLOR_BLACK);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.begin (alias for init)
 * ============================================== */
static void c_tft_begin(mrbc_vm *vm, mrbc_value *v, int argc)
{
    c_tft_init(vm, v, argc);
}

/* ==============================================
 * Method: TFT.width
 * ============================================== */
static void c_tft_width(mrbc_vm *vm, mrbc_value *v, int argc)
{
    SET_INT_RETURN(st7789_width());
}

/* ==============================================
 * Method: TFT.height
 * ============================================== */
static void c_tft_height(mrbc_vm *vm, mrbc_value *v, int argc)
{
    SET_INT_RETURN(st7789_height());
}

/* ==============================================
 * Method: TFT.set_rotation(rotation)
 * ============================================== */
static void c_tft_set_rotation(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 1) {
        uint8_t rotation = (uint8_t)GET_INT_ARG(1);
        st7789_set_rotation(rotation);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.get_rotation
 * ============================================== */
static void c_tft_get_rotation(mrbc_vm *vm, mrbc_value *v, int argc)
{
    SET_INT_RETURN(st7789_get_rotation());
}

/* ==============================================
 * Method: TFT.fill_screen(color)
 * ============================================== */
static void c_tft_fill_screen(mrbc_vm *vm, mrbc_value *v, int argc)
{
    uint16_t color = COLOR_BLACK;
    if (argc >= 1) {
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(1);
        color = rgb888_to_rgb565(rgb888);
    }
    st7789_fill_screen(color);
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.draw_pixel(x, y, color)
 * ============================================== */
static void c_tft_draw_pixel(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 3) {
        int16_t x = (int16_t)GET_INT_ARG(1);
        int16_t y = (int16_t)GET_INT_ARG(2);
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(3);
        uint16_t color = rgb888_to_rgb565(rgb888);
        st7789_draw_pixel(x, y, color);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.fill_rect(x, y, w, h, color)
 * ============================================== */
static void c_tft_fill_rect(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 5) {
        int16_t x = (int16_t)GET_INT_ARG(1);
        int16_t y = (int16_t)GET_INT_ARG(2);
        int16_t w = (int16_t)GET_INT_ARG(3);
        int16_t h = (int16_t)GET_INT_ARG(4);
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(5);
        uint16_t color = rgb888_to_rgb565(rgb888);
        st7789_fill_rect(x, y, w, h, color);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.set_cursor(x, y)
 * ============================================== */
static void c_tft_set_cursor(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 2) {
        int16_t x = (int16_t)GET_INT_ARG(1);
        int16_t y = (int16_t)GET_INT_ARG(2);
        st7789_set_cursor(x, y);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.set_text_color(color)
 * ============================================== */
static void c_tft_set_text_color(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 1) {
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(1);
        uint16_t color = rgb888_to_rgb565(rgb888);
        st7789_set_text_color(color);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.set_text_size(size)
 * ============================================== */
static void c_tft_set_text_size(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 1) {
        uint8_t size = (uint8_t)GET_INT_ARG(1);
        st7789_set_text_size(size);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.set_text_wrap(wrap)
 * ============================================== */
static void c_tft_set_text_wrap(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 1) {
        bool wrap = mrbc_type(v[1]) == MRBC_TT_TRUE;
        st7789_set_text_wrap(wrap);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.print(text)
 * ============================================== */
static void c_tft_print(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 1) {
        const char* text = (const char*)GET_STRING_ARG(1);
        st7789_print(text);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.set_backlight(level)
 * ============================================== */
static void c_tft_set_backlight(mrbc_vm *vm, mrbc_value *v, int argc)
{
    uint8_t level = 16;  // default max
    if (argc >= 1) {
        level = (uint8_t)GET_INT_ARG(1);
    }
    st7789_set_backlight(level);
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.draw_fast_h_line(x, y, w, color)
 * ============================================== */
static void c_tft_draw_fast_h_line(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 4) {
        int16_t x = (int16_t)GET_INT_ARG(1);
        int16_t y = (int16_t)GET_INT_ARG(2);
        int16_t w = (int16_t)GET_INT_ARG(3);
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(4);
        uint16_t color = rgb888_to_rgb565(rgb888);
        st7789_draw_fast_h_line(x, y, w, color);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.draw_fast_v_line(x, y, h, color)
 * ============================================== */
static void c_tft_draw_fast_v_line(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 4) {
        int16_t x = (int16_t)GET_INT_ARG(1);
        int16_t y = (int16_t)GET_INT_ARG(2);
        int16_t h = (int16_t)GET_INT_ARG(3);
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(4);
        uint16_t color = rgb888_to_rgb565(rgb888);
        st7789_draw_fast_v_line(x, y, h, color);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.draw_rect(x, y, w, h, color)
 * ============================================== */
static void c_tft_draw_rect(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 5) {
        int16_t x = (int16_t)GET_INT_ARG(1);
        int16_t y = (int16_t)GET_INT_ARG(2);
        int16_t w = (int16_t)GET_INT_ARG(3);
        int16_t h = (int16_t)GET_INT_ARG(4);
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(5);
        uint16_t color = rgb888_to_rgb565(rgb888);
        st7789_draw_rect(x, y, w, h, color);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.draw_round_rect(x, y, w, h, r, color)
 * ============================================== */
static void c_tft_draw_round_rect(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 6) {
        int16_t x = (int16_t)GET_INT_ARG(1);
        int16_t y = (int16_t)GET_INT_ARG(2);
        int16_t w = (int16_t)GET_INT_ARG(3);
        int16_t h = (int16_t)GET_INT_ARG(4);
        int16_t r = (int16_t)GET_INT_ARG(5);
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(6);
        uint16_t color = rgb888_to_rgb565(rgb888);
        st7789_draw_round_rect(x, y, w, h, r, color);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Method: TFT.fill_round_rect(x, y, w, h, r, color)
 * ============================================== */
static void c_tft_fill_round_rect(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc >= 6) {
        int16_t x = (int16_t)GET_INT_ARG(1);
        int16_t y = (int16_t)GET_INT_ARG(2);
        int16_t w = (int16_t)GET_INT_ARG(3);
        int16_t h = (int16_t)GET_INT_ARG(4);
        int16_t r = (int16_t)GET_INT_ARG(5);
        uint32_t rgb888 = (uint32_t)GET_INT_ARG(6);
        uint16_t color = rgb888_to_rgb565(rgb888);
        st7789_fill_round_rect(x, y, w, h, r, color);
    }
    SET_NIL_RETURN();
}

/* ==============================================
 * Initialize TFT class
 * ============================================== */
void mrbc_tft_init(mrbc_vm *vm)
{
    mrbc_class_TFT = mrbc_define_class(vm, "TFT", mrbc_class_object);

    mrbc_define_method(vm, mrbc_class_TFT, "init", c_tft_init);
    mrbc_define_method(vm, mrbc_class_TFT, "begin", c_tft_begin);
    mrbc_define_method(vm, mrbc_class_TFT, "width", c_tft_width);
    mrbc_define_method(vm, mrbc_class_TFT, "height", c_tft_height);
    mrbc_define_method(vm, mrbc_class_TFT, "set_rotation", c_tft_set_rotation);
    mrbc_define_method(vm, mrbc_class_TFT, "get_rotation", c_tft_get_rotation);
    mrbc_define_method(vm, mrbc_class_TFT, "fill_screen", c_tft_fill_screen);
    mrbc_define_method(vm, mrbc_class_TFT, "draw_pixel", c_tft_draw_pixel);
    mrbc_define_method(vm, mrbc_class_TFT, "fill_rect", c_tft_fill_rect);
    mrbc_define_method(vm, mrbc_class_TFT, "set_cursor", c_tft_set_cursor);
    mrbc_define_method(vm, mrbc_class_TFT, "set_text_color", c_tft_set_text_color);
    mrbc_define_method(vm, mrbc_class_TFT, "set_text_size", c_tft_set_text_size);
    mrbc_define_method(vm, mrbc_class_TFT, "set_text_wrap", c_tft_set_text_wrap);
    mrbc_define_method(vm, mrbc_class_TFT, "print", c_tft_print);
    mrbc_define_method(vm, mrbc_class_TFT, "set_backlight", c_tft_set_backlight);
    mrbc_define_method(vm, mrbc_class_TFT, "draw_fast_h_line", c_tft_draw_fast_h_line);
    mrbc_define_method(vm, mrbc_class_TFT, "draw_fast_v_line", c_tft_draw_fast_v_line);
    mrbc_define_method(vm, mrbc_class_TFT, "draw_rect", c_tft_draw_rect);
    mrbc_define_method(vm, mrbc_class_TFT, "draw_round_rect", c_tft_draw_round_rect);
    mrbc_define_method(vm, mrbc_class_TFT, "fill_round_rect", c_tft_fill_round_rect);
}
