#!/usr/bin/env python3
from pathlib import Path
import struct
import zlib
import sys

def write_png(path, w, h, pixel_fn):
    raw = bytearray()

    for y in range(h):
        raw.append(0)
        for x in range(w):
            r, g, b = pixel_fn(x, y)
            raw.extend([r, g, b])

    def chunk(kind, data):
        return (
            struct.pack(">I", len(data)) +
            kind +
            data +
            struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
        )

    png = bytearray()
    png.extend(b"\x89PNG\r\n\x1a\n")
    png.extend(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
    png.extend(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
    png.extend(chunk(b"IEND", b""))

    Path(path).write_bytes(png)

def solid(rgb):
    return lambda x, y: rgb

def box(x, y):
    if 10 <= x <= 40 and 10 <= y <= 40:
        return (0, 0, 0)
    return (255, 255, 255)

def circle(x, y):
    dx = x - 32
    dy = y - 32
    if dx * dx + dy * dy <= 22 * 22:
        return (0, 0, 0)
    return (255, 255, 255)

patterns = {
    "red": solid((255, 0, 0)),
    "blue": solid((0, 0, 255)),
    "black": solid((0, 0, 0)),
    "white": solid((255, 255, 255)),
    "box": box,
    "circle": circle,
}

if len(sys.argv) != 3:
    print("usage: make_png.py <pattern> <output.png>", file=sys.stderr)
    sys.exit(2)

pattern = sys.argv[1]
out = sys.argv[2]

if pattern not in patterns:
    print(f"unknown pattern: {pattern}", file=sys.stderr)
    sys.exit(2)

write_png(out, 64, 64, patterns[pattern])
