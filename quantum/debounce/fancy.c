/*
 * Copyright 2017 Alex Ong <the.onga@gmail.com>
 * Copyright 2020 Andrei Purdea <andrei@purdea.ro>
 * Copyright 2021 Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
Basic symmetric per-key algorithm. Uses an 8-bit counter per key.
When no state changes have occured for DEBOUNCE milliseconds, we push the state.
*/

#include "matrix.h"
#include "timer.h"
#include "quantum.h"
#include <assert.h>
#include <stdlib.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/*
 * Keyboards with more than 16 columns can save a significant number of
 * instructions on AVR by using 24-bit integers instead of 32-bit.
 */
#if (MATRIX_COLS > 16 && MATRIX_COLS <= 24)
typedef __uint24 local_row_t;
#else
typedef matrix_row_t local_row_t;
#endif

#ifdef PROTOCOL_CHIBIOS
#    if CH_CFG_USE_MEMCORE == FALSE
#        error ChibiOS is configured without a memory allocator. Your keyboard may have set `#define CH_CFG_USE_MEMCORE FALSE`, which is incompatible with this debounce algorithm.
#    endif
#endif

#ifndef DEBOUNCE_USE_FRAMES
#    define DEBOUNCE_USE_FRAMES 0
#endif

#ifndef DEBOUNCE
#    define DEBOUNCE 5
#endif

#ifndef DEBOUNCE_DOWN
#    define DEBOUNCE_DOWN DEBOUNCE
#endif

#ifndef DEBOUNCE_UP
#    define DEBOUNCE_UP DEBOUNCE
#endif

#ifndef DEBOUNCE_QUIESCE
#    define DEBOUNCE_QUIESCE 30
#endif

#define iprintf(...) ((void)0)
//#define iprintf(...) (printf(__VA_ARGS__))

// *** TIMER ***

#if DEBOUNCE_USE_FRAMES

// This debouncer counts scan frames instead of milliseconds. This
// introduces less sampling distortion for keyboards that sample at
// a high, near-kHz rate.

static void time_init(void) {}
static void time_free(void) {}
inline fast_timer_t get_elapsed(void) {
    return 1;
}

#else

static bool last_time_initialized;
static fast_timer_t last_time;

static void time_init(void) {
    last_time_initialized = false;
}

static void time_free(void) {
    last_time_initialized = false;
}

static fast_timer_t get_elapsed(void) {
    if (unlikely(!last_time_initialized)) {
        last_time_initialized = true;
        last_time = timer_read_fast();
        iprintf("init timer: %d\n", last_time);
        return 1;
    }

    fast_timer_t now = timer_read_fast();
    fast_timer_t elapsed_time = TIMER_DIFF_FAST(now, last_time);
    last_time = now;
    iprintf("new timer: %d\n", last_time);
    return (elapsed_time > 255) ? 255 : elapsed_time;
}

#endif

// *** ANTI-GHOST COUNTS ***

// Nonzero if multiple keys are pressed in the row.
static uint8_t* multiple_in_row; // [num_rows]
// Count of keys in column.
static uint8_t down_in_col[MATRIX_COLS];

static void ghost_init(uint8_t num_rows) {
    multiple_in_row = calloc(num_rows, sizeof(*multiple_in_row));
}

static void ghost_free(void) {
    free(multiple_in_row);
}

static void ghost_compute(const matrix_row_t raw[], uint8_t num_rows) {
    // prevent stores from reloading the pointer:
    uint8_t* mrp = multiple_in_row;

    for (uint8_t r = 0; r < MATRIX_COLS; ++r) {
        down_in_col[r] = 0;
    }
    while (num_rows--) {
        matrix_row_t row = *raw++;
        if (likely(row == 0)) {
            *mrp++ = 0;
            continue;
        }
        uint8_t multiple = 0 != (row & (row - 1));
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            if (row & 1) {
                down_in_col[col] += 1;
            }
            row >>= 1;
        }
        *mrp++ = multiple;
    }
}

static inline bool multiple_in_row_and_col(uint8_t r, uint8_t c) {
    return multiple_in_row[r] && down_in_col[c] > 1;
}

// *** DEBOUNCE STATE ***

enum {
    WAITING = 0,
    DEBOUNCING = 1,
    QUIESCING = 2,
};

typedef struct {
    uint8_t state;
    // If nonzero, number of debounce milliseconds remaining.
    uint8_t remaining;
} key_state_t;

static key_state_t *key_states;

// we use num_rows rather than MATRIX_ROWS to support split keyboards
void debounce_init(uint8_t num_rows) {
    time_init();
    ghost_init(num_rows);

    key_states = calloc(num_rows * MATRIX_COLS, sizeof(key_state_t));
    key_state_t *p = key_states;
    for (uint8_t r = 0; r < num_rows; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            p->state = WAITING;
            ++p;
        }
    }
}

void debounce_free(void) {
    free(key_states);
    key_states = NULL;

    ghost_free();
    time_free();
}

static fast_timer_t get_time(void) {
    static bool first_time_initialized = false;
    static fast_timer_t first_time;
    if (unlikely(!first_time_initialized)) {
        first_time_initialized = true;
        first_time = timer_read_fast();
    }
    return timer_read_fast() - first_time;
}

static void log_transition(const char* name) {
#if 0
    printf("transitioning to %s %d\n", name, get_time());
#else
    (void)name;
    (void)get_time;
#endif
}

#if 0
staticuint8_t popcount8(uint8_t byte) {
    uint8_t count = 0;
    asm(
        "lsr %1\n"
        "adc %0, r1\n"
        "lsr %1\n"
        "adc %0, r1\n"
        "lsr %1\n"
        "adc %0, r1\n"
        "lsr %1\n"
        "adc %0, r1\n"
        "lsr %1\n"
        "adc %0, r1\n"
        "lsr %1\n"
        "adc %0, r1\n"
        "lsr %1\n"
        "adc %0, %1\n"
        : "+rm" (count)
        , "+rm" (byte)
    );
    return count;
}
#endif

void debounce(matrix_row_t raw[], matrix_row_t cooked[], uint8_t num_rows, bool changed) {
    const uint8_t elapsed = get_elapsed();

    if (changed) {
        ghost_compute(raw, num_rows);
    }

    key_state_t* p = key_states;
    for (uint8_t r = 0; r < num_rows; ++r) {
        matrix_row_t raw_row = raw[r];
        matrix_row_t cooked_row = cooked[r];
        matrix_row_t delta = cooked_row ^ raw_row;

        matrix_row_t col_mask = 1;
        for (uint8_t col = 0; col < MATRIX_COLS; ++col, col_mask <<= 1, ++p) {
            if (multiple_in_row_and_col(r, col)) {
                // Possible ghost, ignore any debouncing logic this time around.
                continue;
            }

            switch (p->state) {
                case WAITING:
                    if (delta & col_mask) {
                        log_transition("DEBOUNCING");
                        p->state = DEBOUNCING;
                        p->remaining = (raw_row & col_mask) ? DEBOUNCE_DOWN : DEBOUNCE_UP;
                    }
                    break;
                case DEBOUNCING:
                    if (0 == (delta & col_mask)) {
                        // Detected bounce -- back to waiting.
                        log_transition("WAITING");
                        p->state = WAITING;
                    } else if (p->remaining > elapsed) {
                        p->remaining -= elapsed;
                    } else {
                        log_transition("QUIESCING");
                        p->state = QUIESCING;
                        p->remaining = DEBOUNCE_QUIESCE;
                        cooked_row ^= col_mask;
                    }
                    break;
                case QUIESCING:
                    if (p->remaining > elapsed) {
                        p->remaining -= elapsed;
                    } else {
                        log_transition("WAITING");
                        p->state = WAITING;
                    }
                    break;
            }
        }
        cooked[r] = cooked_row;
    }    

    //iprintf("debounce: changed=%s, elapsed=%d\n", changed ? "true" : "false", elapsed);

}

bool debounce_active(void) { return true; }
