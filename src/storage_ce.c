#include "storage_ce.h"

#include <fileioc.h>
#include <string.h>

static const char *const names[2] = {"CCSAVE0", "CCSAVE1"};

static int read_slot(void *ctx, unsigned slot, uint8_t *data, size_t capacity,
                     size_t *actual_size) {
    uint8_t handle;
    size_t count;
    bool good;
    (void)ctx;
    handle = ti_Open(names[slot], "r");
    if (!handle) {
        void *search = NULL;
        char *name;
        while ((name = ti_Detect(&search, "")) != NULL)
            if (!strcmp(name, names[slot])) return -1;
        return 0;
    }
    *actual_size = ti_GetSize(handle);
    count = *actual_size < capacity ? *actual_size : capacity;
    good = ti_Read(data, 1, count, handle) == count;
    if (!ti_Close(handle)) good = false;
    return good ? 1 : -1;
}

static bool write_slot(void *ctx, unsigned slot, const uint8_t *data, size_t size) {
    uint8_t handle;
    bool good;
    (void)ctx;
    handle = ti_Open(names[slot], "w");
    if (!handle) return false;
    good = ti_Write(data, 1, size, handle) == size;
    if (!ti_Close(handle)) good = false;
    return good;
}

static bool archive_slot(void *ctx, unsigned slot) {
    uint8_t handle;
    bool good;
    (void)ctx;
    handle = ti_Open(names[slot], "r");
    if (!handle) return false;
    good = ti_IsArchived(handle) || ti_SetArchiveStatus(true, handle) != 0;
    if (!ti_IsArchived(handle)) good = false;
    if (!ti_Close(handle)) good = false;
    return good;
}

static JournalIO io = {NULL, read_slot, write_slot, archive_slot, NULL};

void storage_ce_init(void (*before_gc)(void), void (*after_gc)(void)) {
    ti_SetGCBehavior(before_gc, after_gc);
}

void storage_ce_set_validator(bool (*validate)(const uint8_t *, size_t, void *)) {
    io.validate = validate;
}

JournalResult storage_ce_load(uint8_t *payload, size_t size, uint32_t *generation) {
    return journal_load(&io, payload, size, generation);
}

JournalResult storage_ce_save(const uint8_t *payload, size_t size, uint32_t *generation) {
    return journal_commit(&io, payload, size, generation);
}
