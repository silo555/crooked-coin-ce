"""Reproduce the original 16x16 calculator icon using only Python's stdlib."""
import argparse
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
COLORS = {
    ".": (9, 34, 40),
    "t": (15, 57, 63),
    "s": (4, 22, 28),
    "b": (117, 66, 26),
    "o": (186, 113, 30),
    "g": (240, 177, 49),
    "l": (255, 219, 99),
    "w": (255, 244, 185),
}
# Individually authored pixels: beveled coin, engraved crooked C, offset shadow.
PIXELS = (
    "................",
    "......tttt......",
    "....blllllb.....",
    "...blwwllllb....",
    "..blwlgggglob...",
    "..blglgggggob...",
    ".blgggbbbgggob..",
    ".blggblwggggob..",
    ".blgblwgggggob..",
    ".bogblwgggggob..",
    "..bogbwllggobss.",
    "..boggbbbgoobss.",
    "...boggggoobss..",
    "....bbooobsss...",
    "......ssss......",
    "................",
)


def png_bytes(scale=1):
    assert len(PIXELS) == 16 and all(len(row) == 16 for row in PIXELS)
    width = height = 16 * scale
    rows = []
    for row in PIXELS:
        scanline = b"\0" + b"".join(bytes(COLORS[p]) * scale for p in row)
        rows.extend([scanline] * scale)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(b"".join(rows), 9))
        + chunk(b"IEND", b"")
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail if committed PNGs differ")
    args = parser.parse_args()
    for name, scale in (("icon.png", 1), ("icon-preview.png", 16)):
        path = ROOT / "assets" / name
        data = png_bytes(scale)
        if args.check:
            if not path.exists() or path.read_bytes() != data:
                raise SystemExit(f"Outdated generated asset: {path.relative_to(ROOT)}")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        print(f"{'Verified' if args.check else 'Wrote'} {path.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
