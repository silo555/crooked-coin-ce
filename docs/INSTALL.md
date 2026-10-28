# Installation

## Requirements

- TI-84 Plus CE or TI-84 Plus CE Python, 320×240 display.
- Enough RAM to run an approximately 47 KB native program and its small working state; enough Archive for the program, libraries, and two save AppVars. Free RAM and real device behavior still require hardware validation.
- [CE libraries v15.0](https://github.com/CE-Programming/libraries/releases/tag/v15.0). The program imports GRAPHX, KEYPADC, FILEIOC and the normal CE library loader. Transfer the official `clibs.8xg` if these libraries are missing or outdated.
- A working native C/assembly launch setup. [Cesium's upstream instructions](https://github.com/mateoconlechuga/cesium) cover its installation. CEdev's [getting-started guide](https://ce-programming.github.io/toolchain/static/getting-started.html) explains the OS 5.5+ native-launch restriction and links to arTIfiCE.

Use an existing working calculator setup when possible. Do not update the calculator OS just to install this game. This repository contains no OS image or ROM.

## Transfer

1. Download `CROOKED.8xp` from the [preview release](https://github.com/silo555/crooked-coin-ce/releases/tag/v0.1.0). If using the ZIP, extract it first.
2. Connect the calculator by USB and open the official [TI Connect CE](https://education.ti.com/en/products/computer-software/ti-connect-ce-sw).
3. Transfer required libraries and `CROOKED.8xp` to Archive.
4. In Cesium or your compatible native launcher, select `CROOKED` and run it. The selected description is **Crooked Coin CE**.
5. Read the short handbook, then press ENTER to flip. Y= opens the shop. GRAPH → SAVE AND QUIT exits safely.

An `Invalid` error on a newer OS can indicate that the native launch setup is missing. A missing-library error requires the official CE library package. This game is not a TI-BASIC program and does not run on the monochrome TI-84 Plus or the TI-84 Plus C Silver Edition.

## Saves and upgrades

The program creates `CCSAVE0` and `CCSAVE1`. Both profiles, local achievements, settings, history, and statistics are stored together. Keep both AppVars when replacing the program with a new version. Back up both using TI Connect CE before removing or changing them. No PC save import is included.

Completed flips and purchases are automatically committed; purchases during a flip are committed when that flip lands. An interrupted write leaves the previous valid copy available. If both copies are unusable, the game offers retry, safe quit, or an explicitly confirmed fresh start. Unknown files with those names are preserved and block writing.

## Validation status

The release is a **preview** because it has not run on a physical calculator or in CEmu. The host preview uses the actual C engine and renderer but cannot establish native keypad behavior, speed, OS archive/garbage-collection behavior, or battery-loss recovery. See [TESTING.md](TESTING.md).
