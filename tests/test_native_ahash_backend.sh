#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

rm -rf /tmp/loogal-native-ahash-test
mkdir -p /tmp/loogal-native-ahash-test

python - <<'PY'
from pathlib import Path
import struct
import zlib

root = Path("/tmp/loogal-native-ahash-test")

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
    path.write_bytes(png)

def left_dark(x, y):
    if x < 32:
        return (0, 0, 0)
    return (255, 255, 255)

write_png(root / "left_dark.png", 64, 64, left_dark)
PY

rm -f data/loogal.bin \
      data/records.jsonl \
      data/identities.jsonl \
      data/locations.jsonl \
      data/events.jsonl \
      data/logs/loogal.jsonl

mkdir -p data/logs

./loogal index --fresh /tmp/loogal-native-ahash-test > /tmp/loogal-native-ahash-index.txt

test -f data/identities.jsonl
test -f data/records.jsonl

grep -q '"ahash":' data/identities.jsonl
grep -q '"ahash":"' data/records.jsonl

echo "NATIVE AHASH BACKEND TEST OK"
cat data/identities.jsonl
