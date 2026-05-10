# Ultimate Shard Finder Solution

The best solution is not plain whole-image hashing.

The best solution is not only fixed sharding.

The best solution is a parent-recovery engine built from two layers:

```text
1. local feature voting
2. tile / patch verification
```

This gives Locus the behavior we actually want:

```text
crop.png
-> find the parent image it came from
-> return parent first
-> return approximate location
-> return exact crop duplicate second, if present
```

## Why whole-image hash fails

A perceptual hash of a whole image describes the whole composition.

A crop describes only a local fragment.

So this often fails:

```text
hash(crop)
vs
hash(parent)
```

Even when the crop came directly from the parent.

## Why only fixed tiles is not enough

Fixed tiles help, but a user crop may not align to our tile grid.

The crop may be:

- larger than a tile
- smaller than a tile
- shifted between tiles
- across tile boundaries
- scaled slightly
- recompressed
- screenshot/export altered

So fixed tiles alone can miss.

## Correct architecture

The winning engine is:

```text
INDEX TIME
parent image
-> decode once
-> extract local visual anchors
-> extract multi-scale patch witnesses
-> store compact binary records
-> build buckets / inverted index

QUERY TIME
crop image
-> extract local visual anchors
-> look up matching anchors
-> vote by parent image id
-> verify top parents with patch/tile witnesses
-> return parent + location
```

## Layer 1: Local feature voting

Each image becomes a set of local visual anchors.

A local anchor is a small distinctive point/patch:

```c
typedef struct {
    uint64_t descriptor;
    uint32_t image_id;
    uint16_t x;
    uint16_t y;
    uint16_t scale;
    uint16_t strength;
} ShardAnchor;
```

The crop also produces anchors.

Search becomes:

```text
query anchor
-> descriptor bucket
-> matching parent anchors
-> parent image receives votes
```

The parent with the most coherent votes wins.

This is the missing magic.

## Layer 2: Tile / patch verification

After voting gives top parent candidates, verify them with patch witnesses.

Patch records:

```c
typedef struct {
    uint64_t dhash;
    uint64_t ahash;
    uint32_t image_id;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t scale;
    uint16_t reserved;
} ShardPatch;
```

Verification answers:

```text
Does this crop actually resemble a region inside this parent?
```

## Parent-first ranking

Result ranking must obey:

```text
parent containment beats exact crop echo
```

If the crop itself exists in the indexed folder, it should be shown as:

```text
exact duplicate / query echo
```

not as the primary answer.

## Fast path

The query hot path should be:

```text
query crop
-> extract anchors
-> bucket lookup
-> parent vote counts
-> top-k parent shortlist
-> patch verification
-> return parent
```

No filesystem traversal.
No JSON parsing.
No decoding indexed parent images.
No external tools.

Only binary records.

## Binary files

Target files:

```text
data/shard_anchors.bin      local anchor records
data/shard_anchor_buckets   descriptor prefix -> offset,count
data/shard_patches.bin      patch/tile witnesses
data/shard_paths.tsv        image_id -> path
```

## Why this is the best solution

This is the most practical high-speed C-native design because it combines:

```text
locality
+ voting
+ verification
+ parent-first ranking
+ binary hot path
```

It is faster and more reliable than whole-image hash search.

It is more flexible than fixed-grid tiles alone.

It can recover parents from crops that do not align to tile boundaries.

## Build order

### Step 1

Keep current `locus-shard` v0 working.

### Step 2

Add multi-scale patch extraction:

```text
64x64
96x96
128x128
192x192
```

with stride half the patch size.

### Step 3

Add local anchors:

```text
corner/contrast keypoints
small binary descriptors
bucketed lookup
parent vote ranking
```

### Step 4

Merge scores:

```text
parent_score = anchor_vote_score * 0.65
             + patch_verify_score * 0.30
             + geometry_coherence * 0.05
```

### Step 5

GUI shows:

```text
PARENT FOUND
path
confidence
matched region
exact crop duplicate, if present
```

## Final command contract

```bash
./locus-shard index
./locus-shard find crop.png
```

Expected result:

```text
1. /path/to/parent.png
   reason: parent-contained-shard
   location: x,y,w,h
   confidence: high

2. /path/to/crop.png
   reason: exact-query-duplicate
   confidence: exact
```

This is the real target.
