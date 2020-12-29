#ifndef effects_h
#define effects_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 TODOs:
 - palette shuffling
 - palette window
 - transition style
 - global palette rotation
 - multiple subeffects
 - multiple palettes
 - pixel color transitions

 */

typedef struct {
    uint32_t next_tick_delay_ms;
    uint8_t effect, step;
} PARAMS;

void effects_reset(int led_count);
void effects_advance(int delta);
void effects_tick(uint8_t *pixels, PARAMS *params);

#ifdef __cplusplus
}
#endif

#endif /* effects_h */
