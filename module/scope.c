#include "telescope.h"
#include "scope.h"
#include "display.h"
#include "bufdisplay.h"
#include "print_funcs.h"

static uint16_t samples[SCOPE_CACHE_SIZE];
static volatile uint32_t sp = 0;
static int16_t zoom = 1;

void scope_zoom(int16_t z) {
    if (z == 0) z = 1;
    if (z > SCOPE_MAX_ZOOM) z = SCOPE_MAX_ZOOM;
    if (z < 0 - SCOPE_MIN_ZOOM) z = 0 - SCOPE_MIN_ZOOM;
    zoom = z;
}

static inline void increment_sp(void) {
    sp = (sp + 1) % SCOPE_CACHE_SIZE;
}

static inline uint32_t normalize_sp(int32_t d) {
    while (d < 0)
        d += SCOPE_CACHE_SIZE;
    return d % SCOPE_CACHE_SIZE;
}

static inline uint16_t get_sample(uint32_t s) {
    return samples[normalize_sp(s)];
}

void scope_init(void) {
    d_start_command();
    d_remap(D_REMAP_COM | D_REMAP_SPLIT | D_REMAP_VERTICAL);
    d_end();
}

void scope_process_sample(uint16_t sample) {
    increment_sp();
    samples[sp] = sample;
}

static inline int32_t get_offset(uint32_t base, int32_t offset) {
    if (zoom > 0)
        return (int32_t)base + ((offset * DISPLAY_DIVISOR) / zoom);
    return (int32_t)base + ((offset * DISPLAY_DIVISOR) * zoom);
}

void scope_draw() {
    static uint8_t count = 0;
    static uint32_t rp = 0;

    if (count == 0) {
        display_new_frame();
        rp = sp;
    }


    uint8_t y = 63 - get_sample(get_offset(rp, -count)) / 64;
    display_poke(count, y, 1);

    count++;

    if (count % 2 == 0) {
        display_render_2_cols(count - 2);
    }

    count %= 128;
}
