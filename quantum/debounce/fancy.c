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
#    define DEBOUNCE_UP (DEBOUNCE * 2)
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

#define DEBOUNCE_NULL 255

typedef struct {
    // If nonzero, number of debounce milliseconds remaining,
    // and this key is in the debounce list.
    uint8_t remaining;
    uint8_t next;
} debounce_counter_t;

static debounce_counter_t *debounce_counters;
static uint8_t debounce_list_head;

static bool last_time_initialized;
static fast_timer_t last_time;

// we use num_rows rather than MATRIX_ROWS to support split keyboards
void debounce_init(uint8_t num_rows) {
    last_time_initialized = false;

    debounce_counters = malloc(num_rows * MATRIX_COLS * sizeof(debounce_counter_t));
    debounce_counter_t *p = debounce_counters;
    for (uint8_t r = 0; r < num_rows; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            p->remaining = 0;
            p->next = DEBOUNCE_NULL;
            ++p;
        }
    }

    debounce_list_head = DEBOUNCE_NULL;
}

void debounce_free(void) {
    free(debounce_counters);
    debounce_counters = NULL;

    debounce_list_head = DEBOUNCE_NULL;

    last_time_initialized = false;
}

#define iprintf(...) ((void)0)
//#define iprintf(...) (printf(__VA_ARGS__))

static uint8_t get_elapsed(void) {
#ifdef DEBOUNCE_USE_FRAMES
    // This debouncer uses frames instead of milliseconds.
    return 1;
#else
    // TODO: initialize last_time somewhere
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
#endif
}


void debounce(matrix_row_t raw[], matrix_row_t cooked[], uint8_t num_rows, bool changed) {
    const uint8_t elapsed = get_elapsed();

    uint8_t* pp = &debounce_list_head;
    uint8_t p = debounce_list_head;
    while (DEBOUNCE_NULL != p) {
        debounce_counter_t* entry = debounce_counters + p;
        assert(entry->remaining != 0);
        if (entry->remaining > elapsed) {
            entry->remaining -= elapsed;
            pp = &entry->next;
            p = *pp;
        } else {
            // Apply raw to cooked.
            uint8_t row = p / MATRIX_COLS;
            uint8_t col = p % MATRIX_COLS;
            matrix_row_t col_mask = 1 << col;
            matrix_row_t delta = (cooked[row] ^ raw[row]) & col_mask;
            iprintf("bumping cooked: %ld\n", (long)cooked[row]);
            cooked[row] ^= delta;

            // Remove from list.
            p = entry->next;
            *pp = p;
            entry->remaining = 0;
            entry->next = DEBOUNCE_NULL;
        }
    }

    iprintf("debounce: changed=%s, elapsed=%d\n", changed ? "true" : "false", elapsed);

    if (changed) {
        debounce_counter_t* p = debounce_counters;
        for (uint8_t row = 0; row < num_rows; ++row) {
            matrix_row_t cooked_row = cooked[row];
            matrix_row_t raw_row = raw[row];
            matrix_row_t delta = cooked_row ^ raw_row;
            if (0 == delta) {
                p += MATRIX_COLS;
                continue;
            }
            for (uint8_t col = 0; col < MATRIX_COLS; ++col, ++p) {
                matrix_row_t col_mask = 1 << col;
                if (col_mask & (cooked_row ^ raw_row)) {
                    // is (row, col) debouncing?
                    if (0 == p->remaining) {
                        // Not debouncing, so add it to the head of the list.
                        p->next = debounce_list_head;
                        debounce_list_head = p - debounce_counters;
                        p->remaining = (col_mask & raw_row) ? DEBOUNCE_DOWN : DEBOUNCE_UP;
                    } else {
                        // fluttering: this frame doesn't count
                        p->remaining += elapsed;
                    }
                }
            }
        }
    }
}

bool debounce_active(void) { return true; }
