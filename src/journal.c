#include "journal.h"

#include <string.h>

static const uint8_t magic[8] = {'C', 'C', 'C', 'E', 'J', 'N', 'L', '1'};

static uint16_t read16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
}

static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void write16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void write32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t crc_update(uint32_t crc, const uint8_t *data, size_t size) {
    size_t i;
    for (i = 0; i < size; ++i) {
        unsigned bit;
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1u) ? UINT32_C(0xedb88320) : 0u);
    }
    return crc;
}

uint32_t journal_crc32(const uint8_t *data, size_t size) {
    return crc_update(UINT32_MAX, data, size) ^ UINT32_MAX;
}

static uint32_t record_crc(const uint8_t *record, size_t payload_size) {
    uint32_t crc = crc_update(UINT32_MAX, record, 16);
    return crc_update(crc, record + JOURNAL_HEADER_SIZE, payload_size) ^ UINT32_MAX;
}

size_t journal_encode(uint8_t *record, size_t capacity, const uint8_t *payload,
                      size_t payload_size, uint32_t generation) {
    if (!record || !payload || !payload_size ||
        payload_size > JOURNAL_MAX_PAYLOAD ||
        capacity < JOURNAL_HEADER_SIZE + payload_size) return 0;
    memcpy(record, magic, sizeof magic);
    write16(record + 8, 1);
    write16(record + 10, (uint16_t)payload_size);
    write32(record + 12, generation);
    memcpy(record + JOURNAL_HEADER_SIZE, payload, payload_size);
    write32(record + 16, record_crc(record, payload_size));
    return JOURNAL_HEADER_SIZE + payload_size;
}

bool journal_decode(const uint8_t *record, size_t record_size,
                    uint8_t *payload, size_t payload_size, uint32_t *generation) {
    if (!record || !payload || !generation || !payload_size ||
        payload_size > JOURNAL_MAX_PAYLOAD ||
        record_size != JOURNAL_HEADER_SIZE + payload_size ||
        memcmp(record, magic, sizeof magic) || read16(record + 8) != 1 ||
        read16(record + 10) != payload_size ||
        read32(record + 16) != record_crc(record, payload_size)) return false;
    memcpy(payload, record + JOURNAL_HEADER_SIZE, payload_size);
    *generation = read32(record + 12);
    return true;
}

typedef enum { SLOT_ABSENT, SLOT_VALID, SLOT_DAMAGED, SLOT_FOREIGN, SLOT_ERROR } SlotKind;
typedef struct {
    SlotKind kind;
    uint32_t generation;
    uint8_t record[JOURNAL_MAX_RECORD];
} Slot;

static void scan(const JournalIO *io, unsigned index, size_t payload_size, Slot *slot) {
    size_t size = 0;
    int result = io->read(io->ctx, index, slot->record, sizeof slot->record, &size);
    slot->generation = 0;
    if (result == 0) { slot->kind = SLOT_ABSENT; return; }
    if (result != 1) { slot->kind = SLOT_ERROR; return; }
    /* A partial magic prefix is owned, so interrupted first writes can heal.
     * An empty object or unknown bytes are protected as a name collision. */
    if (!size || memcmp(slot->record, magic, size < sizeof magic ? size : sizeof magic)) {
        slot->kind = SLOT_FOREIGN;
        return;
    }
    /* Future schema versions must never be overwritten by an older program. */
    if (size >= 10 && read16(slot->record + 8) != 1) {
        slot->kind = SLOT_FOREIGN;
        return;
    }
    slot->kind = SLOT_DAMAGED;
    if (size != JOURNAL_HEADER_SIZE + payload_size ||
        size > sizeof slot->record || read16(slot->record + 10) != payload_size ||
        read32(slot->record + 16) != record_crc(slot->record, payload_size)) return;
    if (io->validate && !io->validate(slot->record + JOURNAL_HEADER_SIZE,
                                     payload_size, io->ctx)) return;
    slot->generation = read32(slot->record + 12);
    slot->kind = SLOT_VALID;
}

static int newest(const Slot slots[2]) {
    uint32_t difference;
    if (slots[0].kind != SLOT_VALID) return slots[1].kind == SLOT_VALID ? 1 : -1;
    if (slots[1].kind != SLOT_VALID) return 0;
    /* Unsigned subtraction is defined across wraparound. Equal generations or
     * an impossible half-range ambiguity choose slot zero deterministically. */
    difference = slots[1].generation - slots[0].generation;
    return difference && difference < UINT32_C(0x80000000) ? 1 : 0;
}

static bool arguments_ok(const JournalIO *io, const uint8_t *payload,
                         size_t size, const uint32_t *generation) {
    return io && io->read && payload && generation && size &&
           size <= JOURNAL_MAX_PAYLOAD;
}

JournalResult journal_load(const JournalIO *io, uint8_t *payload,
                           size_t payload_size, uint32_t *generation) {
    Slot slots[2];
    int current;
    if (!arguments_ok(io, payload, payload_size, generation)) return JOURNAL_BAD_ARGUMENT;
    scan(io, 0, payload_size, &slots[0]);
    scan(io, 1, payload_size, &slots[1]);
    current = newest(slots);
    if (current >= 0) {
        SlotKind other = slots[1 - current].kind;
        memcpy(payload, slots[current].record + JOURNAL_HEADER_SIZE, payload_size);
        *generation = slots[current].generation;
        return other == SLOT_VALID || other == SLOT_ABSENT ? JOURNAL_OK : JOURNAL_RECOVERED;
    }
    if (slots[0].kind == SLOT_ERROR || slots[1].kind == SLOT_ERROR) return JOURNAL_IO_ERROR;
    if (slots[0].kind == SLOT_FOREIGN || slots[1].kind == SLOT_FOREIGN) return JOURNAL_COLLISION;
    if (slots[0].kind == SLOT_ABSENT && slots[1].kind == SLOT_ABSENT) return JOURNAL_EMPTY;
    return JOURNAL_CORRUPT;
}

JournalResult journal_commit(const JournalIO *io, const uint8_t *payload,
                             size_t payload_size, uint32_t *generation) {
    Slot slots[2];
    uint8_t record[JOURNAL_MAX_RECORD];
    uint32_t next;
    size_t size;
    int current;
    unsigned target;
    if (!arguments_ok(io, payload, payload_size, generation) || !io->write || !io->archive)
        return JOURNAL_BAD_ARGUMENT;
    if (io->validate && !io->validate(payload, payload_size, io->ctx)) return JOURNAL_BAD_ARGUMENT;
    scan(io, 0, payload_size, &slots[0]);
    scan(io, 1, payload_size, &slots[1]);
    if (slots[0].kind == SLOT_ERROR || slots[1].kind == SLOT_ERROR) return JOURNAL_IO_ERROR;
    if (slots[0].kind == SLOT_FOREIGN || slots[1].kind == SLOT_FOREIGN) return JOURNAL_COLLISION;
    current = newest(slots);
    target = current < 0 ? (slots[0].kind == SLOT_ABSENT ? 0u : 1u) : (unsigned)(1 - current);
    next = current < 0 ? 1u : slots[current].generation + 1u;
    /* A failed earlier archive can leave a VALID newer record in RAM. Make
     * that current record durable before replacing the previous durable one. */
    if (current >= 0) {
        size = JOURNAL_HEADER_SIZE + payload_size;
        memcpy(record, slots[current].record, size);
        if (!io->archive(io->ctx, (unsigned)current)) return JOURNAL_IO_ERROR;
        scan(io, (unsigned)current, payload_size, &slots[current]);
        if (slots[current].kind != SLOT_VALID ||
            memcmp(slots[current].record, record, size)) return JOURNAL_IO_ERROR;
    }
    size = journal_encode(record, sizeof record, payload, payload_size, next);
    if (!io->write(io->ctx, target, record, size)) return JOURNAL_IO_ERROR;
    scan(io, target, payload_size, &slots[target]);
    if (slots[target].kind != SLOT_VALID ||
        memcmp(slots[target].record, record, size)) return JOURNAL_IO_ERROR;
    if (!io->archive(io->ctx, target)) return JOURNAL_IO_ERROR;
    scan(io, target, payload_size, &slots[target]);
    if (slots[target].kind != SLOT_VALID ||
        memcmp(slots[target].record, record, size)) return JOURNAL_IO_ERROR;
    *generation = next;
    return JOURNAL_OK;
}
