#include "effects.h"

void effects_get(uint32_t t, uint8_t *pixels) {
    for (int i = 0; i < kLEDCount; i++) {
        pixels[i] = (t + i) % kPalletteSize;
    }
}
