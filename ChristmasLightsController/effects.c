#include "config.h"
#if ENABLE_EFFECTS
#include "effects.h"
#include <stdlib.h>
#include <stdbool.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#ifdef ARDUINO
#include <Arduino.h>
#define R(x) pgm_read_byte(&(x))
// #define LOGSTR(str) Serial.print(F(str))
// #define LOGSTRLN(str) Serial.println(F(str))
// #define LOGINT(val) Serial.print(val)
// #define LOGINTLN(val) Serial.println(val)

extern void effects_log_string(const char *message);
extern void effects_log_int(int value);

#define LOGSTR(str) effects_log_string(str)
#define LOGSTRLN(str) effects_log_string(str "\n")
#define LOGINT(val) effects_log_int(val)
#define LOGINTLN(val) do{}while(effects_log_int(val); effects_log_string("\n");)
#else
#define PROGMEM
#define R(x) (x)
#define LOGSTR(str) printf("%s", str)
#define LOGSTRLN(str) printf("%s\n", str)
#define LOGINT(str) printf("%d", str)
#define LOGINTLN(str) printf("%d\n", str)
#endif

enum {
    O_HALT = 0,
    O_BEGIN_EFFECT = '{',
    O_END_EFFECT = '}',
    O_ERASE_ALL = 'E',
    O_SET = '=',
    O_FILL_PERC = 'F',
    O_LOOP = 'L',
    O_MAIN_LOOP = 'M',
    O_NEXT = 'N',
    O_WAIT = 'W',
    O_SPEED_MS = 'S',
    O_SPEED_X = 'X',
    O_PAL_ROTATE_RIGHT = ']',
    O_PAL_ROTATE_LEFT = '[',
    O_PIX_ROTATE_RIGHT = '>',
    O_PIX_ROTATE_LEFT = '<',
};

enum {
    kEffectMax = 255,
    kMaxArgCount = 10,
    kMaxLoopDepth = 4,
};

typedef uint8_t RENDERING_FLAGS;

enum {
    RF_DEFAULT = 0,
    RF_REPEATING = 0x01,
};

#define kMainLoopCount -42

#ifdef __cplusplus
extern "C" {
#endif


static int available_led_count;

static uint16_t effect_first, effect_last;
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

static const uint8_t effects_data[][1+kMaxArgCount] PROGMEM = {
    {O_BEGIN_EFFECT, 1, 4, RF_REPEATING},
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
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 2, 4, RF_REPEATING},
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
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 3, 4, RF_REPEATING},
    {O_SPEED_MS, 10, 00},
    {O_MAIN_LOOP},
        {O_LOOP, 10},
            {O_SET, 1, 0, 1, 0},
            {O_SET, 0, 3, 0, 3},
        {O_NEXT},
        {O_PAL_ROTATE_RIGHT, 1},
    {O_NEXT},
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 4, 4, RF_REPEATING},
    {O_MAIN_LOOP},
        {O_LOOP, 10},
            {O_SET, 1, 2, 3, 0},
            {O_SET, 1, 2, 0, 3},
            {O_SET, 1, 0, 2, 3},
            {O_SET, 0, 1, 2, 3},
        {O_NEXT},
        {O_PAL_ROTATE_RIGHT, 1},
    {O_NEXT},
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 5, 8, RF_REPEATING},
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
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 6, 5, RF_REPEATING},
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
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 7, 4, RF_REPEATING},
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
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 8, 5, RF_REPEATING},
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
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 9, 5, RF_REPEATING},
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
        {O_WAIT, 4},
        {O_PAL_ROTATE_RIGHT, 1},
    {O_NEXT},
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 10, 5, RF_REPEATING},
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
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 11, 2, RF_REPEATING},
    {O_SPEED_MS, 5, 00},
    {O_MAIN_LOOP},
        {O_LOOP, 5},
            {O_SET, 1, 2},
            {O_SET, 2, 1},
        {O_NEXT},
        {O_PAL_ROTATE_RIGHT, 2},
    {O_NEXT},
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 12, 0, RF_DEFAULT},
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
    {O_END_EFFECT},

    {O_BEGIN_EFFECT, 13, 5, RF_REPEATING},
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
    {O_END_EFFECT},
};

#define kStepCount (sizeof(effects_data)/sizeof(effects_data[0]))

static int effects_find_start(int effect) {
    for (int i = 0; i < kStepCount; i++) {
        if (R(effects_data[i][0]) == O_BEGIN_EFFECT) {
            if (R(effects_data[i][1]) == effect) {
                return i;
            }
        }
    }
    return 0;
}

static int effects_determine_prev(int effect) {
    int candidate = effect_last;
    for (int i = 0; i < kStepCount; i++) {
        if (R(effects_data[i][0]) == O_BEGIN_EFFECT) {
            uint8_t e = R(effects_data[i][1]);
            if (e < effect && (candidate == effect_last || e > candidate)) {
                candidate = e;
            }
        }
    }
    return candidate;
}

static int effects_determine_next(int effect) {
    int candidate = effect_first;
    for (int i = 0; i < kStepCount; i++) {
        if (R(effects_data[i][0]) == O_BEGIN_EFFECT) {
            uint8_t e = R(effects_data[i][1]);
            if (e > effect && (candidate == effect_first || e < candidate)) {
                candidate = e;
            }
        }
    }
    return candidate;
}

void effects_goto_effect(int effect) {
    effect_idx = effect;
    step_idx = effects_find_start(effect);
    loop_depth = 0;
}

void effects_advance(int delta) {
    int e = effect_idx;
    while (delta > 0) {
        e = effects_determine_next(e);
        delta--;
    }
    while (delta < 0) {
        e = effects_determine_prev(e);
        delta++;
    }
    effects_goto_effect(e);
}

void effects_reset(int a_available_led_count) {
    available_led_count = a_available_led_count;

    effect_first = kEffectMax;
    effect_last = 0;
    int effect = kEffectMax;
    for (int i = 0; i < kStepCount; i++) {
        if (R(effects_data[i][0]) == O_BEGIN_EFFECT) {
            uint8_t e = R(effects_data[i][1]);
            if (e < effect_first) {
                effect_first = e;
            }
            if (e > effect_last) {
                effect_last = e;
            }
        }
    }

    effects_goto_effect((kInitialEffect == 0 ? effect_first : kInitialEffect));
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
        while (i < available_led_count) {
            pixels[i] = color;
            i += pixel_count;
        }
    }
}

#define RARG(k) R(effects_data[step_idx][k])

bool effects_exec_step(uint8_t *pixels) {
    uint8_t op = RARG(0);
    bool more = true;

    switch (op) {
        case O_BEGIN_EFFECT:
            effect_idx = RARG(1);
            pixel_count = (RARG(2) == 0 ? available_led_count : RARG(2));
            rendering_flags = RARG(3);
#if TRACE_EXECUTION
            LOGSTR("B pxc=");
            LOGINT(effect_idx);
            LOGSTR(" pxc=");
            LOGINT(pixel_count);
            LOGSTR(" rf=");
            LOGINT(rendering_flags);
            LOGSTR("\n");
#endif
            skip_count = 0;
            palette_rotation = 0;
            pixel_rotation = 0;
            effect_duration_ms = 0;
            params.next_tick_delay_ms = kBaseSpeed;
            break;

        case O_END_EFFECT:
#if TRACE_EXECUTION
            LOGSTR("E\n");
#endif
            effects_goto_effect(effects_determine_next(effect_idx));
            break;

        case O_HALT:
            LOGSTRLN("HALT");
            abort();

        case O_ERASE_ALL:
#if TRACE_EXECUTION
            LOGSTR("ERASE\n");
#endif
            for (int i = 0; i < available_led_count; i++) {
                pixels[i] = 0;
            }
            break;
        case O_SET: {
            int count = MIN(pixel_count, kMaxArgCount);
#if TRACE_EXECUTION
            LOGSTR("SET");
#endif
            for (int i = 0; i < count; i++) {
              uint8_t c = RARG(1+i);
#if TRACE_EXECUTION
            LOGSTR(" ");
            LOGINT(c);
#endif
                set_pixel(pixels, i, c);
            }
#if TRACE_EXECUTION
            LOGSTR("\n");
#endif
            more = false;
            break;
        }
        case O_FILL_PERC: {
            int s = pixel_count * RARG(2) / RARG(3);
            int e = pixel_count * RARG(4) / RARG(5);
            uint8_t c = RARG(1);
#if TRACE_EXECUTION
            LOGSTR("FILL ");
            LOGINT(c);
            LOGSTR(" IN ");
            LOGINT(s);
            LOGSTR("..");
            LOGINT(e);
            LOGSTR("\n");
#endif
            for (int i = s; i < e; i++) {
                set_pixel(pixels, i, c);
            }
            more = false;
            break;
        }
        case O_MAIN_LOOP:
            if (loop_depth + 1 >= kMaxLoopDepth) {
                abort();
            }
            loop_depth++;
            loop_counts[loop_depth-1] = kMainLoopCount;
#if TRACE_EXECUTION
            LOGSTR("LOOP MAIN\n");
#endif
            break;
        case O_LOOP:
            if (loop_depth + 1 >= kMaxLoopDepth) {
                abort();
            }
            uint8_t n = RARG(1);
            loop_depth++;
            loop_counts[loop_depth-1] = n;
#if TRACE_EXECUTION
            LOGSTR("LOOP ");
            LOGINT(n);
            LOGSTR("\n");
#endif
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
#if TRACE_EXECUTION
            LOGSTR("NEXT\n");
#endif
                int nesting = 0;
                for (step_idx--; step_idx > 0; step_idx--) {
                    uint8_t op = R(effects_data[step_idx][0]);
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
#if TRACE_EXECUTION
            LOGSTR("NEXT (done)\n");
#endif
                loop_depth--;
            }
            break;
        }
        case O_WAIT: {
            if (skip_count == 0) {
                skip_count = MAX(1, RARG(1));
            }
#if TRACE_EXECUTION
            LOGSTR("WAIT ");
            LOGINT(skip_count);
            LOGSTR("\n");
#endif
            skip_count--;
            if (skip_count > 0) {
                step_idx--;
            }
            more = false;
            break;
        }
        case O_SPEED_MS:
            params.next_tick_delay_ms = (uint32_t)RARG(1) * 100 + RARG(2);
#if TRACE_EXECUTION
            LOGSTR("SPEED ");
            LOGINT(params.next_tick_delay_ms);
            LOGSTR("\n");
#endif
            break;
        case O_SPEED_X:
            params.next_tick_delay_ms = (uint32_t)kBaseSpeed * RARG(1) / RARG(2);
#if TRACE_EXECUTION
            LOGSTR("SPEED ");
            LOGINT(params.next_tick_delay_ms);
            LOGSTR("\n");
#endif
            break;
        case O_PAL_ROTATE_RIGHT: {
            uint8_t n = RARG(1);
#if TRACE_EXECUTION
            LOGSTR("PAL_R ");
            LOGINT(n);
            LOGSTR("\n");
#endif
            palette_rotation += n;
            palette_rotation = palette_rotation % kPalletteSize;
            break;
        }
        case O_PAL_ROTATE_LEFT: {
            uint8_t n = RARG(1);
#if TRACE_EXECUTION
            LOGSTR("PAL_L ");
            LOGINT(n);
            LOGSTR("\n");
#endif
            palette_rotation += (kPalletteSize - RARG(1));
            palette_rotation = palette_rotation % kPalletteSize;
            break;
        }
        case O_PIX_ROTATE_RIGHT: {
            uint8_t n = RARG(1);
#if TRACE_EXECUTION
            LOGSTR("PX_R ");
            LOGINT(n);
            LOGSTR("\n");
#endif
            pixel_rotation += RARG(1);
            break;
        }
        case O_PIX_ROTATE_LEFT: {
            uint8_t n = RARG(1);
#if TRACE_EXECUTION
            LOGSTR("PX_L ");
            LOGINT(n);
            LOGSTR("\n");
#endif
            pixel_rotation -= RARG(1);
            break;
        }
        default:
            abort();
    }

    step_idx++;
    return more;
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

#ifdef __cplusplus
}
#endif

#endif
