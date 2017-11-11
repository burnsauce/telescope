#ifndef DISPLAY_H
#define DISPLAY_H

#include "spi.h"
#include "conf_board.h"
#include "delay.h"
#include "gpio.h"

#define D_REMAP_COLUMN     0x01
#define D_REMAP_NIBBLE     0x02
#define D_REMAP_VERTICAL   0x04
#define D_REMAP_COM        0x10
#define D_REMAP_SPLIT      0x40

#define D_OPTION_FILL      0x01
#define D_OPTION_XWRAP     0x02
#define D_OPTION_REVCOPY   0x10

#define D_MODE_NORMAL      0x00
#define D_MODE_ON          0x01
#define D_MODE_OFF         0x02
#define D_MODE_INVERT      0x03

#define D_SCROLL_12  0
#define D_SCROLL_64  1
#define D_SCROLL_128 2
#define D_SCROLL_256 3

#define d_write(x) ( spi_write(OLED_SPI, x) )

void d_start_command(void);

void d_end(void);

void d_start_data(void);

#ifdef TWO_TIMERS
void d_assert_chip(void);
#else
#define d_assert_chip() 
#endif

static inline void d_col(uint8_t start, uint8_t end) {
    d_assert_chip();
    d_write(0x15);
    d_write(start);
    d_write(end);
}

static inline void d_row(uint8_t start, uint8_t end) {
    d_assert_chip();
    d_write(0x75);
    d_write(start);
    d_write(end);
}

static inline void d_contrast(uint8_t current) {
    d_assert_chip();
    d_write(0x81);
    d_write(current);
}

static inline void d_contrast_level(uint8_t level) {
    d_assert_chip();
    if (level > 2) level = 2;
    d_write(0x84 | level);
}

static inline void d_remap(uint8_t mode) {
    d_assert_chip();
    d_write(0xA0);
    d_write(mode);
}

static inline void d_start_line(uint8_t line) {
    d_assert_chip();
    d_write(0xA1);
    d_write(line);
}

static inline void d_offset(uint8_t offset) {
    d_assert_chip();
    d_write(0xA2);
    d_write(offset);
}

static inline void d_mode(uint8_t mode) {
    d_assert_chip();
    d_write(0xA4 | (mode));
}

static inline void d_multiplex(uint8_t ratio) {
    d_assert_chip();
    d_write(0xA8);
    d_write(ratio);
}

static inline void d_master_config(uint8_t config) {
    d_assert_chip();
    d_write(0xAD);
    d_write(config);
}

static inline void d_display_off(void) {
    d_assert_chip();
    d_write(0xAE);
}

static inline void d_display_on(void) {
    d_assert_chip();
    d_write(0xAF);
}

static inline void d_precharge_comp_enable(void) {
    d_assert_chip();
    d_write(0xB0);
    d_write(0x28);
}

static inline void d_phase_len(uint8_t len) {
    d_assert_chip();
    d_write(0xB1);
    d_write(len);
}

static inline void d_row_period(uint8_t period) {
    d_assert_chip();
    d_write(0xB2);
    d_write(period);
}

static inline void d_clock(uint8_t data) {
    d_assert_chip();
    d_write(0xB3);
    d_write(data);
}

static inline void d_precharge_comp(uint8_t comp) {
    d_assert_chip();
    d_write(0xB4);
    d_write(comp);
}

static inline void d_greyscale_table(uint8_t a, uint8_t b, uint8_t c, uint8_t d,
                              uint8_t e, uint8_t f, uint8_t g, uint8_t h) {
    d_assert_chip();
    d_write(0xB8);
    d_write(a);
    d_write(b);
    d_write(c);
    d_write(d);
    d_write(e);
    d_write(f);
    d_write(g);
    d_write(h);
}

static inline void d_precharge_voltage(uint8_t voltage) {
    d_assert_chip();
    d_write(0xBC);
    d_write(voltage);
}

static inline void d_vcom_voltage(uint8_t voltage) {
    d_assert_chip();
    d_write(0xBE);
    d_write(voltage);
}

static inline void d_vsl(uint8_t voltage) {
    d_assert_chip();
    d_write(0xBF);
    d_write(voltage);
}

static inline void d_nop(void) {
    d_assert_chip();
    d_write(0xE3);
}

static inline void d_options(uint8_t options) {
    d_assert_chip();
    d_write(0x23);
    d_write(options);
}

static inline void d_draw_rect(uint8_t start_col, uint8_t start_row,
              uint8_t end_col,   uint8_t end_row, uint8_t pattern) {
    d_assert_chip();
    d_write(0x24);
    d_write(start_col);
    d_write(start_row);
    d_write(end_col);
    d_write(end_row);
    d_write(pattern);
}

static inline void d_copy(uint8_t start_col, uint8_t start_row,
                   uint8_t end_col,   uint8_t end_row,
                   uint8_t dst_col,   uint8_t dst_row) {
    d_assert_chip();
    d_write(0x25);
    d_write(start_col);
    d_write(start_row);
    d_write(end_col);
    d_write(end_row);
    d_write(dst_col);
    d_write(dst_row);
}

static inline void d_scroll(uint8_t offset, uint8_t rows, uint8_t interval) {
    d_assert_chip();
    d_write(0x26);
    d_write(offset);
    d_write(rows);
    d_write(interval);
}

static inline void d_scroll_stop(void) {
    d_assert_chip();
    d_write(0x2E);
}

static inline void d_scroll_start(void) {
    d_assert_chip();
    d_write(0x2F);
}

static inline void d_clear(void) {
    d_start_data();
    for (int i = 0; i < 4096; i++) {
        d_assert_chip();
        d_write(0);
    }
}
    

static inline void d_init(void) {
    gpio_set_gpio_pin(OLED_RES_PIN);
    delay_ms(1);
    gpio_clr_gpio_pin(OLED_RES_PIN);
    delay_ms(1);
    gpio_set_gpio_pin(OLED_RES_PIN);
    delay_ms(10);

    //// initialize OLED
    d_start_command();
    d_display_off();
    d_clock(0x91);
    d_multiplex(0x3F);
    d_contrast_level(0x02);
    d_contrast(0x7F);
    d_row_period(0x51);
    d_multiplex(0x3F);
    d_precharge_voltage(0x10);
    d_vcom_voltage(0x1C);
    d_master_config(0x02);
    d_remap(D_REMAP_COM | D_REMAP_SPLIT);
    d_start_line(0x00);
    d_offset(0x4C);
    d_phase_len(0x55);
    d_precharge_comp(0x02);
    d_precharge_comp_enable();
    d_vsl(0xF);
    d_mode(D_MODE_NORMAL);
    d_greyscale_table(0x01, 0x11, 0x22, 0x32, 0x43, 0x54, 0x65, 0x76);
    d_col(0x00, 0x3F);
    d_row(0x00, 0x3F);
    d_display_on();
    d_end();

    delay_ms(10);
    d_clear();
    d_end();
}

#endif
