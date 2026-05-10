# Shard Speed Model

Locus shard search is not a full-image brute force system.

The correct model is a staged containment pipeline:

```text
index time:
  parent image
  -> overlapping tiles
  -> compact visual witnesses
  -> binary shard records
  -> optional bucket offsets

query time:
  query shard
  -> cheap hash gate
  -> bucket narrowed candidates
  -> score only survivors
  -> return parent image + tile coordinate
```

## Prime law

Do expensive work once at index time.

Do almost nothing at query time.

## Hot path rules

The query hot path must not:

- decode indexed images
- walk the filesystem
- parse JSON
- allocate per candidate
- open parent files
- call external tools
- use floating point until survivor scoring

The query hot path may:

- read mmap binary records
- compare fixed-width integers
- count bits
- track top-k results

## Record model

Each indexed tile record should remain fixed-size:

```c
typedef struct {
    uint64_t dhash;
    uint64_t ahash;
    uint32_t image_id;
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
} LoogalShardTile;
```

Future versions may add:

```c
uint16_t scale_id;
uint16_t bucket;
uint32_t reserved;
```

but only if the speed gain justifies the bytes.

## Query modes

`loogal shard-find query.png` should be hybrid by default:

1. whole-image check for exact/full-image queries
2. tile containment check for crop/shard queries
3. top-k merge ranked by confidence

## Candidate reduction

The fastest practical pipeline:

```text
query dhash prefix
-> bucket range
-> hamming dhash
-> hamming ahash
-> aspect/scale gate
-> top-k heap
```

The ideal index layout:

```text
data/shards.bin          fixed tile records
data/shards.paths        image_id -> path
data/shards.bucket       prefix -> offset,count
```

## Why this wins

A crop search becomes a memory scan over compact records, not an image operation.

For 1,000,000 tiles:

```text
1,000,000 * 32-ish bytes = tens of MB
```

That is cacheable, mmap-friendly, and brutally simple.

## Design target

The demo threshold:

```text
index folder
crop a shard
search returns parent image + location instantly
```

The product threshold:

```text
index entire machine
drop any visual fragment
Locus finds where it lives
```

