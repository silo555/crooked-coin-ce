# Build and development

## Native calculator build

Use the official [CEdev v15.0 release](https://github.com/CE-Programming/toolchain/releases/tag/v15.0). Reuse an installed v15 toolchain. Both the checkout and toolchain paths must have no spaces.

On Linux/macOS, add `CEdev/bin` to PATH and set CEDEV if your environment needs it:

```sh
export CEDEV=/path/to/CEdev
export PATH="$CEDEV/bin:$PATH"
make
python3 tools/verify_8xp.py bin/CROOKED.8xp --icon-asm obj/icon.s
```

On Windows PowerShell:

```powershell
./tools/build.ps1 -Cedev C:/CEdev
```

Or launch the existing `cedev.bat`, enter the project folder, and run `make`. Build output is `bin/CROOKED.8xp`; `obj/icon.s` contains the converted shell icon. The validator checks the TI83F header, type, variable name, archive flag, lengths, checksum, every embedded icon palette index, and the description.

`make clean` removes generated build output. Original assets are in `assets/`; do not edit generated `obj/` files. `python3 tools/make_icon.py --check` verifies the original icon's reproducibility. Omitting `--check` regenerates both PNG files using only Python's standard library.

## Host tests

Python 3.10+ and a C99 host compiler are sufficient. No commercial game, Steam SDK, Unity installation, ROM, Python imaging packages, or network access is needed.

```sh
CC=clang python3 tools/run_tests.py --sanitize
python3 tools/verify_8xp.py --self-test
```

On Windows with an existing Zig compiler:

```powershell
python tools/run_tests.py --cc C:/tools/zig/zig.exe
```

The test runner explicitly enables assertions even with optimizing compilers. ASan/UBSan is exercised in Linux CI; a Windows compiler may not supply ASan. See [TESTING.md](TESTING.md) for exactly which checks were run.

## Interactive preview and screenshots

```sh
python3 preview/run.py --cc clang
python3 preview/run.py --cc clang --screenshots
```

The first command serves a local preview at `http://127.0.0.1:8766` (use `--port` to change it). The second renders ten documented setup fixtures into `docs/screenshots/` and exits. The host bridge is outside `src/` and is not part of the calculator program. Fixtures are for visual review; they do not prove progression or emulator execution. Preview progress lasts only for the current server session, and its save acknowledgments are simulated. Real persistence is exercised separately by the journal/integration tests and by the unverified native fileioc backend.

## Layout

| Path | Purpose |
| --- | --- |
| `src/game.*` | Economy, sampling/landing phases, achievements, history, explicit save codec |
| `src/ui.*` | Actual screens, state transitions, controls and confirmations |
| `src/render.*` | Original pixel font/art, clipped primitives, CE/host drawing |
| `src/journal.*` | Portable two-slot journal, CRC, generation and collision rules |
| `src/storage_ce.*` | Native AppVar reads/writes/archive and GC callbacks |
| `src/main.c` | CE keyboard, RTC/tick timing, palette and persistence integration |
| `preview/` | Same portable C code in a desktop shared library and local web UI |
| `tests/fixtures/` | Numerical observations and source digests, no extracted source/assets |

The [prepared CI workflow](build-workflow.yml.example) pins the official v15.0 archive by SHA-256. It is currently inactive because the GitHub sign-in lacks `workflow` permission. After authorization, move it to `.github/workflows/ci.yml`. Changes to the toolchain pin should be deliberate and tested. Do not add commercial binaries, decompiled code, personal saves, credentials, or ROMs to the repository.
