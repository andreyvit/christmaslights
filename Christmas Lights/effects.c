#include "effects.h"
#include <stdlib.h>

static int effect_idx;

static void effect_0(uint32_t t, uint8_t *pixels);
static void effect_1(uint32_t t, uint8_t *pixels);
static void effect_2(uint32_t t, uint8_t *pixels);

void effects_reset(void) {
    effect_idx = 2;
}

void effects_get(uint32_t t, uint8_t *pixels) {
    switch (effect_idx) {
        case 0:
            effect_0(t, pixels);
            break;
        case 1:
            effect_1(t, pixels);
            break;
        case 2:
            effect_2(t, pixels);
            break;

        default:
            abort();
    }
}

void effect_0(uint32_t t, uint8_t *pixels) {
    for (int i = 0; i < kLEDCount; i++) {
        pixels[i] = 1 + (t + i) % kPalletteSize;
    }
}

uint8_t effect_1_data[][4] = {
    {1, 0, 0, 0},
    {1, 2, 0, 0},
    {1, 2, 3, 0},
    {1, 2, 3, 4},
    {0, 2, 3, 4},
    {0, 0, 3, 4},
    {0, 0, 0, 4},
    {0, 0, 0, 0},
};

void effect_1(uint32_t t, uint8_t *pixels) {
    int et = t % (sizeof(effect_1_data) / sizeof(effect_1_data[0]));
    for (int i = 0; i < kLEDCount; i++) {
        int ei = i % (sizeof(effect_1_data[0]) / sizeof(effect_1_data[0][0]));
        pixels[i] = effect_1_data[et][ei];
    }
}

uint8_t effect_2_data[][4] = {
    {1, 0, 1, 0},
    {0, 3, 0, 3},
    {1, 0, 1, 0},
    {0, 3, 0, 3},
    {1, 0, 1, 0},
    {0, 3, 0, 3},
    {1, 0, 1, 0},
    {0, 3, 0, 3},
    {2, 0, 2, 0},
    {0, 4, 0, 4},
    {2, 0, 2, 0},
    {0, 4, 0, 4},
    {2, 0, 2, 0},
    {0, 4, 0, 4},
    {2, 0, 2, 0},
    {0, 4, 0, 4},
    {2, 0, 2, 0},
    {0, 4, 0, 4},
};

void effect_2(uint32_t t, uint8_t *pixels) {
    int et = t % (sizeof(effect_2_data) / sizeof(effect_2_data[0]));
    for (int i = 0; i < kLEDCount; i++) {
        int ei = i % (sizeof(effect_2_data[0]) / sizeof(effect_2_data[0][0]));
        pixels[i] = effect_2_data[et][ei];
    }
}
