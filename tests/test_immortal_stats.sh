#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

rm -rf /tmp/loogal-stats-a /tmp/loogal-stats-b
mkdir -p /tmp/loogal-stats-a /tmp/loogal-stats-b

magick -size 64x64 xc:red /tmp/loogal-stats-a/red.png
magick -size 64x64 xc:blue /tmp/loogal-stats-b/blue.png
cp /tmp/loogal-stats-a/red.png /tmp/loogal-stats-b/red-copy.png

rm -f data/loogal.bin data/records.jsonl data/identities.jsonl data/locations.jsonl data/events.jsonl data/logs/loogal.jsonl

./loogal index --fresh /tmp/loogal-stats-a >/tmp/loogal-stats-1.out
./loogal index /tmp/loogal-stats-b >/tmp/loogal-stats-2.out

STATS="$(./loogal stats)"

echo "$STATS" | grep -q "Visual identities          : 2"
echo "$STATS" | grep -q "Known locations            : 3"
echo "$STATS" | grep -q "Duplicate references       : 1"
echo "$STATS" | grep -q "Search records             : 3"
echo "$STATS" | grep -q "Binary index               : ok"
echo "$STATS" | grep -q "identities.jsonl           : ok"
echo "$STATS" | grep -q "locations.jsonl            : ok"
echo "$STATS" | grep -q "events.jsonl               : ok"

JSON="$(./loogal stats --json)"

echo "$JSON" | grep -q '"visual_identities": 2'
echo "$JSON" | grep -q '"known_locations": 3'
echo "$JSON" | grep -q '"duplicate_references": 1'
echo "$JSON" | grep -q '"active_search_records": 3'
echo "$JSON" | grep -q '"binary_index": "ok"'

echo "IMMORTAL STATS TEST OK"
