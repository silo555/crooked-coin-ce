"""Validate the TI transfer envelope, native program and Cesium icon metadata."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def require(condition, message):
    if not condition:
        raise ValueError(message)


def read_icon_asm(path):
    """Read only constant byte declarations emitted by convimg for an icon."""
    values = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        match = re.match(r"\s*\.db\s+(.*)", line)
        if not match:
            continue
        for token in re.findall(r'"[^"\\]*"|0x[0-9a-fA-F]+|\d+', match.group(1)):
            if token.startswith('"'):
                values.extend(token[1:-1].encode("ascii"))
            else:
                value = int(token, 0)
                require(0 <= value < 256, "Invalid byte in icon assembly")
                values.append(value)
    result = bytes(values)
    require(result[:3] == b"\x01\x10\x10", "Icon assembly is not a 16x16 color icon")
    require(len(result) == 3 + 256 + len(b"Crooked Coin CE\0"), "Unexpected icon metadata length")
    return result


def verify(data, expected_name="CROOKED", icon=None):
    require(len(data) >= 82, "TI transfer file is too short")
    require(data[:11] == b"**TI83F*\x1a\x0a\x00", "Invalid TI-83/84 transfer signature")
    u16 = lambda offset: struct.unpack_from("<H", data, offset)[0]
    section_size = u16(53)
    require(len(data) == section_size + 57, "Transfer section length mismatch or trailing data")
    require(sum(data[55:-2]) & 0xFFFF == u16(len(data) - 2), "TI transfer checksum mismatch")
    require(u16(55) == 13, "Unexpected variable-entry header size")
    var_size = u16(57)
    require(section_size == var_size + 17, "Expected exactly one variable entry")
    require(data[59] == 0x06, "Variable must be a protected program (type 0x06)")
    name = data[60:68]
    require(name == expected_name.encode("ascii").ljust(8, b"\0"), "Wrong calculator variable name")
    require(data[68] == 0, "Unexpected variable version")
    require(data[69] in (0, 0x80), "Unexpected archive flag")
    require(u16(70) == var_size, "Repeated variable length disagrees")
    require(u16(72) == var_size - 2, "Internal program length disagrees")
    program = data[74:-2]
    require(program[:3] == b"\xef\x7b\x00", "Missing native eZ80 program marker")
    require(program[3] == 0xC3, "Missing icon jump instruction")
    # The eZ80 ADL JP instruction is one opcode followed by a three-byte address.
    metadata = program[7:]
    require(metadata[:3] == b"\x01\x10\x10", "Missing 16x16 Cesium color icon")
    description = b"Crooked Coin CE\0"
    require(metadata[259:259 + len(description)] == description, "Wrong spaced program description")
    if icon is not None:
        require(metadata[:len(icon)] == icon, "Embedded icon differs from convimg output")
    return {
        "variable": expected_name,
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "transfer": "valid",
        "protected_program": True,
        "archived": data[69] == 0x80,
        "icon": "16x16 color, verified" if icon else "16x16 color",
        "description": "Crooked Coin CE",
    }


def fixture():
    metadata = b"\x01\x10\x10" + bytes(range(256)) + b"Crooked Coin CE\0"
    program = b"\xef\x7b\x00\xc3\x00\xa8\xd1" + metadata + b"\xc9"
    payload = struct.pack("<H", len(program)) + program
    entry = struct.pack("<HHB8sBBH", 13, len(payload), 6, b"CROOKED\0", 0, 0x80, len(payload)) + payload
    header = b"**TI83F*\x1a\x0a\x00" + b"\0" * 42 + struct.pack("<H", len(entry))
    return header + entry + struct.pack("<H", sum(entry) & 0xFFFF)


def self_test():
    good = fixture()
    verify(good)
    # Include mutations with a corrected transfer checksum to exercise inner checks.
    for offset in (0, 53, 55, 57, 59, 60, 68, 69, 70, 72, 74, 77, 81, 82, 83, 340):
        bad = bytearray(good)
        bad[offset] ^= 1
        bad[-2:] = struct.pack("<H", sum(bad[55:-2]) & 0xFFFF)
        try:
            verify(bytes(bad))
        except ValueError:
            pass
        else:
            raise AssertionError(f"Accepted mutation at offset {offset}")
    for bad in (good[:-1], good + b"\0", bytes(80)):
        try:
            verify(bad)
        except ValueError:
            pass
        else:
            raise AssertionError("Accepted malformed transfer envelope")
    bad = bytearray(good)
    bad[-1] ^= 1
    try:
        verify(bytes(bad))
    except ValueError:
        pass
    else:
        raise AssertionError("Accepted invalid checksum")
    print("TI verifier self-tests passed (20 rejection cases).")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path, nargs="?")
    parser.add_argument("--icon-asm", type=Path, help="compare every embedded palette byte to convimg output")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    if args.path:
        icon = read_icon_asm(args.icon_asm) if args.icon_asm else None
        result = verify(args.path.read_bytes(), icon=icon)
        result["file"] = args.path.name
        print(json.dumps(result, indent=2))
    elif not args.self_test:
        parser.error("supply an .8xp path or --self-test")


if __name__ == "__main__":
    main()
