#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

rm -rf /tmp/loogal-search-a /tmp/loogal-search-b
mkdir -p /tmp/loogal-search-a /tmp/loogal-search-b

magick -size 64x64 xc:red /tmp/loogal-search-a/red.png
magick -size 64x64 xc:blue /tmp/loogal-search-b/blue.png
cp /tmp/loogal-search-a/red.png /tmp/loogal-search-b/red-copy.png

rm -f data/loogal.bin data/records.jsonl data/identities.jsonl data/locations.jsonl data/events.jsonl data/logs/loogal.jsonl

./loogal index --fresh /tmp/loogal-search-a >/tmp/loogal-search-1.out
./loogal index /tmp/loogal-search-b >/tmp/loogal-search-2.out

SEARCH="$(./loogal search /tmp/loogal-search-a/red.png --min 99 --limit 5)"

echo "$SEARCH" | grep -q "Loogal search results:"
echo "$SEARCH" | grep -q "identity_id:"
echo "$SEARCH" | grep -q "known_locations: 2"
echo "$SEARCH" | grep -q "/tmp/loogal-search-a/red.png"
echo "$SEARCH" | grep -q "/tmp/loogal-search-b/red-copy.png"

JSON="$(./loogal search /tmp/loogal-search-a/red.png --min 99 --limit 5 --json)"

echo "$JSON" | grep -q '"engine": "binary_index:dhash:v1"'
echo "$JSON" | grep -q '"identity_id":'
echo "$JSON" | grep -q '"location_count": 2'
echo "$JSON" | grep -q '{"path": "/tmp/loogal-search-a/red.png"}'
echo "$JSON" | grep -q '{"path": "/tmp/loogal-search-b/red-copy.png"}'

echo "IMMORTAL SEARCH IDENTITIES TEST OK"
