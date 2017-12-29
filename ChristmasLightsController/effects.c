#include "effects.h"
#include <stdlib.h>
#include <stdbool.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

enum {
    O_END,
    O_REPEAT_PX,
    O_SET,
    O_LOOP,
    O_NEXT,
    O_SKIP,
    O_SPEED_MS,
    O_SPEED_X,
};

static int effect_idx;
static int step_idx;
static int loop_count;
static int skip_count;
static uint8_t repeating_pixel_count;
static PARAMS params;

#define kBaseSpeed 250

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
        {O_LOOP, 20},
        {O_SET, 1, 2, 3, 4},
        {O_SET, 2, 3, 4, 1},
        {O_SET, 3, 4, 1, 2},
        {O_SET, 4, 1, 2, 3},
        {O_NEXT},
    },
    { // 1
        {O_REPEAT_PX, 4},
        {O_LOOP, 20},
        {O_SET, 1, 0, 0, 0},
        {O_SET, 1, 2, 0, 0},
        {O_SET, 1, 2, 3, 0},
        {O_SET, 1, 2, 3, 4},
        {O_SET, 0, 2, 3, 4},
        {O_SET, 0, 0, 3, 4},
        {O_SET, 0, 0, 0, 4},
        {O_SET, 0, 0, 0, 0},
        {O_NEXT},
    },
    { // 2
        {O_REPEAT_PX, 4},
        {O_SPEED_X, 10, 5},
        {O_LOOP, 10},
        {O_SET, 1, 0, 1, 0},
        {O_SET, 0, 3, 0, 3},
        {O_NEXT},
        {O_LOOP, 10},
        {O_SET, 2, 0, 2, 0},
        {O_SET, 0, 4, 0, 4},
        {O_NEXT},
    },
    { // 3 XXX replace same as 0
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
        {O_SPEED_X, 3, 5},
        {O_LOOP, 20},
        {O_SET, 4, 0, 0, 0, 0, 0, 0, 1},
        {O_SET, 4, 4, 0, 0, 0, 0, 1, 1},
        {O_SET, 4, 4, 4, 0, 0, 1, 1, 1},
        {O_SET, 4, 4, 4, 4, 1, 1, 1, 1},
        {O_SET, 0, 4, 4, 4, 1, 1, 1, 0},
        {O_SET, 0, 0, 4, 4, 1, 1, 0, 0},
        {O_SET, 0, 0, 0, 4, 1, 0, 0, 0},
        {O_SET, 0, 0, 4, 0, 0, 1, 0, 0},
        {O_SET, 0, 4, 0, 0, 0, 0, 1, 0},
        {O_NEXT},
    },
    { // 5
        {O_REPEAT_PX, 4},
        {O_LOOP, 20},
        {O_SET, 1, 0, 0, 0},
        {O_SET, 0, 2, 0, 0},
        {O_SET, 0, 0, 3, 0},
        {O_SET, 0, 0, 0, 4},
        {O_SET, 0, 0, 3, 0},
        {O_SET, 0, 2, 0, 0},
        {O_NEXT},
    },
    { // 6
        {O_REPEAT_PX, 4},
        {O_LOOP, 10},
        {O_SET, 1, 0, 3, 0},
        {O_SET, 0, 2, 0, 4},
        {O_NEXT},
    },
    { // 7
        {O_REPEAT_PX, 5},
        {O_SPEED_X, 6, 1},
        {O_LOOP, 20},
        {O_SET, 1, 0, 0, 0, 0},
        {O_SET, 1, 0, 0, 2, 0},
        {O_SET, 1, 3, 0, 2, 0},
        {O_SET, 1, 3, 0, 2, 4},
        {O_SET, 0, 3, 0, 2, 4},
        {O_SET, 0, 3, 0, 0, 4},
        {O_SET, 0, 0, 0, 0, 4},
        {O_SET, 0, 0, 0, 0, 0},
        {O_NEXT},
    },
    { // 8
        {O_REPEAT_PX, 5},
        {O_SPEED_MS, 3, 00},
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
        {O_SKIP, 4},
        {O_NEXT},
    },
    { // 9
        {O_REPEAT_PX, 5},
        {O_SPEED_X, 2, 1},
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
    skip_count = 0;
    repeating_pixel_count = 0;
    params.next_tick_delay_ms = kBaseSpeed;
}

void effects_reset(void) {
    //effects_start(kEffectCount - 1);
    effects_start(0);
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
        case O_SKIP: {
            if (skip_count == 0) {
                skip_count = MAX(1, step[1]);
            }
            skip_count--;
            if (skip_count > 0) {
                step_idx--;
            }
            return false;
        }
        case O_SPEED_MS:
            params.next_tick_delay_ms = (uint32_t)step[1] * 100 + step[2];
            break;
        case O_SPEED_X:
            params.next_tick_delay_ms = (uint32_t)kBaseSpeed * step[1] / step[2];
            break;
        default:
            abort();
    }
    return true;
}

void effects_tick(uint8_t *pixels, PARAMS *a_params) {
    while (effects_exec_step(pixels)) {
        continue;
    }
    *a_params = params;
    a_params->effect = effect_idx;
    a_params->step = step_idx;
}

