/* Left-stick to D-pad translation in the mod menu only.
 * Use axis dominance (one direction at a time) and a substantial dead zone
 * so analog noise never moves the selection on its own. */
#pragma once
#include <stdint.h>
#define LD_STICK_DEADZONE 48
static inline uint32_t ld_grid_stick_direction(uint8_t lx, uint8_t ly) {
    const int x = (int)lx - 128, y = (int)ly - 128;
    const int ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    if (ax < LD_STICK_DEADZONE && ay < LD_STICK_DEADZONE) return 0;
    if (ax >= ay) return (x < 0) ? 0x80u : 0x20u;
    return (y < 0) ? 0x10u : 0x40u;
}
