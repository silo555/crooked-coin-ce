#ifndef CC_STORAGE_CE_H
#define CC_STORAGE_CE_H

#include "journal.h"

/* Install before any save operation while graphx is active. before_gc must
 * call gfx_End; after_gc must call gfx_Begin AND restore palette/draw buffer
 * state. Both callbacks may be NULL only when graphics are already ended.
 * No cached AppVar data pointers are retained by this adapter. */
void storage_ce_init(void (*before_gc)(void), void (*after_gc)(void));
void storage_ce_set_validator(bool (*validate)(const uint8_t *, size_t, void *));
JournalResult storage_ce_load(uint8_t *payload, size_t size, uint32_t *generation);
JournalResult storage_ce_save(const uint8_t *payload, size_t size, uint32_t *generation);

#endif
