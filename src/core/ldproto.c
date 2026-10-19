/* Virtual LEGO Dimensions Toy Pad - freestanding protocol core.
 * Protocol and crypto follow the RPCS3/shadPS4 Dimensions emulation and node-ld,
 * validated against the PS4 build of the game (CUSA00935 v1.23). */
#include "ldproto.h"

static const uint8_t COMMAND_KEY[16] = {0x55, 0xFE, 0xF6, 0xB0, 0x62, 0xBF, 0x0B, 0x41,
                                        0xC9, 0xB3, 0x7C, 0xB4, 0x97, 0x3E, 0x29, 0x7B};
static const uint8_t CHAR_CONSTANT[17] = {0xB7, 0xD5, 0xD7, 0xE6, 0xE7, 0xBA, 0x3C, 0xA8, 0xD8,
                                          0x75, 0x47, 0x68, 0xCF, 0x23, 0xE9, 0xFE, 0xAA};
/* "(c) Copyright LEGO 2014" + 0xAA 0xAA */
static const uint8_t PWD_CONSTANT[25] = {0x28, 0x63, 0x29, 0x20, 0x43, 0x6F, 0x70, 0x79, 0x72,
                                         0x69, 0x67, 0x68, 0x74, 0x20, 0x4C, 0x45, 0x47, 0x4F,
                                         0x20, 0x32, 0x30, 0x31, 0x34, 0xAA, 0xAA};
/* WAKE reply captured from a real PS4 Toy Pad (0E6F:0241) talking to the PS4 game:
 * status, hardware/protocol/firmware versions, 12-byte portal UID.
 * (RPCS3/shadPS4 answer "(c) LEGO 2014" here, which the PS4 build v1.23 rejects and keeps resetting.) */
static const uint8_t WAKE_REPLY[24] = {0x00, 0x2f, 0x02, 0x01, 0x02, 0x02, 0x04, 0x02,
                                       0xf5, 0x00, 0x19, 0x87, 0x54, 0xfd, 0xbe, 0xeb,
                                       0xae, 0x4e, 0x90, 0xe4, 0x0e, 0x00, 0x10, 0x0c};

static void mcpy(void *d, const void *s, uint32_t n) {
    uint8_t *dd = (uint8_t *)d; const uint8_t *ss = (const uint8_t *)s;
    while (n--) *dd++ = *ss++;
}
static void mset(void *d, uint8_t v, uint32_t n) {
    uint8_t *dd = (uint8_t *)d;
    while (n--) *dd++ = v;
}
static uint32_t rd32le(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}
static void wr32le(uint8_t *b, uint32_t v) {
    b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); b[2] = (uint8_t)(v >> 16); b[3] = (uint8_t)(v >> 24);
}
static uint32_t rotl(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }
static uint32_t rotr(uint32_t v, int n) { return (v >> n) | (v << (32 - n)); }
static uint8_t checksum(const uint8_t *b, uint32_t n) {
    uint32_t s = 0;
    while (n--) s += *b++;
    return (uint8_t)s;
}

/* ---------------------------------------------------------------- crypto */

void ld_tea_encrypt(const uint8_t in[8], uint8_t out[8], const uint8_t key[16]) {
    uint32_t v0 = rd32le(in), v1 = rd32le(in + 4), s = 0;
    uint32_t k0 = rd32le(key), k1 = rd32le(key + 4), k2 = rd32le(key + 8), k3 = rd32le(key + 12);
    for (int i = 0; i < 32; i++) {
        s += 0x9E3779B9u;
        v0 += ((v1 << 4) + k0) ^ (v1 + s) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + s) ^ ((v0 >> 5) + k3);
    }
    wr32le(out, v0); wr32le(out + 4, v1);
}

void ld_tea_decrypt(const uint8_t in[8], uint8_t out[8], const uint8_t key[16]) {
    uint32_t v0 = rd32le(in), v1 = rd32le(in + 4), s = 0xC6EF3720u;
    uint32_t k0 = rd32le(key), k1 = rd32le(key + 4), k2 = rd32le(key + 8), k3 = rd32le(key + 12);
    for (int i = 0; i < 32; i++) {
        v1 -= ((v0 << 4) + k2) ^ (v0 + s) ^ ((v0 >> 5) + k3);
        v0 -= ((v1 << 4) + k0) ^ (v1 + s) ^ ((v1 >> 5) + k1);
        s -= 0x9E3779B9u;
    }
    wr32le(out, v0); wr32le(out + 4, v1);
}

static uint32_t randomize(const uint8_t *key, int count) {
    uint32_t sc = 0;
    for (int i = 0; i < count; i++)
        sc = rd32le(key + i * 4) + rotr(sc, 25) + rotr(sc, 10) - sc;
    return sc;
}

static uint32_t scramble(const uint8_t uid[7], int count) {
    uint8_t buf[24];
    mcpy(buf, uid, 7);
    mcpy(buf + 7, CHAR_CONSTANT, 17);
    buf[count * 4 - 1] = 0xAA;
    uint32_t v = randomize(buf, count);
    /* python/C++ reference reads the 4 LE bytes back as big endian */
    return ((v & 0xFF) << 24) | (((v >> 8) & 0xFF) << 16) | (((v >> 16) & 0xFF) << 8) | (v >> 24);
}

void ld_figure_key(const uint8_t uid[7], uint8_t key[16]) {
    for (int i = 0; i < 4; i++) {
        uint32_t s = scramble(uid, 3 + i);
        key[i * 4 + 0] = (uint8_t)(s >> 24); key[i * 4 + 1] = (uint8_t)(s >> 16);
        key[i * 4 + 2] = (uint8_t)(s >> 8);  key[i * 4 + 3] = (uint8_t)s;
    }
}

void ld_pwd(const uint8_t uid[7], uint8_t out[4]) {
    uint8_t buf[32];
    mcpy(buf, uid, 7);
    mcpy(buf + 7, PWD_CONSTANT, 25);   /* uid + "(c) Copyright LEGO 2014" + AA AA = 32 bytes */
    wr32le(out, randomize(buf, 8));
}

static void tag_uid(const uint8_t *tag, uint8_t uid[7]) {
    uid[0] = tag[0]; uid[1] = tag[1]; uid[2] = tag[2];
    uid[3] = tag[4]; uid[4] = tag[5]; uid[5] = tag[6]; uid[6] = tag[7];
}

uint32_t ld_tag_figure_id(const uint8_t *tag) {
    uint8_t uid[7], key[16], dec[8];
    tag_uid(tag, uid);
    ld_figure_key(uid, key);
    ld_tea_decrypt(tag + 0x24 * 4, dec, key);
    uint32_t n = rd32le(dec);
    if (n < 1000) return n;
    return rd32le(tag + 0x24 * 4);
}

/* repair images made by older builds: vehicle tags without the page 0x26 vehicle marker */
void ld_fix_tag(uint8_t *d) {
    uint32_t id = ld_tag_figure_id(d);
    if (id >= 1000 && d[0x26 * 4] == 0 && d[0x26 * 4 + 1] == 0) d[0x26 * 4 + 1] = 0x01;
}

void ld_build_tag(uint8_t *d, const uint8_t uid[7], uint32_t fig_id) {
    mset(d, 0, LD_TAG_SIZE);
    d[0] = uid[0]; d[1] = uid[1]; d[2] = uid[2];
    d[3] = 0x88 ^ uid[0] ^ uid[1] ^ uid[2];
    d[4] = uid[3]; d[5] = uid[4]; d[6] = uid[5]; d[7] = uid[6];
    d[8] = uid[3] ^ uid[4] ^ uid[5] ^ uid[6];
    d[9] = 0x48; d[10] = 0x0F; d[11] = 0xE0;
    d[12] = 0xF1; d[13] = 0x10; d[14] = 0xFF; d[15] = 0xEE;
    d[16] = 0xA5;
    /* pages 0x1a-0x1c: factory serial in ASCII, like real tags ("9606234R2915") */
    {
        static const char hex[] = "0123456789";
        uint32_t v = (uint32_t)uid[1] << 16 | (uint32_t)uid[2] << 8 | uid[3];
        char *sn = (char *)d + 0x1a * 4;
        sn[0] = '9'; sn[1] = '6'; sn[2] = '0'; sn[3] = '6';
        for (int i = 0; i < 3; i++) { sn[4 + i] = hex[v % 10]; v /= 10; }
        sn[7] = 'R';
        for (int i = 0; i < 4; i++) { sn[8 + i] = hex[v % 10]; v /= 10; }
    }
    if (fig_id < 1000) {
        uint8_t key[16], plain[8];
        ld_figure_key(uid, key);
        wr32le(plain, fig_id); wr32le(plain + 4, fig_id);
        ld_tea_encrypt(plain, d + 0x24 * 4, key);
    } else {
        /* vehicles/gadgets: 0x23 = upgrades part 1, 0x24 = model id (LE), 0x25 = upgrades part 2,
         * 0x26 = 00 01 00 00 marks the tag as a vehicle (a zero page 0x26 means "character") */
        wr32le(d + 0x24 * 4, fig_id);
        d[0x26 * 4 + 1] = 0x01;
    }
    /* lock / config pages as read from a real tag: 60 0c 3f bd | 04 00 00 1e */
    d[0x28 * 4 + 0] = 0x60; d[0x28 * 4 + 1] = 0x0C; d[0x28 * 4 + 2] = 0x3F; d[0x28 * 4 + 3] = 0xBD;
    d[0x29 * 4 + 0] = 0x04; d[0x29 * 4 + 3] = 0x1E;
    d[0x2A * 4 + 0] = 0x5F;
    ld_pwd(uid, d + 0x2B * 4);
}

/* ------------------------------------------------------------ RNG (burtle) */

static uint32_t rng_next(ld_pad *p) {
    uint32_t e = p->rng_a - rotl(p->rng_b, 21);
    p->rng_a = p->rng_b ^ rotl(p->rng_c, 19);
    p->rng_b = p->rng_c + rotl(p->rng_d, 6);
    p->rng_c = p->rng_d + e;
    p->rng_d = e + p->rng_a;
    return p->rng_d;
}

static void rng_seed(ld_pad *p, uint32_t seed) {
    p->rng_a = 0xF1EA5EEDu;
    p->rng_b = p->rng_c = p->rng_d = seed;
    for (int i = 0; i < 42; i++) rng_next(p);
}

/* ----------------------------------------------------------------- queue */

static ld_msg *q_push(ld_pad *p) {
    if (p->q_tail - p->q_head >= LD_QUEUE) p->q_head++; /* drop oldest */
    ld_msg *m = &p->q[p->q_tail % LD_QUEUE];
    mset(m->b, 0, 32);
    p->q_tail++;
    return m;
}

int ld_pad_pending(const ld_pad *p) { return p->q_tail != p->q_head; }

int ld_pad_read(ld_pad *p, uint8_t *out) {
    if (p->q_head == p->q_tail) return 0;
    mcpy(out, p->q[p->q_head % LD_QUEUE].b, 32);
    p->q_head++;
    return 1;
}

/* reply: 55 <len> <seq> <payload...> <chk> ; len = 1 + payload */
static void reply(ld_pad *p, uint8_t seq, const uint8_t *payload, uint32_t n) {
    ld_msg *m = q_push(p);
    m->b[0] = 0x55;
    m->b[1] = (uint8_t)(n + 1);
    m->b[2] = seq;
    if (n) mcpy(m->b + 3, payload, n);
    m->b[3 + n] = checksum(m->b, 3 + n);
}

static void tag_event(ld_pad *p, const ld_figure *f, int removed) {
    ld_msg *m = q_push(p);
    m->b[0] = 0x56; m->b[1] = 0x0B;
    m->b[2] = f->pad; m->b[3] = 0;
    m->b[4] = f->index; m->b[5] = removed ? 1 : 0;
    m->b[6] = f->data[0]; m->b[7] = f->data[1]; m->b[8] = f->data[2];
    m->b[9] = f->data[4]; m->b[10] = f->data[5]; m->b[11] = f->data[6]; m->b[12] = f->data[7];
    m->b[13] = checksum(m->b, 13);
}

/* ----------------------------------------------------------- figure slots */

int ld_slot_pad(int slot) {
    if (slot == 0) return LD_PAD_CENTER;
    return slot <= 3 ? LD_PAD_LEFT : LD_PAD_RIGHT;
}

int ld_find_free_slot(const ld_pad *p, int pad) {
    for (int s = 0; s < LD_MAX_SLOTS; s++)
        if (ld_slot_pad(s) == pad && !p->fig[s].used) return s;
    return -1;
}

static ld_figure *by_index(ld_pad *p, uint8_t index) {
    /* the real PS4 Toy Pad reports 0-based tag indices (captured: 56 0b 02 00 00 00 <uid>) */
    for (int s = 0; s < LD_MAX_SLOTS; s++)
        if (p->fig[s].used && p->fig[s].index == index) return &p->fig[s];
    return 0;
}

static uint8_t free_index(const ld_pad *p) {
    for (uint8_t i = 0; i < LD_MAX_SLOTS; i++) {
        int taken = 0;
        for (int s = 0; s < LD_MAX_SLOTS; s++) if (p->fig[s].used && p->fig[s].index == i) taken = 1;
        if (!taken) return i;
    }
    return 0;
}

void ld_pad_init(ld_pad *p, uint32_t entropy) {
    mset(p, 0, sizeof(*p));
    p->uid_counter = entropy ? entropy : 0x1234567u;
}

static void new_uid(ld_pad *p, uint8_t uid[7]) {
    /* xorshift for unique-looking NXP UIDs (04 xx xx xx xx xx 80) */
    uint32_t x = p->uid_counter;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    uint32_t y = x * 2654435761u;
    p->uid_counter = x;
    uid[0] = 0x04;
    uid[1] = (uint8_t)x; uid[2] = (uint8_t)(x >> 8); uid[3] = (uint8_t)(x >> 16);
    uid[4] = (uint8_t)(x >> 24); uid[5] = (uint8_t)(y >> 8);
    uid[6] = 0x80;
}

int ld_place_tag(ld_pad *p, int slot, const uint8_t *tag) {
    if (slot < 0 || slot >= LD_MAX_SLOTS) return -1;
    if (p->fig[slot].used) ld_remove(p, slot);
    ld_figure *f = &p->fig[slot];
    mcpy(f->data, tag, LD_TAG_SIZE);
    ld_fix_tag(f->data);
    f->id = ld_tag_figure_id(tag);
    f->pad = (uint8_t)ld_slot_pad(slot);
    f->index = free_index(p);
    f->dirty = 0;
    f->used = 1;
    tag_event(p, f, 0);
    return 0;
}

int ld_place_id(ld_pad *p, int slot, uint32_t fig_id) {
    uint8_t uid[7], tag[LD_TAG_SIZE];
    new_uid(p, uid);
    ld_build_tag(tag, uid, fig_id);
    return ld_place_tag(p, slot, tag);
}

int ld_remove(ld_pad *p, int slot) {
    if (slot < 0 || slot >= LD_MAX_SLOTS || !p->fig[slot].used) return -1;
    tag_event(p, &p->fig[slot], 1);
    p->fig[slot].used = 0;
    return 0;
}

int ld_move(ld_pad *p, int from, int to) {
    if (from == to || from < 0 || to < 0 || from >= LD_MAX_SLOTS || to >= LD_MAX_SLOTS) return -1;
    if (!p->fig[from].used) return -1;
    uint8_t tag[LD_TAG_SIZE];
    uint8_t dirty = p->fig[from].dirty;
    mcpy(tag, p->fig[from].data, LD_TAG_SIZE);
    if (p->fig[to].used) ld_remove(p, to);
    ld_remove(p, from);
    ld_place_tag(p, to, tag);
    p->fig[to].dirty = dirty;
    return 0;
}

/* ------------------------------------------------------------- commands */

void ld_pad_write(ld_pad *p, const uint8_t *req, uint32_t len) {
    if (len < 4 || req[0] != 0x55) return;
    uint8_t cmd = req[2], seq = req[3];
    const uint8_t *pl = req + 4;
    uint8_t out[24], tmp[8];
    p->stats_cmds++;

    switch (cmd) {
    case 0xB0: /* wake: reply with portal info, then re-announce every tag */
        p->awake = 1;
        reply(p, seq, WAKE_REPLY, sizeof(WAKE_REPLY));
        for (int s = 0; s < LD_MAX_SLOTS; s++)
            if (p->fig[s].used) tag_event(p, &p->fig[s], 0);
        break;
    case 0xB1: { /* seed: payload = TEA(seed LE, conf BE) -> reply TEA(conf, 0) */
        ld_tea_decrypt(pl, tmp, COMMAND_KEY);
        rng_seed(p, rd32le(tmp));
        uint8_t v[8] = {tmp[4], tmp[5], tmp[6], tmp[7], 0, 0, 0, 0};
        ld_tea_encrypt(v, out, COMMAND_KEY);
        reply(p, seq, out, 8);
        break;
    }
    case 0xB3: { /* challenge: reply TEA(next_rand LE, conf) */
        ld_tea_decrypt(pl, tmp, COMMAND_KEY);
        uint8_t v[8];
        wr32le(v, rng_next(p));
        v[4] = tmp[0]; v[5] = tmp[1]; v[6] = tmp[2]; v[7] = tmp[3];
        ld_tea_encrypt(v, out, COMMAND_KEY);
        reply(p, seq, out, 8);
        break;
    }
    case 0xC0: case 0xC2: case 0xC3: { /* colour/fade/flash one zone: pad, r, g, b ... */
        uint8_t z = pl[0];
        const uint8_t *rgb = cmd == 0xC0 ? pl + 1 : pl + 3;
        if (z >= 1 && z <= 3) { p->led[z - 1][0] = rgb[0]; p->led[z - 1][1] = rgb[1]; p->led[z - 1][2] = rgb[2]; }
        else if (z == 0) for (int i = 0; i < 3; i++) { p->led[i][0] = rgb[0]; p->led[i][1] = rgb[1]; p->led[i][2] = rgb[2]; }
        reply(p, seq, 0, 0);
        break;
    }
    case 0xC1: { /* get colour of a zone */
        uint8_t z = pl[0];
        uint8_t c[3] = {0, 0, 0};
        if (z >= 1 && z <= 3) { c[0] = p->led[z - 1][0]; c[1] = p->led[z - 1][1]; c[2] = p->led[z - 1][2]; }
        reply(p, seq, c, 3);
        break;
    }
    case 0xC4: case 0xC6: case 0xC7: case 0xC8: /* random/all-zone variants */
        if (cmd == 0xC8) /* colour all: [on,r,g,b] x3 */
            for (int i = 0; i < 3; i++) if (pl[i * 4]) { p->led[i][0] = pl[i * 4 + 1]; p->led[i][1] = pl[i * 4 + 2]; p->led[i][2] = pl[i * 4 + 3]; }
        reply(p, seq, 0, 0);
        break;
    case 0xD0: { /* tag list: pairs of (pad<<4 | index, flags) for every tag present */
        uint32_t n = 0;
        for (int s = 0; s < LD_MAX_SLOTS; s++)
            if (p->fig[s].used) { out[n++] = (uint8_t)((p->fig[s].pad << 4) | p->fig[s].index); out[n++] = 0x09; }
        reply(p, seq, out, n);
        break;
    }
    case 0xD2: { /* read 4 pages: [index, page] -> status + 16 bytes */
        ld_figure *f = by_index(p, pl[0]);
        uint32_t page = pl[1];
        mset(out, 0, 17);
        if (f && page < LD_PAGES) {
            for (uint32_t i = 0; i < 16; i++) {
                uint32_t off = page * 4 + i;
                out[1 + i] = off < LD_TAG_SIZE ? f->data[off] : 0;
            }
        } else out[0] = 0x01; /* no tag */
        reply(p, seq, out, 17);
        break;
    }
    case 0xD3: { /* write one page: [index, page, b0..b3] -> status */
        ld_figure *f = by_index(p, pl[0]);
        uint32_t page = pl[1];
        uint8_t st = 1;
        if (f && page >= 4 && page < 0x28) { /* user pages only */
            mcpy(f->data + page * 4, pl + 2, 4);
            if (page == 0x24 || page == 0x25) f->id = ld_tag_figure_id(f->data);
            f->dirty = 1;
            st = 0;
        }
        reply(p, seq, &st, 1);
        break;
    }
    case 0xD4: { /* model: TEA(index, .., conf) -> status + TEA(id LE, conf) */
        ld_tea_decrypt(pl, tmp, COMMAND_KEY);
        ld_figure *f = by_index(p, tmp[0]);
        uint8_t v[8] = {0, 0, 0, 0, tmp[4], tmp[5], tmp[6], tmp[7]};
        mset(out, 0, 9);
        if (f) wr32le(v, f->id);
        else out[0] = 0xF2;
        ld_tea_encrypt(v, out + 1, COMMAND_KEY);
        reply(p, seq, out, 9);
        break;
    }
    case 0xE1: /* set password mode */
    case 0xE5: /* set active / tag polling */
    case 0xFF: /* LED query */
    default:
        if (cmd != 0xE1 && cmd != 0xE5 && cmd != 0xFF) p->stats_unknown++;
        reply(p, seq, 0, 0);
        break;
    }
}
