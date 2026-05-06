#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

rm -rf /tmp/loogal-shadow-search-test
mkdir -p /tmp/loogal-shadow-search-test/index
mkdir -p data/logs

python - <<'PY'
from pathlib import Path
import struct
import zlib
import shutil

root = Path("/tmp/loogal-shadow-search-test")
idx = root / "index"

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

def original(x, y):
    if 10 <= x <= 42 and 10 <= y <= 42:
        return (0, 0, 0)
    return (255, 255, 255)

def minor_variant(x, y):
    if 11 <= x <= 44 and 10 <= y <= 43:
        return (0, 0, 0)
    return (255, 255, 255)

def different(x, y):
    if abs(x - y) <= 3:
        return (0, 0, 0)
    return (255, 255, 255)

write_png(idx / "original.png", 64, 64, original)
write_png(idx / "minor_variant.png", 64, 64, minor_variant)
write_png(idx / "different.png", 64, 64, different)

shutil.copyfile(idx / "original.png", root / "query.png")
PY

rm -f data/loogal.bin \
      data/records.jsonl \
      data/identities.jsonl \
      data/locations.jsonl \
      data/events.jsonl \
      data/logs/loogal.jsonl

./loogal index --fresh /tmp/loogal-shadow-search-test/index > /tmp/loogal-shadow-search-index.txt
./loogal search /tmp/loogal-shadow-search-test/query.png --engine shadow 70 > /tmp/loogal-shadow-search.txt
./loogal search /tmp/loogal-shadow-search-test/query.png --engine shadow 70 --json > /tmp/loogal-shadow-search.json

grep -q "engine:     binary_index:shadow:v1" /tmp/loogal-shadow-search.txt
grep -q "shadow:" /tmp/loogal-shadow-search.txt
grep -q "dhash:" /tmp/loogal-shadow-search.txt
grep -q "ahash:" /tmp/loogal-shadow-search.txt
grep -q "original.png" /tmp/loogal-shadow-search.txt
grep -q "minor_variant.png" /tmp/loogal-shadow-search.txt

grep -q '"engine": "binary_index:shadow:v1"' /tmp/loogal-shadow-search.json
grep -q '"ahash_distance":' /tmp/loogal-shadow-search.json
grep -q '"aspect_similarity":' /tmp/loogal-shadow-search.json

echo "SHADOW SEARCH ENGINE TEST OK"
cat /tmp/loogal-shadow-search.txt
