/*
Copyright 2017 Alex Ong<the.onga@gmail.com>
Copyright 2021 Chad Austin <chad@chadaustin.me>
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
Asymmetric per-key debounce algorithm. Allocates a byte per row and a byte
per key.

#define DEBOUNCE_DOWN and DEBOUNCE_UP. If not specified, they default to DEBOUNCE.
*/

#include "matrix.h"
#include "timer.h"
#include "quantum.h"
#include <stdlib.h>

#ifndef DEBOUNCE
#    define DEBOUNCE 5
#endif

#ifndef DEBOUNCE_DOWN
#    define DEBOUNCE_DOWN DEBOUNCE
#endif

#ifndef DEBOUNCE_UP
#    define DEBOUNCE_UP DEBOUNCE
#endif

#define ROW_SHIFTER ((matrix_row_t)1)

typedef struct asym_defer_state_t {
    uint8_t count;
} asym_defer_state_t;

static uint16_t last_time;
// [row][column] milliseconds until key's state is considered debounced.
static asym_defer_state_t* state;
// [row] number of keys in row currently debouncing. Used to efficiently
// skip rows.
static uint8_t* row_counts;

void debounce_init(uint8_t num_rows) {
    state = (asym_defer_state_t*)calloc(num_rows, MATRIX_COLS * sizeof(asym_defer_state_t));
    row_counts = (uint8_t*)calloc(num_rows, sizeof(uint8_t));

    last_time = timer_read();
}

void debounce(matrix_row_t raw[], matrix_row_t cooked[], uint8_t num_rows, bool changed) {
    uint16_t elapsed16 = timer_elapsed(last_time);
    uint8_t elapsed = (elapsed16 > 255) ? 255 : elapsed16;

    asym_defer_state_t *statep = state;

    for (uint8_t row = 0; row < num_rows; ++row) {
      if (row_counts[row] == 0 && !changed) {
          statep += MATRIX_COLS;
          continue;
      }

      matrix_row_t raw_row = raw[row];
      matrix_row_t cooked_row = cooked[row];
      matrix_row_t delta = raw_row ^ cooked_row;

      matrix_row_t col_mask = ROW_SHIFTER;
      for (uint8_t col = 0; col < MATRIX_COLS; ++col, col_mask <<= 1, ++statep) {
        if (changed && (delta & col_mask)) {
          uint8_t new_count = (raw_row & col_mask) ? DEBOUNCE_DOWN : DEBOUNCE_UP;
          if (0 == statep->count) {
            ++row_counts[row];
          }
          statep->count = new_count;
        } else if (statep->count > elapsed) {
          statep->count -= elapsed;
        } else if (statep->count) {
          statep->count = 0;
          cooked_row = (~col_mask & cooked_row) | (col_mask & raw_row);
          --row_counts[row];
        }
      }

      cooked[row] = cooked_row;
    }
}

bool debounce_active(void) { return true; }
