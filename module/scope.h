#ifndef SCOPE_H
#define SCOPE_H

#include <stdint.h>

void scope_init(void);
void scope_draw(void);
void scope_zoom(uint16_t);
void scope_process_sample(uint16_t);

#endif
