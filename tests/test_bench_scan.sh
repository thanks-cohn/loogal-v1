#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

rm -rf /tmp/loogal-bench-scan-test
mkdir -p /tmp/loogal-bench-scan-test/a
mkdir -p /tmp/loogal-bench-scan-test/b

python tests/lib/make_png.py red /tmp/loogal-bench-scan-test/a/red.png
python tests/lib/make_png.py blue /tmp/loogal-bench-scan-test/b/blue.png
printf 'not an image\n' > /tmp/loogal-bench-scan-test/readme.txt

rm -f data/loogal.bin \
      data/records.jsonl \
      data/identities.jsonl \
      data/locations.jsonl \
      data/events.jsonl \
      data/logs/loogal.jsonl

./loogal bench-scan /tmp/loogal-bench-scan-test > /tmp/loogal-bench-scan-output.txt

grep -q "LOOGAL BENCH SCAN" /tmp/loogal-bench-scan-output.txt
grep -q "privacy:       no paths, no hashes, no records written" /tmp/loogal-bench-scan-output.txt
grep -q "files_seen:" /tmp/loogal-bench-scan-output.txt
grep -q "supported_images:" /tmp/loogal-bench-scan-output.txt
grep -q "throughput:" /tmp/loogal-bench-scan-output.txt
grep -q "ns/file:" /tmp/loogal-bench-scan-output.txt

test ! -f data/records.jsonl
test ! -f data/identities.jsonl
test ! -f data/locations.jsonl
test ! -f data/loogal.bin

echo "BENCH SCAN TEST OK"
cat /tmp/loogal-bench-scan-output.txt
