#ifndef effects_h
#define effects_h

#include <stdint.h>

#define kLEDCount 288
#define kPalletteSize 4

/*
 TODOs:
 - nested loops
 - palette rotation
 - palette window
 - transition style
 - speed adjustment
 - global palette rotation
 - multiple effects
 - N pixel repeating animations
 - declarative steps
 - multiple subeffects
 - multiple palettes
 - pixel color transitions
*/

void effects_reset(void);
void effects_get(uint32_t t, uint8_t *pixels);

#endif /* effects_h */
