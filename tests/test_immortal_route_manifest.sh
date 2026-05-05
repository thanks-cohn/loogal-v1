#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

rm -rf /tmp/loogal-route-root
mkdir -p /tmp/loogal-route-root/images
mkdir -p /tmp/loogal-route-root/docs
mkdir -p /tmp/loogal-route-root/comic_pages

magick -size 64x64 xc:red /tmp/loogal-route-root/images/red.png
magick -size 64x64 xc:blue /tmp/loogal-route-root/images/blue.jpg
magick -size 64x64 xc:white /tmp/loogal-route-root/comic_pages/page_001.png

printf "fake pdf placeholder\n" > /tmp/loogal-route-root/docs/paper.pdf
printf "notes\n" > /tmp/loogal-route-root/notes.txt

rm -f data/routes.jsonl data/logs/loogal.jsonl

OUT="$(./loogal ingest /tmp/loogal-route-root --all --dry-run)"

echo "$OUT" | grep -q "Route manifest"
echo "$OUT" | grep -q "data/routes.jsonl"

test -f data/routes.jsonl

grep -q '"type":"ingest.route"' data/routes.jsonl
grep -q '"route":"loogal-c-core"' data/routes.jsonl
grep -q '"route":"Article2Assets"' data/routes.jsonl
grep -q '"route":"skip"' data/routes.jsonl
grep -q '"status":"planned"' data/routes.jsonl
grep -q '/tmp/loogal-route-root/images/red.png' data/routes.jsonl
grep -q '/tmp/loogal-route-root/docs/paper.pdf' data/routes.jsonl
grep -q '/tmp/loogal-route-root/notes.txt' data/routes.jsonl

JSON="$(./loogal ingest /tmp/loogal-route-root --all --dry-run --json)"

echo "$JSON" | grep -q '"route_manifest": "data/routes.jsonl"'
echo "$JSON" | grep -q '"article2assets": 1'
echo "$JSON" | grep -q '"loogal_c_core": 3'
echo "$JSON" | grep -q '"skipped_unknown": 1'

test -f data/logs/loogal.jsonl
grep -q "ingest.complete" data/logs/loogal.jsonl
grep -q "route_manifest=data/routes.jsonl" data/logs/loogal.jsonl

echo "IMMORTAL ROUTE MANIFEST TEST OK"
