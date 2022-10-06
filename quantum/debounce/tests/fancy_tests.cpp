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

#if 0

TEST_F(DebounceTest, OneKeyShort2) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {2, {}, {{0, 1, DOWN}}},
        /* 1ms delay */
        {3, {{0, 1, UP}}, {}},
        {5, {}, {{0, 1, UP}}},
    });
    runEvents();
}

TEST_F(DebounceTest, LongBounceUpIgnored) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {2, {}, {{0, 1, DOWN}}},
        {3, {{0, 1, UP}}, {}},
        {4, {}, {{0, 1, UP}}},
        /* 20ms delay */
        {24, {{0, 1, DOWN}}, {}},
        {25, {{0, 1, UP}}, {}},
}

TEST_F(DebounceTest, OneKeyShort3) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},

        {5, {}, {{0, 1, DOWN}}},
        /* 2ms delay */
        {7, {{0, 1, UP}}, {}},

        {12, {}, {{0, 1, UP}}},
    });
    runEvents();
}

TEST_F(DebounceTest, OneKeyTooQuick1) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        /* Release key exactly on the debounce time */
        {5, {{0, 1, UP}}, {}},
    });
    runEvents();
}

TEST_F(DebounceTest, OneKeyTooQuick2) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},

        {5, {}, {{0, 1, DOWN}}},
        {6, {{0, 1, UP}}, {}},

        /* Press key exactly on the debounce time */
        {11, {{0, 1, DOWN}}, {}},
    });
    runEvents();
}

TEST_F(DebounceTest, OneKeyBouncing1) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {1, {{0, 1, UP}}, {}},
        {2, {{0, 1, DOWN}}, {}},
        {3, {{0, 1, UP}}, {}},
        {4, {{0, 1, DOWN}}, {}},
        {5, {{0, 1, UP}}, {}},
        {6, {{0, 1, DOWN}}, {}},
        {11, {}, {{0, 1, DOWN}}}, /* 5ms after DOWN at time 7 */
    });
    runEvents();
}

TEST_F(DebounceTest, OneKeyBouncing2) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {5, {}, {{0, 1, DOWN}}},
        {6, {{0, 1, UP}}, {}},
        {7, {{0, 1, DOWN}}, {}},
        {8, {{0, 1, UP}}, {}},
        {9, {{0, 1, DOWN}}, {}},
        {10, {{0, 1, UP}}, {}},
        {15, {}, {{0, 1, UP}}}, /* 5ms after UP at time 10 */
    });
    runEvents();
}

TEST_F(DebounceTest, OneKeyLong) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},

        {5, {}, {{0, 1, DOWN}}},

        {25, {{0, 1, UP}}, {}},

        {30, {}, {{0, 1, UP}}},

        {50, {{0, 1, DOWN}}, {}},

        {55, {}, {{0, 1, DOWN}}},
    });
    runEvents();
}

TEST_F(DebounceTest, TwoKeysShort) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {1, {{0, 2, DOWN}}, {}},

        {6, {}, {{0, 1, DOWN}, {0, 2, DOWN}}},

        {7, {{0, 1, UP}}, {}},
        {8, {{0, 2, UP}}, {}},

        {13, {}, {{0, 1, UP}, {0, 2, UP}}},
    });
    runEvents();
}

TEST_F(DebounceTest, TwoKeysSimultaneous1) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}, {0, 2, DOWN}}, {}},

        {5, {}, {{0, 1, DOWN}, {0, 2, DOWN}}},
        {6, {{0, 1, UP}, {0, 2, UP}}, {}},

        {11, {}, {{0, 1, UP}, {0, 2, UP}}},
    });
    runEvents();
}

TEST_F(DebounceTest, TwoKeysSimultaneous2) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},
        {1, {{0, 2, DOWN}}, {}},

        {5, {}, {}},
        {6, {}, {{0, 1, DOWN}, {0, 2, DOWN}}},
        {7, {{0, 1, UP}}, {}},
        {8, {{0, 2, UP}}, {}},

        {13, {}, {{0, 1, UP}, {0, 2, UP}}},
    });
    runEvents();
}

TEST_F(DebounceTest, OneKeyDelayedScan1) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},

        /* Processing is very late */
        {300, {}, {{0, 1, DOWN}}},
        /* Immediately release key */
        {300, {{0, 1, UP}}, {}},

        {305, {}, {{0, 1, UP}}},
    });
    time_jumps_ = true;
    runEvents();
}

TEST_F(DebounceTest, OneKeyDelayedScan2) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},

        /* Processing is very late */
        {300, {}, {{0, 1, DOWN}}},
        /* Release key after 1ms */
        {301, {{0, 1, UP}}, {}},

        {306, {}, {{0, 1, UP}}},
    });
    time_jumps_ = true;
    runEvents();
}

TEST_F(DebounceTest, OneKeyDelayedScan3) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},

        /* Release key before debounce expires */
        {300, {{0, 1, UP}}, {}},
    });
    time_jumps_ = true;
    runEvents();
}

TEST_F(DebounceTest, OneKeyDelayedScan4) {
    addEvents({ /* Time, Inputs, Outputs */
        {0, {{0, 1, DOWN}}, {}},

        /* Processing is a bit late */
        {50, {}, {{0, 1, DOWN}}},
        /* Release key after 1ms */
        {51, {{0, 1, UP}}, {}},

        {56, {}, {{0, 1, UP}}},
    });
    time_jumps_ = true;
    runEvents();
}

#endif
