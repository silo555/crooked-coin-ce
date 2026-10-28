#include "game.h"
#include "journal.h"
#include "ui.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { RECORD_SIZE = JOURNAL_HEADER_SIZE + CC_SAVE_SIZE };
typedef struct {
    uint8_t data[RECORD_SIZE];
    size_t size;
    bool exists, archived;
} MemorySlot;
typedef struct {
    MemorySlot slots[2];
    size_t write_limit;
    unsigned writes;
} Memory;

static int read_slot(void *ctx, unsigned slot, uint8_t *out, size_t capacity,
                     size_t *actual_size) {
    const MemorySlot *s = &((Memory *)ctx)->slots[slot];
    assert(slot < 2);
    if (!s->exists) return 0;
    *actual_size = s->size;
    memcpy(out, s->data, capacity < s->size ? capacity : s->size);
    return 1;
}

static bool write_slot(void *ctx, unsigned slot, const uint8_t *data, size_t size) {
    Memory *m = ctx;
    MemorySlot *s = &m->slots[slot];
    size_t count = size < m->write_limit ? size : m->write_limit;
    assert(slot < 2 && size == RECORD_SIZE);
    ++m->writes;
    s->exists = true;
    s->archived = false;
    s->size = count;
    memcpy(s->data, data, count);
    return count == size;
}

static bool archive_slot(void *ctx, unsigned slot) {
    MemorySlot *s = &((Memory *)ctx)->slots[slot];
    assert(slot < 2 && s->exists && s->size == RECORD_SIZE);
    s->archived = true;
    return true;
}

static JournalIO backend(Memory *m) {
    JournalIO io = {m, read_slot, write_slot, archive_slot, cc_validate_save};
    return io;
}

static void memory_init(Memory *m) {
    memset(m, 0, sizeof *m);
    m->write_limit = RECORD_SIZE;
}

static void power_loss(Memory *m) {
    unsigned i;
    for (i = 0; i < 2; ++i)
        if (!m->slots[i].archived) m->slots[i].exists = false;
}

static void same_game(const CCGame *a, const CCGame *b) {
    uint8_t left[CC_SAVE_SIZE], right[CC_SAVE_SIZE];
    cc_encode(a, left);
    cc_encode(b, right);
    assert(!memcmp(left, right, sizeof left));
    assert(!a->in_flight && !b->in_flight);
}

static JournalResult load_game(const JournalIO *io, CCGame *g, uint32_t *gen) {
    uint8_t bytes[CC_SAVE_SIZE];
    JournalResult result = journal_load(io, bytes, sizeof bytes, gen);
    if (result == JOURNAL_OK || result == JOURNAL_RECOVERED)
        assert(cc_decode(g, bytes, sizeof bytes));
    return result;
}

static void save_game(const JournalIO *io, const CCGame *g, uint32_t *gen) {
    uint8_t bytes[CC_SAVE_SIZE];
    assert(!g->in_flight);
    cc_encode(g, bytes);
    assert(cc_validate_save(bytes, sizeof bytes, NULL));
    assert(journal_commit(io, bytes, sizeof bytes, gen) == JOURNAL_OK);
}

/* This is the application's save scheduling boundary: changes made in the
 * shop during a toss remain pending until the sampled result has landed. */
static bool service_ui_save(CCUI *u, const JournalIO *io, uint32_t *gen) {
    uint8_t bytes[CC_SAVE_SIZE];
    JournalResult result;
    if (!u->save_requested || u->flipping) return false;
    assert(!u->game.in_flight);
    cc_encode(&u->game, bytes);
    result = journal_commit(io, bytes, sizeof bytes, gen);
    cc_ui_saved(u, result == JOURNAL_OK);
    return true;
}

static void prepare_profiles(CCGame *g) {
    unsigned i;
    cc_new(g, UINT32_C(0x12345));
    g->tutorial_seen = 1;
    for (i = 0; i < 7; ++i) assert(cc_flip(g, 0.f, 0.f).heads);
    assert(!cc_flip(g, 1.f, 0.f).heads);
    assert(cc_buy(g, CC_CHANCE));
    assert(cc_buy(g, CC_SPEED));
    assert(cc_buy(g, CC_COMBO));
    for (i = 0; i < 16; ++i) cc_flip(g, 1.f, 0.f);
    cc_advance(g, 23456);
    g->active = 1;
    g->commentary = 1;
    for (i = 0; i < 4; ++i) assert(cc_flip(g, 0.f, 0.f).heads);
    assert(cc_buy(g, CC_CHANCE));
    cc_advance(g, 34567);
    g->fish = g->reduced_motion = 1;
    assert(g->achievements == 0x7f);
    assert(g->run[0].history_count == CC_HISTORY && g->run[1].history_count);
    assert(g->run[0].level[CC_SPEED] && !g->run[1].level[CC_SPEED]);
}

static void earn_all_endings(CCGame *g) {
    static const float rolls[5] = {.1f, .3f, .5f, .7f, .95f};
    unsigned ending, i;
    g->active = 0;
    for (ending = 0; ending < 5; ++ending) {
        cc_restart(g);
        for (i = 0; i < 9; ++i) assert(cc_flip(g, 0.f, 0.f).heads);
        assert(cc_flip(g, 0.f, rolls[ending]).ending == ending + 1);
        assert(g->ending_pending && g->ending_count[ending] == 1);
    }
    assert(g->achievements == 0x3fff && g->best_streak == 10);
    assert(g->run[1].history_count && g->run[1].bank == 3);
}

static void persistence_and_recovery(void) {
    Memory memory, corrupted;
    JournalIO io, damaged_io;
    CCGame old, all_badges, loaded;
    CCUI ui;
    uint32_t gen = 0;
    prepare_profiles(&old);
    all_badges = old;
    earn_all_endings(&all_badges);
    memory_init(&memory); io = backend(&memory);
    save_game(&io, &old, &gen); assert(gen == 1);
    save_game(&io, &all_badges, &gen); assert(gen == 2);
    assert(load_game(&io, &loaded, &gen) == JOURNAL_OK);
    same_game(&loaded, &all_badges);
    assert(loaded.achievements == 0x3fff && loaded.ending_pending);

    /* A later transaction is lost, while the previously archived badges,
     * ending presentation, separate profile and ledger remain recoverable. */
    loaded.fish ^= 1;
    save_game(&io, &loaded, &gen); assert(gen == 3);
    corrupted = memory;
    corrupted.slots[0].data[RECORD_SIZE - 1] ^= 0x80;
    damaged_io = backend(&corrupted);
    assert(load_game(&damaged_io, &loaded, &gen) == JOURNAL_RECOVERED && gen == 2);
    same_game(&loaded, &all_badges);

    memory.slots[0].size = RECORD_SIZE / 2;
    assert(load_game(&io, &loaded, &gen) == JOURNAL_RECOVERED && gen == 2);
    same_game(&loaded, &all_badges);
    cc_ui_init(&ui, &loaded);
    assert(ui.screen == VIEW_END);
    cc_ui_tick(&ui, 1000);
    cc_ui_key(&ui, KEY_ENTER);
    assert(ui.screen == VIEW_PLAY && !ui.game.ending_pending && ui.save_requested);
    assert(service_ui_save(&ui, &io, &gen) && ui.saved && gen == 3);
    assert(load_game(&io, &loaded, &gen) == JOURNAL_OK);
    same_game(&loaded, &ui.game);
    assert(loaded.achievements == 0x3fff);
    ui.game.rng = 1; /* Deterministic low first roll, then actual UI toss. */
    cc_ui_key(&ui, KEY_ENTER);
    assert(ui.flipping && ui.pending_head);
    cc_ui_tick(&ui, 1000); cc_ui_tick(&ui, 1000);
    assert(!ui.flipping && ui.result.heads);
    assert(service_ui_save(&ui, &io, &gen));
    assert(load_game(&io, &loaded, &gen) == JOURNAL_OK);
    assert(loaded.achievements == 0x3fff && loaded.run[0].streak == 1);
    assert(loaded.run[1].level[CC_CHANCE] == 1 && loaded.run[1].bank == 3);
    puts("integration: both profiles, settings, ledger, all14 badges and ending survive save/recovery/UI resume");
}

static void transaction_cuts(void) {
    CCGame old, next, loaded;
    uint8_t old_bytes[CC_SAVE_SIZE], next_bytes[CC_SAVE_SIZE], got[CC_SAVE_SIZE];
    size_t cut;
    prepare_profiles(&old); next = old; earn_all_endings(&next);
    cc_encode(&old, old_bytes); cc_encode(&next, next_bytes);
    assert(memcmp(old_bytes, next_bytes, sizeof old_bytes));
    assert(RECORD_SIZE == 660);
    for (cut = 0; cut <= RECORD_SIZE; ++cut) {
        Memory m;
        JournalIO io;
        uint32_t gen = 0;
        JournalResult result;
        memory_init(&m); io = backend(&m);
        save_game(&io, &old, &gen); save_game(&io, &old, &gen);
        assert(gen == 2);
        m.write_limit = cut;
        result = journal_commit(&io, next_bytes, sizeof next_bytes, &gen);
        assert(result == (cut == RECORD_SIZE ? JOURNAL_OK : JOURNAL_IO_ERROR));
        assert(gen == (cut == RECORD_SIZE ? 3u : 2u));
        result = journal_load(&io, got, sizeof got, &gen);
        assert(result == JOURNAL_OK || result == JOURNAL_RECOVERED);
        assert(!memcmp(got, cut == RECORD_SIZE ? next_bytes : old_bytes, sizeof got));
        assert(cc_decode(&loaded, got, sizeof got));
        assert(loaded.achievements == (cut == RECORD_SIZE ? 0x3fff : 0x7f));
        power_loss(&m);
        assert(load_game(&io, &loaded, &gen) == JOURNAL_OK);
        same_game(&loaded, cut == RECORD_SIZE ? &next : &old);
    }
    puts("integration: all661 byte-write cut positions return the complete old/new game, never blended progress");
}

static void semantic_recovery(void) {
    Memory m;
    JournalIO io;
    CCGame old, bad, loaded;
    uint8_t bytes[CC_SAVE_SIZE];
    uint32_t gen = 0;
    prepare_profiles(&old); bad = old; bad.active = 2;
    memory_init(&m); io = backend(&m);
    save_game(&io, &old, &gen);
    cc_encode(&bad, bytes);
    assert(!cc_validate_save(bytes, sizeof bytes, NULL));
    m.slots[1].size = journal_encode(m.slots[1].data,
        sizeof m.slots[1].data, bytes, sizeof bytes, 2);
    m.slots[1].exists = m.slots[1].archived = true;
    assert(m.slots[1].size == RECORD_SIZE);
    /* The envelope checksum is valid: only the actual game validator catches
     * this unsafe profile index and recovers the older valid game. */
    assert(journal_decode(m.slots[1].data, RECORD_SIZE, bytes, sizeof bytes, &gen));
    assert(gen == 2);
    assert(load_game(&io, &loaded, &gen) == JOURNAL_RECOVERED && gen == 1);
    same_game(&loaded, &old);
    puts("integration: valid-CRC/invalid-game newest record falls back through real semantic validator");
}

static void pending_purchase(void) {
    Memory m;
    JournalIO io;
    CCGame game, loaded;
    CCUI ui;
    uint32_t gen = 0;
    unsigned writes;
    cc_new(&game, 1); game.tutorial_seen = 1;
    assert(cc_flip(&game, 0.f, 0.f).payout == 1);
    memory_init(&m); io = backend(&m);
    cc_ui_init(&ui, &game); ui.save_requested = 1;
    assert(service_ui_save(&ui, &io, &gen));
    writes = m.writes;
    cc_ui_key(&ui, KEY_ENTER);
    assert(ui.flipping && ui.pending_head && ui.game.in_flight);
    cc_ui_key(&ui, KEY_SHOP);
    cc_ui_key(&ui, KEY_DOWN); cc_ui_key(&ui, KEY_DOWN);
    assert(ui.selection == CC_COMBO);
    cc_ui_key(&ui, KEY_ENTER);
    assert(ui.game.run[0].level[CC_COMBO] == 1 && ui.save_requested);
    assert(!service_ui_save(&ui, &io, &gen) && m.writes == writes && gen == 1);
    assert(load_game(&io, &loaded, &gen) == JOURNAL_OK);
    assert(!loaded.run[0].level[CC_COMBO] && loaded.run[0].bank == 1);
    cc_ui_tick(&ui, 1000);
    assert(ui.flipping && !service_ui_save(&ui, &io, &gen));
    cc_ui_tick(&ui, 1000);
    assert(!ui.flipping && !ui.game.in_flight && ui.result.payout == 2);
    assert(service_ui_save(&ui, &io, &gen) && m.writes == writes + 1 && gen == 2);
    assert(load_game(&io, &loaded, &gen) == JOURNAL_OK);
    same_game(&loaded, &ui.game);
    assert(loaded.run[0].level[CC_COMBO] == 1 && loaded.run[0].bank == 2);
    assert(loaded.run[0].streak == 2 && loaded.achievements == 3);
    puts("integration: midair purchase defers persistence, landing commits upgrade+payout+badge together");
}

int main(void) {
    persistence_and_recovery();
    transaction_cuts();
    semantic_recovery();
    pending_purchase();
    puts("integration tests passed");
    return 0;
}
