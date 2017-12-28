#include "effects.h"
#include <stdlib.h>
#include <stdbool.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

enum {
    O_END,
    O_REPEAT_PX,
    O_SET,
    O_LOOP,
    O_NEXT,
};

static int effect_idx;
static int step_idx;
static int loop_count;
static uint32_t internal_time;
static uint8_t repeating_pixel_count;

static void effect_0(uint32_t t, uint8_t *pixels);

enum {
    kMaxStepCount = 50,
    kMaxArgCount = 10,
};

static uint8_t effects_data[][kMaxStepCount][1+kMaxArgCount] = {
    { // 0
        {O_REPEAT_PX, 4},
        {O_LOOP, 20},
        {O_SET, 1, 2, 3, 4},
        {O_SET, 4, 1, 2, 3},
        {O_SET, 3, 4, 1, 2},
        {O_SET, 2, 3, 4, 1},
        {O_NEXT},
    },
    { // 1
        {O_REPEAT_PX, 4},
        {O_SET, 1, 0, 0, 0},
        {O_SET, 1, 2, 0, 0},
        {O_SET, 1, 2, 3, 0},
        {O_SET, 1, 2, 3, 4},
        {O_SET, 0, 2, 3, 4},
        {O_SET, 0, 0, 3, 4},
        {O_SET, 0, 0, 0, 4},
        {O_SET, 0, 0, 0, 0},
    },
    { // 2
        {O_REPEAT_PX, 4},
        {O_LOOP, 10},
        {O_SET, 1, 0, 1, 0},
        {O_SET, 0, 3, 0, 3},
        {O_NEXT},
        {O_LOOP, 10},
        {O_SET, 2, 0, 2, 0},
        {O_SET, 0, 4, 0, 4},
        {O_NEXT},
    },
    { // 3
        {O_REPEAT_PX, 4},
        {O_LOOP, 10},
        {O_SET, 1, 2, 3, 4},
        {O_SET, 4, 1, 2, 3},
        {O_SET, 3, 4, 1, 2},
        {O_SET, 2, 3, 4, 1},
        {O_NEXT},
    },
    { // 4
        {O_REPEAT_PX, 8},
        {O_LOOP, 10},
        {O_SET, 1, 0, 0, 0, 0, 0, 0, 4},
        {O_SET, 0, 1, 0, 0, 0, 0, 4, 0},
        {O_SET, 0, 0, 1, 0, 0, 4, 0, 0},
        {O_SET, 0, 0, 0, 1, 4, 0, 0, 0},
        {O_SET, 0, 0, 0, 4, 1, 0, 0, 0},
        {O_SET, 0, 0, 4, 3, 2, 1, 0, 0},
        {O_SET, 4, 4, 3, 3, 2, 2, 1, 1},
        {O_SET, 0, 0, 0, 0, 0, 0, 0, 0},
        {O_SET, 4, 4, 3, 3, 2, 2, 1, 1},
        {O_SET, 0, 0, 0, 0, 0, 0, 0, 0},
        {O_SET, 4, 4, 3, 3, 2, 2, 1, 1},
        {O_SET, 0, 0, 0, 0, 0, 0, 0, 0},
        {O_SET, 4, 4, 3, 3, 2, 2, 1, 1},
        {O_SET, 0, 0, 0, 0, 0, 0, 0, 0},
        {O_SET, 4, 4, 3, 3, 2, 2, 1, 1},
        {O_NEXT},
    },
    { // 5
        {O_REPEAT_PX, 4},
        {O_LOOP, 10},
        {O_SET, 1, 0, 0, 0},
        {O_SET, 0, 0, 0, 0},
        {O_SET, 0, 2, 0, 0},
        {O_SET, 0, 0, 0, 0},
        {O_SET, 0, 0, 3, 0},
        {O_SET, 0, 0, 0, 0},
        {O_SET, 0, 0, 0, 4},
        {O_SET, 0, 0, 0, 0},
        {O_SET, 0, 0, 0, 4},
        {O_SET, 0, 0, 0, 0},
        {O_SET, 0, 0, 3, 0},
        {O_SET, 0, 0, 0, 0},
        {O_SET, 0, 2, 0, 0},
        {O_SET, 0, 0, 0, 0},
        {O_SET, 1, 0, 0, 0},
        {O_SET, 0, 0, 0, 0},
        {O_NEXT},
    },
    { // 6
        {O_REPEAT_PX, 4},
        {O_LOOP, 10},
        {O_SET, 1, 0, 3, 0},
        {O_SET, 0, 2, 0, 4},
        {O_NEXT},
        {O_LOOP, 4},
        {O_SET, 1, 2, 3, 4},
        {O_SET, 0, 0, 0, 0},
        {O_NEXT},
    },
    { // 7
        {O_REPEAT_PX, 5},
        {O_LOOP, 10},
        {O_SET, 1, 0, 0, 0, 0},
        {O_SET, 0, 1, 0, 0, 0},
        {O_SET, 2, 0, 1, 0, 0},
        {O_SET, 0, 2, 0, 1, 0},
        {O_SET, 3, 0, 2, 0, 1},
        {O_SET, 1, 3, 0, 2, 0},
        {O_SET, 4, 1, 3, 0, 2},
        {O_SET, 2, 4, 1, 3, 0},
        {O_SET, 1, 2, 4, 1, 3},
        {O_NEXT},
    },
    { // 8
        {O_REPEAT_PX, 5},
        {O_LOOP, 10},
        {O_SET, 1, 0, 0, 0, 0},
        {O_SET, 0, 1, 0, 0, 0},
        {O_SET, 0, 0, 1, 0, 0},
        {O_SET, 0, 0, 0, 1, 0},
        {O_SET, 0, 0, 0, 0, 1},
        {O_SET, 1, 2, 0, 0, 0},
        {O_SET, 0, 1, 2, 0, 0},
        {O_SET, 0, 0, 1, 2, 0},
        {O_SET, 0, 0, 0, 1, 2},
        {O_SET, 2, 0, 0, 0, 1},
        {O_SET, 1, 2, 3, 0, 0},
        {O_SET, 0, 1, 2, 3, 0},
        {O_SET, 0, 0, 1, 2, 3},
        {O_SET, 3, 0, 0, 1, 2},
        {O_SET, 2, 3, 0, 0, 1},
        {O_SET, 1, 2, 3, 4, 0},
        {O_SET, 0, 1, 2, 3, 4},
        {O_SET, 4, 0, 1, 2, 3},
        {O_SET, 3, 4, 0, 1, 2},
        {O_SET, 2, 3, 4, 0, 1},
        {O_SET, 1, 2, 3, 4, 0},
        {O_SET, 0, 1, 2, 3, 4},
        {O_SET, 4, 0, 1, 2, 3},
        {O_SET, 3, 4, 0, 1, 2},
        {O_SET, 2, 3, 4, 0, 1},
        {O_SET, 0, 2, 3, 4, 0},
        {O_SET, 0, 0, 2, 3, 4},
        {O_SET, 4, 0, 0, 2, 3},
        {O_SET, 3, 4, 0, 0, 2},
        {O_SET, 2, 3, 4, 0, 0},
        {O_SET, 0, 2, 3, 4, 0},
        {O_SET, 0, 0, 0, 3, 4},
        {O_SET, 4, 0, 0, 0, 3},
        {O_SET, 3, 4, 0, 0, 0},
        {O_SET, 0, 3, 4, 0, 0},
        {O_SET, 0, 0, 0, 4, 0},
        {O_SET, 0, 0, 0, 0, 4},
        {O_SET, 4, 0, 0, 0, 0},
        {O_SET, 0, 4, 0, 0, 0},
        {O_SET, 0, 0, 4, 0, 0},
        {O_SET, 0, 0, 0, 0, 0},
        {O_SET, 0, 0, 0, 0, 0},
        {O_SET, 0, 0, 0, 0, 0},
        {O_SET, 0, 0, 0, 0, 0},
        {O_SET, 0, 0, 0, 0, 0},
        {O_NEXT},
    },
    { // 9
        {O_REPEAT_PX, 5},
        {O_LOOP, 10},
        {O_SET, 1, 2, 3, 4, 0},
        {O_SET, 0, 1, 2, 3, 4},
        {O_SET, 4, 0, 1, 2, 3},
        {O_SET, 3, 4, 0, 1, 2},
        {O_SET, 2, 3, 4, 0, 1},
        {O_SET, 1, 2, 3, 4, 0},
        {O_SET, 2, 3, 4, 0, 1},
        {O_SET, 3, 4, 0, 1, 2},
        {O_SET, 4, 0, 1, 2, 3},
        {O_SET, 0, 1, 2, 3, 4},
        {O_NEXT},
    },
};

#define kEffectCount (sizeof(effects_data)/sizeof(effects_data[0]))

static void effects_start(int effect) {
    effect_idx = effect;
    step_idx = 0;
    loop_count = 0;
    repeating_pixel_count = 0;
}

void effects_reset(void) {
    internal_time = 0;
    effects_start(kEffectCount - 1);
}

bool effects_exec_step(uint8_t *pixels) {
    uint8_t *step = effects_data[effect_idx][step_idx++];
    uint8_t op = step[0];
    if (step_idx == kMaxStepCount) {
        abort();
    }
    
    switch (op) {
        case O_END:
            effects_start((effect_idx + 1) % kEffectCount);
            return false;
        case O_REPEAT_PX:
            repeating_pixel_count = step[1];
            break;
        case O_SET:
            if (repeating_pixel_count > 0) {
                for (int i = 0; i < kLEDCount; i++) {
                    int ei = i % repeating_pixel_count;
                    pixels[i] = step[1+ei];
                }
            } else {
                int count = MIN(kLEDCount, kMaxArgCount);
                for (int i = 0; i < count; i++) {
                    pixels[i] = step[1+i];
                }
            }
            return false;
        case O_LOOP:
            loop_count = step[1];
            break;
        case O_NEXT:
            if (loop_count > 0) {
                loop_count--;
                while (step_idx > 0 && effects_data[effect_idx][step_idx-1][0] != O_LOOP) {
                    step_idx--;
                }
            }
            break;
        default:
            abort();
    }
    return true;
}

void effects_exec_tick(uint8_t *pixels) {
    while (effects_exec_step(pixels)) {
        continue;
    }
}

void effects_get(uint32_t t, uint8_t *pixels) {
    switch (effect_idx) {
        case 0:
            effect_0(t, pixels);
            return;
    }
    
    while (internal_time < t) {
        internal_time++;
        effects_exec_tick(pixels);
    }
}

void effect_0(uint32_t t, uint8_t *pixels) {
    for (int i = 0; i < kLEDCount; i++) {
        pixels[i] = 1 + (t + i) % kPalletteSize;
    }
}

