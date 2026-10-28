# Feature and fidelity checklist

Checked 2026-10-27 against owned Steam build **25535451**. This release preserves the verified economy and ending state machine. It does **not** claim complete audiovisual, narrative, Steam-service, or hardware parity.

## Evidence

The audit inspected the installed managed gameplay assembly and all 107 serialized MonoBehaviour scene records, including upgrade costs and event wiring. Actual methods in the installed Unity math assembly were executed for every combo level and streak 1–9: 54 independent factors. Numerical observations and artifact digests are in [the test fixture](../tests/fixtures/owned-build-25535451.json).

| Shipped artifact | SHA-256 |
| --- | --- |
| `Assembly-CSharp.dll` | `7ca6cbee1634195c7c8397fc87b71e42dc8e32d0fd41b944178ada56eeeee39d` |
| `level0` | `7925fcab0877fac86ed477885ab4c7a4f5c3422a7178e82d5ad5c5770e91ee50` |
| `UnityEngine.CoreModule.dll` | `5afc743d350e95dc30379a6d2bafa2293b63259acf196fe3f7370f03644ef6f4` |

REA managed metadata evidence IDs: `ev_a331c24968d51643ef59888a23911d7e5a153da75990131258444a3612f6ca5a` and `ev_b63e34bc2ad10b7c2f557291fe452006e85f204b9d95c9fc82da44ebdb312ef0`. Full evidence, decompilation, scene extracts, and personal save backups remain private, outside this repository. The audit executed math methods, not the full PC gameplay/cinematic loop. Both personal `.coin` files were checked byte-for-byte against their backups afterward and were unchanged.

Public corroboration: [Steam store](https://store.steampowered.com/app/3925760/Unfair_Flips/), [14 achievements](https://steamcommunity.com/stats/3925760/achievements/), [official update history](https://steamcommunity.com/app/3925760/allnews/). The official April update describes mechanically identical rules and separate progress. That alternate executable is **not installed**, so its code, art, information display, and alternate final presentation were not locally verified.

## Exact progression

Costs below are **cents**, in purchase order. The initial bank is zero; only heads earns money.

| Upgrade | Initial → cap | Costs in cents |
| --- | --- | --- |
| Heads chance | 20% → 60%, +5 percentage points, 8 purchases | 1,10,100,1000,10000,100000,1000000,10000000 |
| Flip speed | 2.0 → 1.0 seconds, −0.2 seconds, 5 purchases | 1,10,100,1000,10000 |
| Combo | 1.0× → 3.5×, +0.5×, 5 purchases | 1,10,100,1000,10000 |
| Coin value | 1 → 5 → 10 → 25 → 100 cents, 4 purchases | 25,100,625,10000 |

The class's field defaults are overridden by scene data. Using the field initializer's five-second duration or 2× combo would be incorrect for this installed game.

For a successful head numbered `streak` in1–9:

```text
payout_cents = coin_cents * ceil(combo ^ (streak - 1))
```

The power is rounded **before** multiplying the denomination. For example, a five-cent coin at 1.5× pays 10c for the second head and 15c for the third, rather than 8c and 12c. CE uses all 54 verified factors directly to avoid differences between host and eZ80 math libraries. Maximum ordinary payout is 2,251,900c ($22,519).

Chance uses the same float32 addition sequence and strict `<` comparison. CE uses its own persisted PRNG; it does not reproduce Unity's random sequence. Flip durations use exact integer milliseconds rather than the PC's tiny float accumulation error, with display/input resolution determined by the native frame loop.

## Checklist

| Feature | Owned PC behavior / evidence | CE implementation and limits |
| --- | --- | --- |
| Four shops, prices, levels | Serialized costs and wired purchase events inspected | Implemented; all 22 purchases, exact affordability and caps tested |
| Probability | Starts `.2f`, repeated `.05f` additions; strict heads comparison | Exact chance float bits and threshold boundaries tested at all9 levels; independent CE RNG |
| Denominations and payouts | Five coin values; rounded-up power then denomination | Implemented; all270 level/streak/denomination combinations tested |
| Airborne purchases | Heads sampled at launch; current value/combo/speed read while landing | Implemented with separate launch/landing phases; no saves commit half-completed flips |
| Timing and controls | Manual click, no autoplay; live duration changes | Manual ENTER/2nd; live speed changes; original CE animation; milliseconds/frame quantization |
| Tails and streaks | Tails pays zero, resets streak, retains money/upgrades | Implemented and tested, including after nine heads |
| Final flip | Only an ordinary successful head reaching streak 10 draws an ending roll | Implemented; tenth attempt alone does not end a run; final pays zero |
| Five endings | Separate roll: `<.2` side; `<.4` eggbug; `<.6` too high; `<.9` destruction; otherwise ten heads | Same branches and 20/20/20/30/10 probabilities; original coin, visitor, sky, fragments and crown presentations |
| Ending reset | PC clears progression before cinematic; no replay of same run | CE clears active run immediately, retains pending ending for reload, then starts a new run; local lifetime totals/badges survive |
| Achievements | Nine1–9-head milestones, plus five endings; Steam owns persistence |14 local persistent badges with original names; both profiles contribute; no Steam unlock calls |
| Statistics | Run flips, current streak, bank and displayed odds; no lifetime dashboard found | Same information plus explicitly added lifetime heads/flips/time/earnings, best streak and ending counts |
| History | Last13 visible messages; tutorial/upgrade/result messages interleave; red warnings are excluded from save |13 typed messages with original prose; includes purchases and tutorial events. Insufficient-funds/cap toasts are transient |
| Standard tutorials |35 attempt-count triggers inspected | All35 trigger counts implemented, with replacement CE prose. Includes original credit purpose and probability explanations |
| Developer commentary |22 personal essays replace tutorials; changing mode resets PC run | **Adapted:** original seven-page “Mint Notes,” plus a short recurring note; confirmed run reset on mode change. Commercial essays and their paragraph feed are not copied |
| April Fools | Official statement: same mechanics, different presentation, separate save | **Partially evidenced:** separate CE run profiles and original satire labels; uses verified normal rules. Exact April presentation/code remains unverified locally |
| Saves/backups | Main and backup `.coin`, last settled state; PC purchases alone do not save | **Adapted:** two versioned/CRC AppVars with archive/readback verification. CE purchases save immediately unless airborne; pending ending and RNG also persist |
| Settings | Audio toggle, restart/quit, promo dismissal, commentary controls | Restart/quit/commentary adapted; adds reduced motion and profile switch. No audio or promotional UI; reduced motion preserves flip time |
| Hidden features | Cross-game save `weirdStage>=3` hides a spectator; no hidden economy manipulation found in inspected gameplay/scene scope | **Excluded cross-game cosmetic:** no other game's save access. An original ALPHA fish toggle is provided; it never affects odds |
| Audio/art/writing | Commercial music, sounds, art, text, guest material | Original silent pixel-art adaptation; none of those assets are redistributed |
| Steam services | Achievements, cloud/progress backup, overlay/community links | Unsupported by calculator; local badges and backups only |
| Overflow/long sessions | PC uses signed counters and64-bit money | CE safely saturates bank/earned at9 quadrillion cents and32-bit counters rather than overflowing. Timers saturate at roughly 49.7 days |
| Actual CE execution | Not established by host inspection/build | **Unverified:** no suitable ROM found, no CEmu/physical run performed |

## Standard tutorial trigger counts

```text
4,5,10,11,12,20,21,24,25,40,100,150,151,152,153,156,
500,520,521,522,523,524,526,527,1000,1020,1021,1022,
1023,1024,1025,1026,1029,5000,10000
```

The search for hidden mechanics was bounded to the inspected normal build's managed gameplay and 107 serialized scene behaviors. It does not prove the absence of hidden content in every Steam branch, future update, or external game.
