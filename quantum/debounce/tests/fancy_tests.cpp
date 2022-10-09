/* Copyright 2021 Simon Arlott
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

#include "gtest/gtest.h"

#include "debounce_test_common.h"

TEST_F(DebounceTest, ShortBounceIgnored) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {1, {{0, 1, UP}}, {}},
        {2, {}, {}},
    });
    runEvents();
}

TEST_F(DebounceTest, OneKeyShort1) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {5, {}, {{0, 1, DOWN}}},
        {40, {}, {}}, // Tick the test simulator to transition out of quiescence
        {57, {{0, 1, UP}}, {}},
        {62, {}, {{0, 1, UP}}},
    });
    runEvents();
}

TEST_F(DebounceTest, RapidBouncingIgnored) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {1, {{0, 1, UP}}, {}},
        {2, {{0, 1, DOWN}}, {}},
        {3, {{0, 1, UP}}, {}},
        {4, {{0, 1, DOWN}}, {}},
        {5, {{0, 1, UP}}, {}},
        {6, {{0, 1, DOWN}}, {}},
        {7, {{0, 1, UP}}, {}},
        {8, {{0, 1, DOWN}}, {}},
        {9, {{0, 1, UP}}, {}},
        {10, {}, {}},
    });
    runEvents();
}

TEST_F(DebounceTest, FastBounceOnPress) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {1, {{0, 1, UP}}, {}},
        {2, {{0, 1, DOWN}}, {}},
        {7, {}, {{0, 1, DOWN}}},
    });
    runEvents();
}

TEST_F(DebounceTest, SlowBounceOnRelease) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {5, {}, {{0, 1, DOWN}}},
        {15, {{0, 1, UP}}, {}},
        {20, {{0, 1, DOWN}}, {}},
    });
    runEvents();
}

TEST_F(DebounceTest, MultipleInRowDontGhost) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 0, DOWN}}, {}},
        {5, {}, {{0, 0, DOWN}}},
        {10, {{0, 1, DOWN}}, {}},
        {15, {}, {{0, 1, DOWN}}},
        {20, {{0, 2, DOWN}}, {}},
        {25, {}, {{0, 2, DOWN}}},
    });
    runEvents();
}

TEST_F(DebounceTest, MultipleInColumnDontGhost) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 0, DOWN}}, {}},
        {5, {}, {{0, 0, DOWN}}},
        {10, {{1, 0, DOWN}}, {}},
        {15, {}, {{1, 0, DOWN}}},
        {20, {{2, 0, DOWN}}, {}},
        {25, {}, {{2, 0, DOWN}}},
    });
    runEvents();
}

TEST_F(DebounceTest, RowGhostsAreIgnored) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 0, DOWN}}, {}},
        {5, {{0, 1, DOWN}}, {{0, 0, DOWN}}},
        {10, {}, {{0, 1, DOWN}}},
        // Simulate ghost -- cannot tell whether {1, 0} or {1, 1} is pressed.
        {15, {{1, 0, DOWN}, {1, 1, DOWN}}, {}},
    });
    runEvents();
}

TEST_F(DebounceTest, RowOffsetGhostingIsIgnored) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 0, DOWN}}, {}},
        {5, {{0, 1, DOWN}}, {{0, 0, DOWN}}},
        {10, {}, {{0, 1, DOWN}}},
        // Simulate ghost -- cannot tell whether {1, 0} or {1, 1} is pressed, but
        // one column shows before the other.
        {15, {{1, 0, DOWN}}, {}},
        {16, {{1, 1, DOWN}}, {}},
        // Then one of them is up before the other.
        {25, {{1, 0, UP}}, {}},
        {26, {{1, 1, UP}}, {}},
    });
    runEvents();
}

TEST_F(DebounceTest, ColGhostsAreIgnored) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 0, DOWN}}, {}},
        {5, {{1, 0, DOWN}}, {{0, 0, DOWN}}},
        {10, {}, {{1, 0, DOWN}}},
        // Simulate ghost -- cannot tell whether {0, 1} or {1, 1} is pressed.
        {15, {{0, 1, DOWN}, {1, 1, DOWN}}, {}},
    });
    runEvents();
}

TEST_F(DebounceTest, ColOffsetGhostingIsIgnored) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 0, DOWN}}, {}},
        {5, {{1, 0, DOWN}}, {{0, 0, DOWN}}},
        {10, {}, {{1, 0, DOWN}}},
        // Simulate ghost -- cannot tell whether {1, 0} or {1, 1} is pressed, but
        // one column shows before the other.
        {15, {{0, 1, DOWN}}, {}},
        {16, {{1, 1, DOWN}}, {}},
        // Then one of them is up before the other.
        {25, {{0, 1, UP}}, {}},
        {26, {{1, 1, UP}}, {}},
    });
    runEvents();
}
