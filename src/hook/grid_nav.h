/* Navigation helpers: no PS5 SDK dependencies, tested on the host. */
#pragma once

/* Change page while preserving the selected cell's position when possible.
 * Wraps at the beginning/end, including a partially populated last page. */
static inline int ld_grid_page_move(int cursor, int count, int per_page, int direction) {
    if (count <= 0 || per_page <= 0) return 0;
    if (cursor < 0) cursor = 0;
    if (cursor >= count) cursor = count - 1;
    int page_count = (count + per_page - 1) / per_page;
    int page = cursor / per_page;
    int within_page = cursor % per_page;
    int target = (page + (direction > 0 ? 1 : page_count - 1)) % page_count;
    int first = target * per_page;
    int available = count - first;
    if (available > per_page) available = per_page;
    return first + (within_page < available ? within_page : available - 1);
}

/* Continue past either end rather than getting stuck on the final item. */
static inline int ld_grid_step(int cursor, int count, int step) {
    if (count <= 0) return 0;
    int next = cursor + step;
    if (next < 0) return count - 1;
    if (next >= count) return 0;
    return next;
}
