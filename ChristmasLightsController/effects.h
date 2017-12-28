#ifndef effects_h
#define effects_h

#include <stdint.h>

#define kLEDCount 288
#define kPalletteSize 4

/*
 TODOs:
 - O_SKIP
 - speed adjustment
 - normalize effects duration
 - pixels rotation
 - palette rotation
 - nested loops
 
 - palette window
 - transition style
 - global palette rotation
 - multiple effects
 - N pixel repeating animations
 - declarative steps
 - multiple subeffects
 - multiple palettes
 - pixel color transitions
*/

typedef struct {
    uint32_t next_tick_delay_ms;
} PARAMS;

void effects_reset(void);
void effects_tick(uint8_t *pixels, PARAMS *params);

#endif /* effects_h */
