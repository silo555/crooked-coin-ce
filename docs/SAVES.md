# Saves and recovery

Two AppVars, `CCSAVE0` and `CCSAVE1`, hold alternating records. They contain both profiles, achievements, counters, settings, recent messages and a persistent RNG state. Save data is owned by this game only when its magic/version/length identify a compatible record. Unknown data under either reserved name blocks writing.

## Record

| Field | Encoding |
| --- | --- |
| Magic |8 bytes, `CCCEJNL1` |
| Schema |u16 little endian, version 1 |
| Payload length |u16 little endian,640 |
| Generation |u32 little endian, wraps using serial-number comparison |
| CRC32 |u32 little endian, IEEE CRC over header bytes0–15 and payload |
| Game payload |640 bytes, explicit little-endian fields and zero reserved bytes |

The game codec starts with `CCSTATE1`, serializes fixed-width integers explicitly, and validates levels, settings, history types, limits, and other invariants before accepting a record. No compiler struct padding or24-bit native `int` is written. The current record is 660 bytes. Both AppVars together store 1,320 record bytes, plus OS variable overhead.

## Commit protocol

1. Read both records fresh, verify header/CRC and semantic payload, and choose the newest valid generation.
2. Reject foreign-name conflicts. Before reusing the older slot, make sure the selected current copy is archived and verified. This also protects a retry after an earlier archive failure.
3. Write the new generation into the alternate slot; the previous valid copy remains intact.
4. Read back and compare the written bytes.
5. Archive the new record, then read back and compare again. Success is reported only after all checks pass.

Completed flips automatically commit. Shop purchases and settings changes commit as well, but a purchase during animation waits for landing. This preserves the PC's sampled-heads/live-payout behavior without serializing an unfinished flip. Native archive/garbage-collection behavior still needs physical or emulator validation; callbacks restore the OS display before GC and rebuild the game palette afterward.

## Failure behavior

- One valid copy and one bad copy: load the valid copy and show a recovery notice. A later save repairs an owned damaged sibling.
- No valid copy: retain the AppVars and offer retry/safe quit. A confirmed fresh start is available only for owned damaged data.
- Foreign data or a future schema under a reserved name: preserve it and block writing. Back up and investigate those AppVars with TI Connect CE; do not blindly delete them.
- Write/readback/archive failure: keep the live game in memory, show retry, and require confirmation to quit without saving.

A checksum cannot protect against loss of both AppVars, a whole-calculator reset, hardware failure, or simultaneous damage to both copies. Keep external backups when progress matters. The portable fault-injection tests establish the journal's behavior under its I/O contract; they cannot prove TI OS behavior during actual power loss.
