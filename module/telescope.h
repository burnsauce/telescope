#define SAMPLE_RATE 4000 
#define DISPLAY_RATE (4000) // double columns per second
#define DISPLAY_DIVISOR (SAMPLE_RATE / DISPLAY_RATE)
#define SCOPE_MAX_ZOOM 32 
#define SCOPE_MIN_ZOOM 32 
#define SCOPE_CACHE_SIZE (128 * DISPLAY_DIVISOR * SCOPE_MIN_ZOOM)
//#define TWO_TIMERS // Async display update (doesn't work well)
