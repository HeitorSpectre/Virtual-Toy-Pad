/* Virtual LEGO Dimensions Toy Pad - freestanding protocol core.
 * No libc: usable inside the game process (hook module) and in host tests. */
#pragma once
#include <stdint.h>

#define LD_PAGES        45
#define LD_TAG_SIZE     (LD_PAGES * 4)
#define LD_MAX_SLOTS    7      /* 1 centre + 3 left + 3 right */
#define LD_QUEUE        32

/* Toy Pad zones as reported to the game: 1 = centre, 2 = left, 3 = right */
enum { LD_PAD_CENTER = 1, LD_PAD_LEFT = 2, LD_PAD_RIGHT = 3 };

typedef struct {
    uint8_t  used;
    uint8_t  pad;          /* 1..3 */
    uint8_t  index;        /* index reported to game (slot + 1 ordering) */
    uint8_t  dirty;        /* game wrote to tag (needs saving) */
    uint32_t id;           /* character (<1000) or vehicle/gadget id */
    uint8_t  data[LD_TAG_SIZE];
} ld_figure;

typedef struct { uint8_t b[32]; } ld_msg;

typedef struct {
    ld_figure fig[LD_MAX_SLOTS];
    ld_msg    q[LD_QUEUE];          /* replies + tag events for the IN endpoint */
    uint32_t  q_head, q_tail;
    uint32_t  rng_a, rng_b, rng_c, rng_d;
    uint32_t  uid_counter;
    uint8_t   awake;
    uint8_t   led[3][3];            /* last colour per zone, for the menu */
    uint32_t  stats_cmds, stats_unknown;
} ld_pad;

void ld_pad_init(ld_pad *p, uint32_t entropy);
/* Feed a 32-byte OUT report from the game. Queues the reply. */
void ld_pad_write(ld_pad *p, const uint8_t *req, uint32_t len);
/* Pop the next IN report (32 bytes). Returns 1 if a message was copied. */
int  ld_pad_read(ld_pad *p, uint8_t *out32);
int  ld_pad_pending(const ld_pad *p);

/* Figure management (used by the menu). slot 0 = centre, 1..3 left, 4..6 right */
int  ld_slot_pad(int slot);
int  ld_place_id(ld_pad *p, int slot, uint32_t fig_id);          /* builds a fresh tag */
int  ld_place_tag(ld_pad *p, int slot, const uint8_t *tag180);  /* uses an existing image */
int  ld_remove(ld_pad *p, int slot);
int  ld_move(ld_pad *p, int from, int to);
int  ld_find_free_slot(const ld_pad *p, int pad);

/* Crypto helpers (exported for tests/tools) */
void ld_tea_encrypt(const uint8_t in[8], uint8_t out[8], const uint8_t key[16]);
void ld_tea_decrypt(const uint8_t in[8], uint8_t out[8], const uint8_t key[16]);
void ld_figure_key(const uint8_t uid[7], uint8_t key[16]);
void ld_pwd(const uint8_t uid[7], uint8_t out[4]);
uint32_t ld_tag_figure_id(const uint8_t *tag180);
void ld_build_tag(uint8_t *tag180, const uint8_t uid[7], uint32_t fig_id);
void ld_fix_tag(uint8_t *tag180);
