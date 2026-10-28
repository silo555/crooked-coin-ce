#ifndef CC_JOURNAL_H
#define CC_JOURNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define JOURNAL_MAX_PAYLOAD 1024u
#define JOURNAL_HEADER_SIZE 20u
#define JOURNAL_MAX_RECORD (JOURNAL_HEADER_SIZE + JOURNAL_MAX_PAYLOAD)

typedef enum {
    JOURNAL_OK = 0,
    JOURNAL_EMPTY,
    JOURNAL_RECOVERED,
    JOURNAL_CORRUPT,
    JOURNAL_COLLISION,
    JOURNAL_IO_ERROR,
    JOURNAL_BAD_ARGUMENT
} JournalResult;

/* read returns 1 present, 0 missing, -1 I/O error. It copies min(size,capacity)
 * bytes and reports the FULL object size even if it exceeds capacity.
 * write replaces only the selected slot and may leave a partial record on
 * failure. archive must confirm durable storage, not merely queue a write.
 * validate is optional, pure, and must not modify the payload or backend.
 * No operation is permitted to modify the OTHER slot. */
typedef struct {
    void *ctx;
    int (*read)(void *ctx, unsigned slot, uint8_t *data, size_t capacity,
                size_t *actual_size);
    bool (*write)(void *ctx, unsigned slot, const uint8_t *data, size_t size);
    bool (*archive)(void *ctx, unsigned slot);
    bool (*validate)(const uint8_t *payload, size_t size, void *ctx);
} JournalIO;

/* Payload output and generation are unchanged on failure. EMPTY means both
 * names are absent. RECOVERED means one usable record and a damaged, foreign,
 * or inaccessible sibling (the valid record can still be played).
 * Do not silently replace a CORRUPT save with new-game defaults. */
JournalResult journal_load(const JournalIO *io, uint8_t *payload,
                           size_t payload_size, uint32_t *generation);

/* Writes the non-current slot, verifies it, archives it, then verifies again.
 * The previous valid record is retained throughout. Generation is read fresh
 * from storage; caller's generation is output-only. Foreign objects under
 * either reserved name cause COLLISION before any write. */
JournalResult journal_commit(const JournalIO *io, const uint8_t *payload,
                             size_t payload_size, uint32_t *generation);

/* Codec is exported for fixture generation and independent corruption tests.
 * Header layout: magic[8], version u16 LE, payload length u16 LE,
 * generation u32 LE, CRC32 u32 LE, payload. CRC covers header[0..15]+payload.
 * Version is 1; no native structs, padding, or target-sized integers on disk. */
size_t journal_encode(uint8_t *record, size_t capacity, const uint8_t *payload,
                      size_t payload_size, uint32_t generation);
bool journal_decode(const uint8_t *record, size_t record_size,
                    uint8_t *payload, size_t payload_size, uint32_t *generation);
uint32_t journal_crc32(const uint8_t *data, size_t size);

#endif
