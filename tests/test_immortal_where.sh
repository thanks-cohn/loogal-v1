#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

rm -rf /tmp/loogal-where-a /tmp/loogal-where-b
mkdir -p /tmp/loogal-where-a /tmp/loogal-where-b

python tests/lib/make_png.py red /tmp/loogal-where-a/red.png
python tests/lib/make_png.py blue /tmp/loogal-where-b/blue.png
cp /tmp/loogal-where-a/red.png /tmp/loogal-where-b/red-copy.png

rm -f data/loogal.bin data/records.jsonl data/identities.jsonl data/locations.jsonl data/events.jsonl data/logs/loogal.jsonl

./loogal index --fresh /tmp/loogal-where-a >/tmp/loogal-where-1.out
./loogal index /tmp/loogal-where-b >/tmp/loogal-where-2.out

WHERE="$(./loogal where /tmp/loogal-where-a/red.png)"

echo "$WHERE" | grep -q "Status: found"
echo "$WHERE" | grep -q "Identity ID:"
echo "$WHERE" | grep -q "Locations: 2"
echo "$WHERE" | grep -q "/tmp/loogal-where-a/red.png"
echo "$WHERE" | grep -q "/tmp/loogal-where-b/red-copy.png"

JSON="$(./loogal where /tmp/loogal-where-a/red.png --json)"

echo "$JSON" | grep -q '"status": "found"'
echo "$JSON" | grep -q '"identity_id": 1'
echo "$JSON" | grep -q '"location_count": 2'
echo "$JSON" | grep -q '{"path": "/tmp/loogal-where-a/red.png"}'
echo "$JSON" | grep -q '{"path": "/tmp/loogal-where-b/red-copy.png"}'

echo "IMMORTAL WHERE TEST OK"
