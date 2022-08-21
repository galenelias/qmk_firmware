/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
A no-op implementation of `debounce` that logs all raw matrix changes
to an internal ring buffer. Exports a `void debounce_debug(void)`
function that prints all recent change events.
*/
#include "matrix.h"
#include "timer.h"
#include "quantum.h"

#ifndef DEBOUNCE_DEBUG_LOG_SIZE
#define DEBOUNCE_DEBUG_LOG_SIZE 128
#endif

#if MATRIX_COLS > 127
#error Too many columns. We need the top bit for whether the change is down or up.
#endif

#if MATRIX_ROWS > 254
#error Too many rows. We need the top value to indicate entry is unused.
#endif

typedef struct debounce_event_t {
    uint16_t now;
    // 255 means this entry is unset.
    uint8_t row;
    // Top bit contains whether event is down is up.
    uint8_t col;
} debounce_event_t;

static debounce_event_t debounce_event_log[DEBOUNCE_DEBUG_LOG_SIZE];
static uint16_t debounce_event_log_wpos = 0;

static void record_event(debounce_event_t evt) {
    uint16_t wpos = debounce_event_log_wpos;
    debounce_event_t* e = debounce_event_log + wpos;
    if (wpos == DEBOUNCE_DEBUG_LOG_SIZE - 1) {
        debounce_event_log_wpos = 0;
    } else {
        debounce_event_log_wpos = wpos + 1;
    }

    *e = evt;
}

static inline void print_event(debounce_event_t* evt) {
    if (evt->row != 255) {
        uprintf("%5u: (%2u, %2u) %s\n", evt->now, evt->row, evt->col & 127, (evt->col & 128) ? "down" : "up");
    }
}

void debounce_debug(void) {
    for (uint16_t i = debounce_event_log_wpos; i < DEBOUNCE_DEBUG_LOG_SIZE; ++i) {
        print_event(debounce_event_log + i);
    }
    for (uint16_t i = 0; i < debounce_event_log_wpos; ++i) {
        print_event(debounce_event_log + i);
    }
}

void debounce_init(uint8_t num_rows) {
    for (uint16_t i = 0; i < DEBOUNCE_DEBUG_LOG_SIZE; ++i) {
        debounce_event_log[i].row = 255;
    }
}

void debounce(matrix_row_t raw[], matrix_row_t cooked[], uint8_t num_rows, bool changed) {
    if (!changed) {
        return;
    }

    uint16_t now = timer_read();

    for (uint8_t i = 0; i < num_rows; ++i) {
        matrix_row_t delta = raw[i] ^ cooked[i];
        matrix_row_t col_mask = 1;
        for (uint8_t j = 0; j < MATRIX_COLS; ++j, col_mask <<= 1) {
            if (delta & col_mask) {
                record_event((debounce_event_t){
                    .now = now,
                    .row = i,
                    .col = j | ((col_mask & raw[i]) ? 128 : 0),
                });
            }
        }
        cooked[i] = raw[i];
    }
}

bool debounce_active(void) { return true; }

void debounce_free(void) {}
