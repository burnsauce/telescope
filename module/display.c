#include "display.h"

static uint8_t chip_selected = 0;
static uint8_t data_selected = 0;

void select_chip(void);
void select_chip(void) {
    spi_selectChip(OLED_SPI, OLED_SPI_NPCS);
    chip_selected = 1;
}

void d_assert_chip(void) {
    if (!chip_selected)
        select_chip();
}

static inline void select_data(void) {
    gpio_set_gpio_pin(OLED_DC_PIN);
    data_selected = 1;
}

static inline void select_command(void) {
    gpio_clr_gpio_pin(OLED_DC_PIN);
    data_selected = 0;
}

void d_start_command(void) {
    if (!chip_selected)
        select_chip();
    if (data_selected) 
        select_command();
}

void d_end(void) {
    spi_unselectChip(OLED_SPI, OLED_SPI_NPCS);
    chip_selected = 0;
}

void d_start_data(void) {
    if (!chip_selected)
        select_chip();
    if (!data_selected)
        select_data();
}

