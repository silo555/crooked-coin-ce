# Testing and remaining validation

Version 0.1.0 is a preview. Native compilation and transfer validation are separate from calculator execution.

## Completed

- Owned build 25535451 audited through managed metadata, private decompilation, scene data/event wiring and actual Unity math-method execution.54 payout factors observed; personal save bytes unchanged. Full PC gameplay/cinematics were not run.
- Windows CEdev v15 native build of `CROOKED.8xp` succeeded. Transfer header/type/name/archive flag/length/checksum and all 256 shell-icon indices plus description verified.
- Portable game tests cover all 22 purchases/caps,270 payout cases, exact float32 chance values at9 levels, `<` boundaries, zero/one/invalid rolls, streak reset, midair purchases, final-trigger sequencing, all ending thresholds, all 14 badges, separate profiles, history, codec validation and saturation.
- Portable UI tests exercise actual control transitions, timing, shopping during a flip, reset confirmation/cancel, load recovery, save failure/retry, loaded result display, all screens and ending animation frames.
- Journal tests cover the CRC known vector,11,520 single-bit mutations,2,160 truncated/interrupted records, semantic rejection, generation wraparound, failed reads/writes/readback/archive, retry protection, and foreign-name preservation.
- Integration tests combine actual UI/engine state, serialization and journal recovery, including interrupted achievement saves. See CI output for their case count.
- The actual C renderer generated ten reviewed host screenshots, clearly identified as setup fixtures. A browser play check completed the handbook, started a flip, and opened the shop during flight.
- The prepared CI example contains host ASan/UBSan and the pinned official Linux CEdev build. It is inactive pending GitHub `workflow` authorization; no GitHub CI run has occurred. Do not infer CI success from this workflow file.

Local Windows tests passed with the already-installed Zig 0.15.2 C compiler, assertions explicitly enabled, both normally and with UndefinedBehaviorSanitizer. The prepared Linux CI also requests AddressSanitizer, which has not yet run. The native compiler issues upstream linker notices concerning GNU-stack metadata; these are not calculator execution failures.

## Not yet verified

- Physical TI-84 Plus CE execution, transfer, keypad handling, frame rate and available RAM.
- CEmu execution: no suitable legally available ROM was found in the checked calculator-project/download/backup locations. The existing CEdev installation includes an autotester, which is insufficient without a ROM.
- Native fileioc error paths, flash archive wear/GC behavior, and interrupted archive operations on the TI OS.
- Full PC differential gameplay/cinematic traces, the alternate April executable, original audiovisual presentation and original developer commentary. These limits are reflected in [PARITY.md](PARITY.md).

## Hardware acceptance procedure

Use your own calculator/ROM and make an external backup of both `CCSAVE` AppVars first.

1. Transfer the native program and required libraries; launch and complete the handbook.
2. Check each mapped key, readability, animation and reduced-motion timing.
3. Earn a head; buy a one-cent upgrade. Leave/reopen the game and verify bank, level and badge.
4. Try Y= and a speed/value/combo purchase during flight; verify the current flip uses the newly purchased value/speed.
5. Switch profiles, quit/reopen, and confirm their separate progression.
6. Test no-space/save errors on disposable test data; ensure retry never reports unverified data as saved.
7. In CEmu using disposable save states, interrupt writes at controlled points and test fallback. Do not deliberately power-cycle a personal calculator during flash writes as a substitute for controlled fault injection.
8. Exercise the five endings using an instrumented **test build** and confirm device rendering, reset and local badge persistence. The distributed native build has no rigging/ending-test key.

Record model, OS, launcher/library versions, free RAM, test results and whether testing was on hardware or CEmu before removing the preview label.
