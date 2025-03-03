/** @file api.cpp
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <e32std.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../api.h"

void update_time(void)
{
    static TInt start_time = 0;
    if (start_time == 0)
    {
        start_time = User::TickCount();
    }

    TInt current_time = User::TickCount();
    TInt elapsed_time_ticks = current_time - start_time;
    fix32_t elapsed_time = fix32_from_int(elapsed_time_ticks) / 1000;
    seconds_since_start = elapsed_time;
}

#ifdef __cplusplus
}
#endif
