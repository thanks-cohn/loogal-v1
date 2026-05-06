#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

rm -rf /tmp/loogal-verify-a /tmp/loogal-verify-b
mkdir -p /tmp/loogal-verify-a /tmp/loogal-verify-b

python tests/lib/make_png.py red /tmp/loogal-verify-a/red.png
python tests/lib/make_png.py blue /tmp/loogal-verify-b/blue.png
cp /tmp/loogal-verify-a/red.png /tmp/loogal-verify-b/red-copy.png

rm -f data/loogal.bin data/records.jsonl data/identities.jsonl data/locations.jsonl data/events.jsonl data/logs/loogal.jsonl

./loogal index --fresh /tmp/loogal-verify-a >/tmp/loogal-verify-1.out
./loogal index /tmp/loogal-verify-b >/tmp/loogal-verify-2.out

VERIFY="$(./loogal verify)"

echo "$VERIFY" | grep -q "System integrity: OK"
echo "$VERIFY" | grep -q "identities.jsonl lines : 2"
echo "$VERIFY" | grep -q "locations.jsonl lines  : 3"
echo "$VERIFY" | grep -q "records.jsonl lines    : 3"
echo "$VERIFY" | grep -q "binary index readable  : yes"
echo "$VERIFY" | grep -q "orphan locations       : 0"
echo "$VERIFY" | grep -q "location/record parity : 3 == 3"
echo "$VERIFY" | grep -q "duplicate references   : 1"

JSON="$(./loogal verify --json)"

echo "$JSON" | grep -q '"status": "ok"'
echo "$JSON" | grep -q '"visual_identities": 2'
echo "$JSON" | grep -q '"known_locations": 3'
echo "$JSON" | grep -q '"active_records": 3'
echo "$JSON" | grep -q '"duplicate_references": 1'
echo "$JSON" | grep -q '"orphan_locations": 0'
echo "$JSON" | grep -q '"binary_index": "ok"'

echo "IMMORTAL VERIFY TEST OK"
