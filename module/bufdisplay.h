#ifndef BUFDISPLAY_H
#define BUFDISPLAY_H

#include <stdint.h>

uint8_t display_peek(uint8_t, uint8_t);
void display_poke(uint8_t, uint8_t, uint8_t);
void display_render(void);
void display_clear(void);
void display_new_frame(void);
void display_render_2_cols(uint8_t);

#endif
