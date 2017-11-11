/* 
 * bufdisplay.c (c) 2017 Poindexter Frink
 *
 * A buffered display driver for the SSD1325
 *
 * Features
 *   - 1-bit display for memory density
 *   - TODO 4-bit display for fidelity
 *   - 2x1 pixel updates for low-density graphics, such as an oscilloscope
 *   - TODO 2x64 pixel updates for complex operations
 *
 *                                                                          */

#include "bufdisplay.h"
#include "display.h"

#include <stdbool.h>
#include <string.h> // memcmp()

/*
 * Internal data type: column
 * Each bit represents a set pixel
 *
 * uint64_t won't work on AVR32
 *                                                                           */
typedef uint32_t column[2];
/*                               LSb    MSb
 *                     [0] = row   0 to 31 
 *                     [1] = row  32 to 63
 *                                                                           */

/*
 * Reverse-shadowed buffer system
 *
 * Screen updates occur on the shadow, live screen holds current screen state
 *                                                                           */
static column  buf_1[128];
static column  buf_2[128];
static column* shadow = buf_1;
static column* live = buf_2;

/*
 * Internal data type: column_operations
 *
 * The SSD only allows access to a 2x1 pixel region at a time
 *                                                                           */
typedef struct {
    column erase_1; // which pixels to erase in the first column
    column write_1; // which pixels to write in the first column
    column erase_2;
    column write_2;
} column_operations;


static inline void render_columns(column_operations* op) {
    uint32_t p;
    uint8_t ri;
    for (uint8_t i = 0; i < 64; i++) {
        if (i > 31) {
            ri = 1;
            p = (1 << (i-32));
        }
        else {
            ri = 0;
            p = (1 << i);
        }
        
        if (p & op->erase_1[ri] || p & op->write_1[ri] ||
                p & op->erase_2[ri] || p & op->write_2[ri]) {

            uint8_t c = 0x00;
            if (p & op->write_2[ri])
                c |= 0xF0;
            if (p & op->write_1[ri])
                c |= 0x0F;

            d_start_command();
            d_row(i, i);
            
            d_start_data();
            d_write(c);
        }
    }
}

static column_operations blank;

static inline bool op_blank(column_operations a) {
    return memcmp(&blank, &a, sizeof(column_operations)) == 0;
}

void display_render_2_cols(uint8_t i) {
    column_operations op;
    uint32_t diff;

    d_start_command();
    d_col(i / 2, i / 2);

    diff = shadow[i][0] ^ live[i][0];
    op.erase_1[0] = live[i][0] & diff;
    op.write_1[0] = shadow[i][0] & diff;

    diff = shadow[i][1] ^ live[i][1];
    op.erase_1[1] = live[i][1] & diff;
    op.write_1[1] = shadow[i][1] & diff;

    diff = shadow[i + 1][0] ^ live[i + 1][0];
    op.erase_2[0] = live[i + 1][0] & diff;
    op.write_2[0] = shadow[i + 1][0] & diff;

    diff = shadow[i + 1][1] ^ live[i + 1][1];
    op.erase_2[1] = live[i + 1][1] & diff;
    op.write_2[1] = shadow[i + 1][1] & diff;

    if (!op_blank(op)) {
        // scan through and mirror columns
        for (int8_t j = 31; j >= 0; j--) {
            if ((1 << j) & op.erase_1[0] || (1 << j) & op.write_1[0])
                op.write_2[0] |= (1 << j) & shadow[i + 1][0];
            if ((1 << j) & op.erase_1[1] || (1 << j) & op.write_1[1])
                op.write_2[1] |= (1 << j) & shadow[i + 1][1];
            if ((1 << j) & op.erase_2[0] || (1 << j) & op.write_2[0])
                op.write_1[0] |= (1 << j) & shadow[i][0];
            if ((1 << j) & op.erase_2[1] || (1 << j) & op.write_2[1])
                op.write_1[1] |= (1 << j) & shadow[i][1];
        }

        render_columns(&op);
    }

    d_end();
}

void display_new_frame() {
    column* temp = live;
    live = shadow;
    shadow = temp;
    display_clear();
}

void display_render(void) {
    for (uint8_t i = 128; i > 0 ; i -= 2) {
        display_render_2_cols(128 - i);
    }

    display_new_frame();
}

uint8_t display_peek(uint8_t col, uint8_t row) {
    if (row > 31)
        return (shadow[col][1] & (1 << (row - 32))) > 0;
    else
        return (shadow[col][0] & (1 << row)) > 0;
}

void display_clear(void) {
    for (uint8_t i = 0; i < 128; i++) {
        shadow[i][0] = 0;
        shadow[i][1] = 0;
    }
}

void display_poke(uint8_t col, uint8_t row, uint8_t set) {
    uint8_t ri = row > 31;
    if (ri)
        row -= 32;
    if (set)
        shadow[col][ri] |= (1 << row);
    else
        shadow[col][ri] &= ~(1 << row); 
}
