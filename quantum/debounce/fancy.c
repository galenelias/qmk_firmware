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


/*
// Maximum debounce: 127ms
#if DEBOUNCE > 127
#    undef DEBOUNCE
#    define DEBOUNCE 127
#endif
*/

#define ROW_SHIFTER ((matrix_row_t)1)

#if (MATRIX_COLS * MATRIX_ROWS) >= 255
#error MATRIX_ROWS * MATRIX_COLS must be smaller than 255
#endif

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

static bool last_time_initialized;
static fast_timer_t last_time;

// we use num_rows rather than MATRIX_ROWS to support split keyboards
void debounce_init(uint8_t num_rows) {
    last_time_initialized = false;

    key_states = malloc(num_rows * MATRIX_COLS * sizeof(key_state_t));
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

    last_time_initialized = false;
}

#define iprintf(...) ((void)0)
//#define iprintf(...) (printf(__VA_ARGS__))

static fast_timer_t first_time;

static uint8_t get_elapsed(void) {
#ifdef DEBOUNCE_USE_FRAMES
    // This debouncer counts scan frames instead of milliseconds. This
    // introduces less sampling distortion for keyboards that sample at
    // a high, but sub-kHz, rate.
    return 1;
#else
    // TODO: initialize last_time somewhere
    if (unlikely(!last_time_initialized)) {
        last_time_initialized = true;
        last_time = timer_read_fast();
        first_time = last_time;
        iprintf("init timer: %d\n", last_time);
        return 1;
    }

    fast_timer_t now = timer_read_fast();
    fast_timer_t elapsed_time = TIMER_DIFF_FAST(now, last_time);
    last_time = now;
    iprintf("new timer: %d\n", last_time);
    return (elapsed_time > 255) ? 255 : elapsed_time;
#endif
}


static fast_timer_t get_time(void) {
    return timer_read_fast() - first_time;
}

void debounce(matrix_row_t raw[], matrix_row_t cooked[], uint8_t num_rows, bool changed) {
    const uint8_t elapsed = get_elapsed();

    //printf("elapsed: %d\n", (int)elapsed);

    key_state_t* p = key_states;
    for (uint8_t r = 0; r < num_rows; ++r) {
        matrix_row_t raw_row = raw[r];
        matrix_row_t cooked_row = cooked[r];
        matrix_row_t delta = cooked_row ^ raw_row;

        matrix_row_t col_mask = 1;
        for (uint8_t col = 0; col < MATRIX_COLS; ++col, col_mask <<= 1, ++p) {
            switch (p->state) {
                case WAITING:
                    if (delta & col_mask) {
                        printf("transitioning to DEBOUNCING %d\n", get_time());
                        p->state = DEBOUNCING;
                        p->remaining = (raw & col_mask) ? DEBOUNCE_DOWN : DEBOUNCE_UP;
                    }
                    break;
                case DEBOUNCING:
                    if (0 == (delta & col_mask)) {
                        // Detected bounce -- back to waiting.
                        printf("transitioning to WAITING %d\n", get_time());
                        p->state = WAITING;
                    } else if (p->remaining > elapsed) {
                        p->remaining -= elapsed;
                    } else {
                        printf("transitioning to QUIESCING %d\n", get_time());
                        p->state = QUIESCING;
                        p->remaining = DEBOUNCE_QUIESCE;
                        cooked_row ^= col_mask;
                    }
                    break;
                case QUIESCING:
                    if (p->remaining > elapsed) {
                        p->remaining -= elapsed;
                    } else {
                        printf("transitioning to WAITING %d\n", get_time());
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
