#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

rm -rf /tmp/loogal-immortal-a /tmp/loogal-immortal-b \
       data/loogal.bin data/records.jsonl data/identities.jsonl data/locations.jsonl data/events.jsonl data/logs/loogal.jsonl
mkdir -p /tmp/loogal-immortal-a /tmp/loogal-immortal-b data/logs

python tests/lib/make_png.py box /tmp/loogal-immortal-a/box.png
python tests/lib/make_png.py circle /tmp/loogal-immortal-b/circle.png
cp /tmp/loogal-immortal-a/box.png /tmp/loogal-immortal-b/box-copy.png

./loogal index --fresh /tmp/loogal-immortal-a > /tmp/loogal-immortal-step1.txt
ids_after_first=$(wc -l < data/identities.jsonl)
locs_after_first=$(wc -l < data/locations.jsonl)
records_after_first=$(wc -l < data/records.jsonl)

./loogal index /tmp/loogal-immortal-b > /tmp/loogal-immortal-step2.txt
ids_after_second=$(wc -l < data/identities.jsonl)
locs_after_second=$(wc -l < data/locations.jsonl)
records_after_second=$(wc -l < data/records.jsonl)

if [[ "$ids_after_first" -ne 1 ]]; then
    echo "expected 1 identity after first run, got $ids_after_first" >&2
    exit 1
fi

if [[ "$locs_after_first" -ne 1 ]]; then
    echo "expected 1 location after first run, got $locs_after_first" >&2
    exit 1
fi

if [[ "$ids_after_second" -ne 2 ]]; then
    echo "expected 2 identities after second run: original box + circle, got $ids_after_second" >&2
    exit 1
fi

if [[ "$locs_after_second" -ne 3 ]]; then
    echo "expected 3 locations after second run: box, box-copy, circle, got $locs_after_second" >&2
    exit 1
fi

if [[ "$records_after_second" -ne 3 ]]; then
    echo "expected 3 active compatibility records after second run, got $records_after_second" >&2
    exit 1
fi

if ! grep -q 'location.duplicate' data/events.jsonl; then
    echo "expected duplicate location event in data/events.jsonl" >&2
    exit 1
fi

if ! grep -q '/tmp/loogal-immortal-a/box.png' data/records.jsonl; then
    echo "first indexed directory disappeared from active projection" >&2
    exit 1
fi

./loogal search /tmp/loogal-immortal-a/box.png 90 > /tmp/loogal-immortal-search.txt

echo "IMMORTAL INDEX TEST OK"
echo "  identities first/second: $ids_after_first -> $ids_after_second"
echo "  locations  first/second: $locs_after_first -> $locs_after_second"
echo "  records    first/second: $records_after_first -> $records_after_second"
echo "  events: $(wc -l < data/events.jsonl)"
