#include "bufdisplay.h"
#include "display.h"
#include "print_funcs.h"

#include <stdbool.h>
#include <string.h> // memcmp()

typedef uint32_t column[2];
static column  buf_1[128];
static column  buf_2[128];
static column* shadow = buf_1;
static column* live = buf_2;

typedef struct {
    column erase_1;
    column write_1;
    column erase_2;
    column write_2;
} column_operations;

static inline column* get_column(uint8_t col) {
    return &shadow[col];
}

uint8_t display_peek(uint8_t col, uint8_t row) {
    if (row > 31)
        return (*get_column(col)[1] & (1 << (row - 32))) > 0;
    else
        return (*get_column(col)[0] & (1 << row)) > 0;
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
            //print_dbg("\r\n\r\nUpdating Row ");
            //print_dbg_ulong(i);

            uint8_t c = 0;
            if (p & op->write_2[ri])
                c |= 0xF0;
            if (p & op->write_1[ri])
                c |= 0x0F;

            d_start_command();
            d_row(i, i);
            //d_end();
            
            d_start_data();
            d_write(c);
            //d_write(0xF0);
            //d_end();

            //print_dbg(" = ");
            //print_dbg_ulong(c);
            //delay_ms(50);
        }
    }
}

static column_operations blank;

static inline bool op_blank(column_operations a) {
    if (memcmp(&blank, &a, sizeof(column_operations)))
        return false;
    return true;
}

void display_render_2_cols(uint8_t i) {
    column_operations op;
    uint32_t diff;

    d_start_command();
    d_col(i / 2, i / 2);
    //d_end();

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

    if (!op_blank(op))
        render_columns(&op);

    d_end();
}

void display_new_frame() {
    column* temp = live;
    live = shadow;
    shadow = temp;
}

void display_render(void) {
    for (uint8_t i = 0; i < 128; i += 2) {
        display_render_2_cols(i);
    }

    display_new_frame();
}
