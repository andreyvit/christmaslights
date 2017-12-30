#include "effects.h"
#include <stdlib.h>
#include <stdbool.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

enum {
    O_END,
    O_INIT,
    O_ERASE_ALL,
    O_SET,
    O_FILL_PERC,
    O_LOOP,
    O_MAIN_LOOP,
    O_NEXT,
    O_SKIP,
    O_SPEED_MS,
    O_SPEED_X,
    O_PAL_ROTATE_RIGHT,
    O_PAL_ROTATE_LEFT,
    O_PIX_ROTATE_RIGHT,
    O_PIX_ROTATE_LEFT,
};

#define kBaseSpeed 500
#define kTargetEffectDurationMs 30000

enum {
    kMaxStepCount = 50,
    kMaxArgCount = 10,
    kMaxLoopDepth = 4,
};

typedef uint8_t RENDERING_FLAGS;

enum {
    RF_DEFAULT = 0,
    RF_REPEATING = 0x01,
};

#define kMainLoopCount -42

static int effect_idx;
static int step_idx;
static int loop_counts[kMaxLoopDepth];
static int loop_depth;
static int skip_count;
static uint8_t palette_rotation;
static int pixel_rotation;
static uint16_t pixel_count;
static uint64_t effect_duration_ms;
static RENDERING_FLAGS rendering_flags;
static PARAMS params;

enum { kInitialEffect = 0 };

static uint8_t effects_data[][kMaxStepCount][1+kMaxArgCount] = {
    { // 0
        {O_INIT, 4, RF_REPEATING},
        {O_MAIN_LOOP},
            {O_LOOP, 10},
                {O_SET, 1, 2, 3, 4},
                {O_PIX_ROTATE_RIGHT, 1},
            {O_NEXT},
            {O_LOOP, 10},
                {O_SET, 1, 2, 3, 4},
                {O_PIX_ROTATE_LEFT, 1},
            {O_NEXT},
        {O_NEXT},
    },
    { // 1
        {O_INIT, 4, RF_REPEATING},
        {O_MAIN_LOOP},
            {O_SET, 1, 0, 0, 0},
            {O_SET, 1, 2, 0, 0},
            {O_SET, 1, 2, 3, 0},
            {O_SET, 1, 2, 3, 4},
            {O_SET, 0, 2, 3, 4},
            {O_SET, 0, 0, 3, 4},
            {O_SET, 0, 0, 0, 4},
            {O_SET, 0, 0, 0, 0},
            {O_PAL_ROTATE_RIGHT, 1},
        {O_NEXT},
    },
    { // 2
        {O_INIT, 4, RF_REPEATING},
        {O_SPEED_MS, 10, 00},
        {O_MAIN_LOOP},
            {O_LOOP, 10},
                {O_SET, 1, 0, 1, 0},
                {O_SET, 0, 3, 0, 3},
            {O_NEXT},
            {O_PAL_ROTATE_RIGHT, 1},
        {O_NEXT},
    },
    { // 3
        {O_INIT, 4, RF_REPEATING},
        {O_MAIN_LOOP},
            {O_LOOP, 10},
                {O_SET, 1, 2, 3, 0},
                {O_SET, 1, 2, 0, 3},
                {O_SET, 1, 0, 2, 3},
                {O_SET, 0, 1, 2, 3},
            {O_NEXT},
            {O_PAL_ROTATE_RIGHT, 1},
        {O_NEXT},
    },
    { // 4
        {O_INIT, 8, RF_REPEATING},
        {O_SPEED_MS, 1, 50},
        {O_MAIN_LOOP},
            {O_SET, 4, 0, 0, 0, 0, 0, 0, 1},
            {O_SET, 4, 4, 0, 0, 0, 0, 1, 1},
            {O_SET, 4, 4, 4, 0, 0, 1, 1, 1},
            {O_SET, 4, 4, 4, 4, 1, 1, 1, 1},
            {O_SET, 0, 4, 4, 4, 1, 1, 1, 0},
            {O_SET, 0, 0, 4, 4, 1, 1, 0, 0},
            {O_SET, 0, 0, 0, 4, 1, 0, 0, 0},
            {O_SET, 0, 0, 4, 0, 0, 1, 0, 0},
            {O_SET, 0, 4, 0, 0, 0, 0, 1, 0},
            {O_PAL_ROTATE_RIGHT, 1},
        {O_NEXT},
    },
    { // 5
        {O_INIT, 5, RF_REPEATING},
        {O_SPEED_MS, 5, 50},
        {O_MAIN_LOOP},
            {O_LOOP, 3},
                {O_SET, 1, 1, 0, 0, 0},
                {O_SET, 0, 1, 1, 0, 0},
                {O_SET, 0, 0, 1, 1, 0},
                {O_SET, 0, 0, 0, 1, 1},
                {O_SET, 0, 0, 1, 1, 0},
                {O_SET, 0, 1, 1, 0, 0},
            {O_NEXT},
            {O_PAL_ROTATE_RIGHT, 1},
        {O_NEXT},
    },
    { // 6
        {O_INIT, 4, RF_REPEATING},
        {O_SPEED_MS, 7, 50},
        {O_MAIN_LOOP},
            {O_SET, 1, 0, 3, 0},
            {O_SET, 0, 1, 0, 3},
            {O_SET, 1, 0, 3, 0},
            {O_SET, 0, 2, 0, 3},
            {O_SET, 2, 0, 3, 0},
            {O_SET, 0, 2, 0, 3},
            {O_SET, 2, 0, 4, 0},
            {O_SET, 0, 2, 0, 4},
            {O_SET, 2, 0, 4, 0},
            {O_SET, 0, 1, 0, 4},
            {O_SET, 1, 0, 4, 0},
            {O_SET, 0, 1, 0, 4},
        {O_NEXT},
    },
    { // 7
        {O_INIT, 5, RF_REPEATING},
        {O_SPEED_MS, 15, 00},
        {O_MAIN_LOOP},
            {O_SET, 1, 0, 0, 0, 0},
            {O_SET, 1, 0, 0, 2, 0},
            {O_SET, 1, 3, 0, 2, 0},
            {O_SET, 1, 3, 0, 2, 4},
            {O_SET, 0, 3, 0, 2, 4},
            {O_SET, 0, 3, 0, 0, 4},
            {O_SET, 0, 0, 0, 0, 4},
            {O_SET, 0, 0, 0, 0, 0},
            {O_PAL_ROTATE_LEFT, 1},
        {O_NEXT},
    },
    { // 8
        {O_INIT, 5, RF_REPEATING},
        {O_SPEED_MS, 4, 00},
        {O_MAIN_LOOP},
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
            {O_PAL_ROTATE_RIGHT, 1},
        {O_NEXT},
    },
    { // 9
        {O_INIT, 5, RF_REPEATING},
        {O_SPEED_MS, 5, 00},
        {O_MAIN_LOOP},
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
            {O_PAL_ROTATE_LEFT, 2},
        {O_NEXT},
    },
    { // 10
        {O_INIT, 2, RF_REPEATING},
        {O_SPEED_MS, 5, 00},
        {O_MAIN_LOOP},
            {O_LOOP, 5},
                {O_SET, 1, 2},
                {O_SET, 2, 1},
            {O_NEXT},
            {O_PAL_ROTATE_RIGHT, 2},
        {O_NEXT},
    },
    { // 11
        {O_INIT, 0, RF_DEFAULT},
        {O_SPEED_MS, 0, 100},
        {O_MAIN_LOOP},
            {O_LOOP, 5},
                {O_LOOP, 120},
                    {O_ERASE_ALL},
                    {O_FILL_PERC, 1, 0, 10, 1, 10},
                    {O_PIX_ROTATE_RIGHT, 1},
                {O_NEXT},
                {O_PAL_ROTATE_RIGHT, 1},
            {O_NEXT},
            {O_LOOP, 5},
                {O_LOOP, 120},
                    {O_ERASE_ALL},
                    {O_FILL_PERC, 1, 0, 10, 1, 10},
                    {O_PIX_ROTATE_LEFT, 1},
                {O_NEXT},
                {O_PAL_ROTATE_RIGHT, 1},
            {O_NEXT},
        {O_NEXT},
    },
    { // 12
        {O_INIT, 5, RF_REPEATING},
        {O_SPEED_MS, 15, 00},
        {O_MAIN_LOOP},
        {O_SET, 1, 0, 0, 0, 0},
        {O_SET, 1, 1, 0, 0, 0},
        {O_SET, 1, 1, 1, 0, 0},
        {O_SET, 1, 1, 1, 1, 0},
        {O_SET, 1, 1, 1, 1, 1},
        {O_SET, 0, 1, 1, 1, 1},
        {O_SET, 0, 0, 1, 1, 1},
        {O_SET, 0, 0, 0, 1, 1},
        {O_SET, 0, 0, 0, 0, 1},
        {O_SET, 0, 0, 0, 0, 0},
        {O_PAL_ROTATE_LEFT, 1},
        {O_NEXT},
    },
};

#define kEffectCount (sizeof(effects_data)/sizeof(effects_data[0]))

void effects_restart(int effect) {
    effect_idx = effect;
    step_idx = 0;
    loop_depth = 0;
    skip_count = 0;
    pixel_count = kLEDCount;
    palette_rotation = 0;
    pixel_rotation = 0;
    effect_duration_ms = 0;
    params.next_tick_delay_ms = kBaseSpeed;
}

void effects_advance(int delta) {
    int e = (int)effect_idx + delta;
    while (e < kEffectCount) {
        e += kEffectCount;
    }
    e = e % kEffectCount;
    effects_restart(e);
}

void effects_reset(void) {
//    effects_restart(kEffectCount - 1);
    effects_restart(kInitialEffect);
}

static uint8_t apply_palette_transformations(uint8_t orig_color) {
    if (orig_color == 0) {
        return 0;
    }
    
    // palette_rotation == 1
    // 2 -> 3
    // 4 -> 1
    //
    // palette_rotation == 3
    // 1 -> 4
    // 3 -> 2
    return 1 + (orig_color - 1 + palette_rotation) % kPalletteSize;
}

static void set_pixel(uint8_t *pixels, int i, uint8_t color) {
    color = apply_palette_transformations(color);
    
    int rotation = pixel_rotation;
    while (rotation < 0) {
        rotation += pixel_count;
    }
    i = (i + rotation) % pixel_count;
    
    pixels[i] = color;
    if (rendering_flags & RF_REPEATING) {
        i += pixel_count;
        while (i < kLEDCount) {
            pixels[i] = color;
            i += pixel_count;
        }
    }
}

bool effects_exec_step(uint8_t *pixels) {
    uint8_t *step = effects_data[effect_idx][step_idx++];
    uint8_t op = step[0];
    if (step_idx == kMaxStepCount) {
        abort();
    }
    
    switch (op) {
        case O_END:
            effects_restart((effect_idx + 1) % kEffectCount);
            return false;
        case O_INIT:
            pixel_count = (step[1] == 0 ? kLEDCount : step[1]);
            rendering_flags = step[2];
            break;
        case O_ERASE_ALL:
            for (int i = 0; i < kLEDCount; i++) {
                pixels[i] = 0;
            }
            break;
        case O_SET: {
            int count = MIN(pixel_count, kMaxArgCount);
            for (int i = 0; i < count; i++) {
                set_pixel(pixels, i, step[1+i]);
            }
            return false;
        }
        case O_FILL_PERC: {
            int s = pixel_count * step[2] / step[3];
            int e = pixel_count * step[4] / step[5];
            for (int i = s; i < e; i++) {
                set_pixel(pixels, i, step[1]);
            }
            return false;
        }
        case O_MAIN_LOOP:
            if (loop_depth + 1 >= kMaxLoopDepth) {
                abort();
            }
            loop_depth++;
            loop_counts[loop_depth-1] = kMainLoopCount;
            break;
        case O_LOOP:
            if (loop_depth + 1 >= kMaxLoopDepth) {
                abort();
            }
            loop_depth++;
            loop_counts[loop_depth-1] = step[1];
            break;
        case O_NEXT: {
            if (loop_depth == 0) {
                abort();
            }
            bool has_more_iterations = false;
            int remaining = loop_counts[loop_depth-1];
            if (remaining == kMainLoopCount) {
                has_more_iterations = (effect_duration_ms < kTargetEffectDurationMs);
            } else {
                if (remaining > 0) {
                    loop_counts[loop_depth-1]--;
                    has_more_iterations = true;
                }
            }

            if (has_more_iterations) {
                int nesting = 0;
                for (step_idx--; step_idx > 0; step_idx--) {
                    uint8_t op = effects_data[effect_idx][step_idx-1][0];
                    if (op == O_LOOP || op == O_MAIN_LOOP) {
                        if (nesting == 0) {
                            break;
                        } else {
                            nesting--;
                        }
                    } else if (op == O_NEXT) {
                        nesting++;
                    }
                }
            } else {
                loop_depth--;
            }
            break;
        }
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
        case O_PAL_ROTATE_RIGHT:
            palette_rotation += step[1];
            palette_rotation = palette_rotation % kPalletteSize;
            break;
        case O_PAL_ROTATE_LEFT:
            palette_rotation += (kPalletteSize - step[1]);
            palette_rotation = palette_rotation % kPalletteSize;
            break;
        case O_PIX_ROTATE_RIGHT:
            pixel_rotation += step[1];
            break;
        case O_PIX_ROTATE_LEFT:
            pixel_rotation -= step[1];
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
    effect_duration_ms += params.next_tick_delay_ms;
    *a_params = params;
    a_params->effect = effect_idx;
    a_params->step = step_idx;
}

