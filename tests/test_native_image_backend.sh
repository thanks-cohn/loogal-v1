#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

rm -rf /tmp/loogal-native-image-test
mkdir -p /tmp/loogal-native-image-test

python - <<'PY'
from pathlib import Path
import struct
import zlib

out = Path("/tmp/loogal-native-image-test/native.png")

w, h = 64, 64
raw = bytearray()

for y in range(h):
    raw.append(0)
    for x in range(w):
        if 10 <= x <= 40 and 10 <= y <= 40:
            raw.extend([0, 0, 0])
        else:
            raw.extend([255, 255, 255])

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

out.write_bytes(png)
PY

./loogal index --fresh /tmp/loogal-native-image-test > /tmp/loogal-native-image-index.txt
./loogal search /tmp/loogal-native-image-test/native.png 90 > /tmp/loogal-native-image-search.txt

grep -q "LOOGAL INDEX COMPLETE" /tmp/loogal-native-image-index.txt
grep -q "native.png" /tmp/loogal-native-image-search.txt

echo "NATIVE IMAGE BACKEND TEST OK"
