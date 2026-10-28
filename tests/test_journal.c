#include "journal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { PAYLOAD_SIZE = 700 };
typedef struct {
    uint8_t data[JOURNAL_MAX_RECORD + 4];
    size_t size;
    bool exists, archived;
} Object;
typedef struct {
    Object slots[2];
    size_t write_limit;
    int read_error, archive_fail_at, archive_calls;
    bool corrupt_write, corrupt_archive;
    unsigned writes;
} Fake;

static int read_object(void *ctx, unsigned slot, uint8_t *out, size_t capacity, size_t *size) {
    Fake *f = ctx;
    Object *o = &f->slots[slot];
    assert(slot < 2);
    if (f->read_error == (int)slot) return -1;
    if (!o->exists) return 0;
    *size = o->size;
    memcpy(out, o->data, o->size < capacity ? o->size : capacity);
    return 1;
}

static bool write_object(void *ctx, unsigned slot, const uint8_t *data, size_t size) {
    Fake *f = ctx;
    Object *o = &f->slots[slot];
    size_t count = size < f->write_limit ? size : f->write_limit;
    assert(size <= JOURNAL_MAX_RECORD && slot < 2);
    ++f->writes;
    o->exists = true;
    o->archived = false;
    o->size = count;
    memcpy(o->data, data, count);
    if (f->corrupt_write && count) o->data[count - 1] ^= 1;
    return count == size;
}

static bool archive_object(void *ctx, unsigned slot) {
    Fake *f = ctx;
    assert(slot < 2);
    ++f->archive_calls;
    if (f->archive_calls == f->archive_fail_at) return false;
    if (f->corrupt_archive) f->slots[slot].data[19] ^= 1;
    f->slots[slot].archived = true;
    return true;
}

static bool valid_payload(const uint8_t *p, size_t size, void *ctx) {
    (void)ctx;
    return size == PAYLOAD_SIZE && p[0] <= 100;
}

static JournalIO backend(Fake *f) {
    JournalIO io = {f, read_object, write_object, archive_object, valid_payload};
    return io;
}

static void init(Fake *f) {
    memset(f, 0, sizeof *f);
    f->write_limit = JOURNAL_MAX_RECORD;
    f->read_error = -1;
    f->archive_fail_at = -1;
}

static void payload(uint8_t *p, unsigned value) {
    size_t i;
    for (i = 0; i < PAYLOAD_SIZE; ++i) p[i] = (uint8_t)(value + i * 29u);
    p[0] = (uint8_t)value;
}

static void seed(Fake *f, unsigned slot, unsigned value, uint32_t gen) {
    uint8_t p[PAYLOAD_SIZE];
    payload(p, value);
    f->slots[slot].size = journal_encode(f->slots[slot].data,
        sizeof f->slots[slot].data, p, sizeof p, gen);
    assert(f->slots[slot].size == JOURNAL_HEADER_SIZE + PAYLOAD_SIZE);
    f->slots[slot].exists = f->slots[slot].archived = true;
}

static void power_loss(Fake *f) {
    unsigned i;
    for (i = 0; i < 2; ++i)
        if (!f->slots[i].archived) f->slots[i].exists = false;
}

static void expect_value(Fake *f, unsigned value, uint32_t generation) {
    uint8_t out[PAYLOAD_SIZE], expected[PAYLOAD_SIZE];
    uint32_t gen = 99;
    JournalIO io = backend(f);
    JournalResult result = journal_load(&io, out, sizeof out, &gen);
    assert(result == JOURNAL_OK || result == JOURNAL_RECOVERED);
    payload(expected, value);
    assert(gen == generation && !memcmp(out, expected, sizeof out));
}

static void codec_tests(void) {
    uint8_t p[PAYLOAD_SIZE], out[PAYLOAD_SIZE], bytes[JOURNAL_MAX_RECORD];
    size_t size, i;
    unsigned bit;
    uint32_t gen;
    assert(journal_crc32((const uint8_t *)"123456789", 9) == UINT32_C(0xcbf43926));
    payload(p, 12);
    size = journal_encode(bytes, sizeof bytes, p, sizeof p, UINT32_C(0xfedcba98));
    assert(size == PAYLOAD_SIZE + JOURNAL_HEADER_SIZE);
    assert(bytes[8] == 1 && bytes[9] == 0 && bytes[10] == 0xbc && bytes[11] == 2);
    assert(bytes[12] == 0x98 && bytes[13] == 0xba && bytes[14] == 0xdc && bytes[15] == 0xfe);
    assert(journal_decode(bytes, size, out, sizeof out, &gen));
    assert(gen == UINT32_C(0xfedcba98) && !memcmp(out, p, sizeof p));
    for (i = 0; i < size; ++i) for (bit = 0; bit < 8; ++bit) {
        bytes[i] ^= (uint8_t)(1u << bit);
        memset(out, 0xa5, sizeof out); gen = 71;
        assert(!journal_decode(bytes, size, out, sizeof out, &gen));
        assert(out[0] == 0xa5 && gen == 71);
        bytes[i] ^= (uint8_t)(1u << bit);
    }
    for (i = 0; i < size; ++i) assert(!journal_decode(bytes, i, out, sizeof out, &gen));
    assert(!journal_decode(bytes, size + 1, out, sizeof out, &gen));
    assert(!journal_encode(bytes, size - 1, p, sizeof p, 1));
    assert(!journal_encode(bytes, sizeof bytes, p, 0, 1));
    assert(!journal_encode(bytes, sizeof bytes, p, JOURNAL_MAX_PAYLOAD + 1, 1));
}

static void selection_tests(void) {
    Fake f;
    JournalIO io;
    uint8_t out[PAYLOAD_SIZE], p[PAYLOAD_SIZE];
    uint32_t gen = 55;
    init(&f); io = backend(&f); memset(out, 0xa5, sizeof out);
    assert(journal_load(&io, out, sizeof out, &gen) == JOURNAL_EMPTY);
    assert(out[0] == 0xa5 && gen == 55);
    seed(&f, 0, 1, UINT32_MAX - 1);
    seed(&f, 1, 2, UINT32_MAX);
    expect_value(&f, 2, UINT32_MAX);
    payload(p, 3);
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_OK && gen == 0);
    expect_value(&f, 3, 0);
    assert(f.slots[1].archived);
    payload(p, 4);
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_OK && gen == 1);
    expect_value(&f, 4, 1);
    /* Validate semantic payloads even when their CRC is correct. */
    seed(&f, 0, 200, 2);
    assert(journal_load(&io, out, sizeof out, &gen) == JOURNAL_RECOVERED);
    expect_value(&f, 4, 1);
    assert(journal_commit(&io, f.slots[0].data + JOURNAL_HEADER_SIZE,
                          PAYLOAD_SIZE, &gen) == JOURNAL_BAD_ARGUMENT);
    seed(&f, 1, 200, 3);
    memset(out, 0xa5, sizeof out); gen = 55;
    assert(journal_load(&io, out, sizeof out, &gen) == JOURNAL_CORRUPT);
    assert(out[0] == 0xa5 && gen == 55);
}

static void interruption_tests(void) {
    size_t cut;
    uint8_t p[PAYLOAD_SIZE];
    payload(p, 3);
    for (cut = 0; cut < JOURNAL_HEADER_SIZE + PAYLOAD_SIZE; ++cut) {
        Fake f;
        JournalIO io;
        Object previous;
        uint32_t gen = 999;
        init(&f); seed(&f, 0, 1, 1); seed(&f, 1, 2, 2);
        previous = f.slots[1]; io = backend(&f); f.write_limit = cut;
        assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_IO_ERROR);
        assert(gen == 999 && !memcmp(&previous, &f.slots[1], sizeof previous));
        expect_value(&f, 2, 2);
        /* An interrupted write never damages the prior archived slot. */
        power_loss(&f);
        expect_value(&f, 2, 2);
        f.write_limit = JOURNAL_MAX_RECORD;
        assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_OK);
        expect_value(&f, 3, 3);
        power_loss(&f); expect_value(&f, 3, 3);
    }
}

static void corruption_recovery_tests(void) {
    size_t i;
    unsigned bit;
    for (i = 0; i < JOURNAL_HEADER_SIZE + PAYLOAD_SIZE; ++i) for (bit = 0; bit < 8; ++bit) {
        Fake f;
        init(&f); seed(&f, 0, 1, 20); seed(&f, 1, 2, 21);
        f.slots[1].data[i] ^= (uint8_t)(1u << bit);
        expect_value(&f, 1, 20);
    }
    /* Every truncated newer record falls back, including an empty object. */
    for (i = 0; i < JOURNAL_HEADER_SIZE + PAYLOAD_SIZE; ++i) {
        Fake f;
        init(&f); seed(&f, 0, 1, 20); seed(&f, 1, 2, 21);
        f.slots[1].size = i;
        expect_value(&f, 1, 20);
    }
}

static void failure_tests(void) {
    Fake f;
    JournalIO io;
    uint8_t p[PAYLOAD_SIZE], out[PAYLOAD_SIZE];
    uint32_t gen;
    Object previous;
    init(&f); io = backend(&f); payload(p, 3);
    seed(&f, 0, 1, 1); seed(&f, 1, 2, 2); previous = f.slots[1];
    f.corrupt_write = true;
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_IO_ERROR);
    assert(!memcmp(&previous, &f.slots[1], sizeof previous)); expect_value(&f, 2, 2);
    f.corrupt_write = false; f.archive_calls = 0; f.archive_fail_at = 2;
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_IO_ERROR);
    assert(!f.slots[0].archived && f.slots[1].archived);
    expect_value(&f, 3, 3);
    /* A retry must first archive this valid RAM slot before overwriting the
     * previous durable slot. If that archive fails, there is no new write. */
    f.archive_calls = 0; f.archive_fail_at = 1; f.writes = 0;
    payload(p, 4);
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_IO_ERROR);
    assert(f.writes == 0 && !memcmp(&previous, &f.slots[1], sizeof previous));
    power_loss(&f); expect_value(&f, 2, 2);
    f.archive_fail_at = -1; f.read_error = 0; f.writes = 0;
    assert(journal_load(&io, out, sizeof out, &gen) == JOURNAL_RECOVERED);
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_IO_ERROR && f.writes == 0);
    f.slots[1].exists = false;
    assert(journal_load(&io, out, sizeof out, &gen) == JOURNAL_IO_ERROR);
    init(&f); seed(&f, 0, 1, 1); f.corrupt_archive = true;
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_IO_ERROR);
    assert(f.writes == 0); /* Current archive readback itself was corrupted. */
}

static void collision_tests(void) {
    Fake f;
    JournalIO io;
    uint8_t p[PAYLOAD_SIZE], out[PAYLOAD_SIZE];
    uint32_t gen;
    Object original;
    init(&f); io = backend(&f); payload(p, 3);
    f.slots[0].exists = true; f.slots[0].size = 5;
    memcpy(f.slots[0].data, "OTHER", 5); original = f.slots[0];
    assert(journal_load(&io, out, sizeof out, &gen) == JOURNAL_COLLISION);
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_COLLISION);
    assert(f.writes == 0 && !memcmp(&original, &f.slots[0], sizeof original));
    seed(&f, 1, 2, 2);
    assert(journal_load(&io, out, sizeof out, &gen) == JOURNAL_RECOVERED);
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_COLLISION && f.writes == 0);
    /* Empty names and saves from future versions are also protected. */
    f.slots[0].size = 0;
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_COLLISION);
    seed(&f, 0, 1, 1); f.slots[0].data[8] = 2;
    assert(journal_commit(&io, p, sizeof p, &gen) == JOURNAL_COLLISION);
    /* Oversized object cannot overflow the scan buffer. */
    seed(&f, 0, 1, 1); f.slots[0].size = JOURNAL_MAX_RECORD + 1;
    assert(journal_load(&io, out, sizeof out, &gen) == JOURNAL_RECOVERED);
    assert(journal_load(NULL, out, sizeof out, &gen) == JOURNAL_BAD_ARGUMENT);
    assert(journal_load(&io, out, JOURNAL_MAX_PAYLOAD + 1, &gen) == JOURNAL_BAD_ARGUMENT);
}

int main(void) {
    codec_tests(); selection_tests(); interruption_tests();
    corruption_recovery_tests(); failure_tests(); collision_tests();
    puts("journal: CRC known vector, 11520 bit mutations, 2160 truncation/interruption cases, wrap, semantic validation, readback/archive failures and foreign-name protection passed");
    return 0;
}
