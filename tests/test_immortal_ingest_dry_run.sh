#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

rm -rf /tmp/loogal-ingest-root
mkdir -p /tmp/loogal-ingest-root/images
mkdir -p /tmp/loogal-ingest-root/docs
mkdir -p /tmp/loogal-ingest-root/comic_pages

magick -size 64x64 xc:red /tmp/loogal-ingest-root/images/red.png
magick -size 64x64 xc:blue /tmp/loogal-ingest-root/images/blue.jpg
magick -size 64x64 xc:white /tmp/loogal-ingest-root/comic_pages/page_001.png
magick -size 64x64 xc:black /tmp/loogal-ingest-root/comic_pages/page_002.png

printf "fake pdf placeholder\n" > /tmp/loogal-ingest-root/docs/paper.pdf
printf "notes\n" > /tmp/loogal-ingest-root/notes.txt

rm -f data/logs/loogal.jsonl

OUT="$(./loogal ingest /tmp/loogal-ingest-root --all --dry-run)"

echo "$OUT" | grep -q "LOOGAL INGEST DRY RUN"
echo "$OUT" | grep -q "Images found"
echo "$OUT" | grep -q "PDFs found"
echo "$OUT" | grep -q "Article2Assets"
echo "$OUT" | grep -q "Loogal C image core"
echo "$OUT" | grep -q "No indexes changed in dry-run mode."

JSON="$(./loogal ingest /tmp/loogal-ingest-root --all --dry-run --json)"

echo "$JSON" | grep -q '"type": "ingest.plan"'
echo "$JSON" | grep -q '"mode": "all"'
echo "$JSON" | grep -q '"images_found": 4'
echo "$JSON" | grep -q '"pdfs_found": 1'
echo "$JSON" | grep -q '"article2assets": 1'
echo "$JSON" | grep -q '"loogal_c_core": 4'

test -f data/logs/loogal.jsonl
grep -q "ingest.complete" data/logs/loogal.jsonl
grep -q "ingest.classify" data/logs/loogal.jsonl

echo "IMMORTAL INGEST DRY RUN TEST OK"
