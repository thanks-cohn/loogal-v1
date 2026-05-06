# Loogal Additive Intelligent Memory Roadmap

## North Star

Loogal is being built as a local-first visual memory engine.

It should remember visual identities across images, folders, PDFs, comics, screenshots, panels, crops, edits, renames, copies, and future asset pipelines.

The filesystem remembers where files are.

Loogal remembers what visual things are.

## Bedrock Doctrine

Indexing must be boring.

Searching can be clever.

Clustering can be expensive.

Caching can be disposable.

Memory must be sacred.

Loogal should pursue speed through dependability.

Loogal should pursue intelligence through auditability.

Loogal should pursue scale through boring records.

Loogal should pursue space savings through signatures, locations, and clusters instead of duplicate assets.

## Why We Chose This Path

The goal is not to make a flashy scanner.

The goal is to build a bedrock layer that can scale because it is dependable, inspectable, auditable, and boring by design.

Fast systems become dangerous when their truth layer is mysterious.

Loogal must avoid that.

The raw memory must be simple enough to inspect, rebuild, resume, repair, and verify.

That is why the durable layer should be JSONL, logs, signatures, locations, and derived clusters.

The binary index exists for speed.

The JSONL memory exists for truth.

If a cache is lost, Loogal can rebuild it.

If memory is lost, Loogal has forgotten.

## Sacred vs Disposable

### Sacred memory

- data/records.jsonl
- data/signatures.jsonl
- data/locations.jsonl
- data/identities.jsonl
- data/logs/loogal.jsonl

### Disposable acceleration

- data/loogal.bin
- data/cache/
- data/tmp/

## Data Model

### Observation Layer

Append-only truth of what Loogal saw.

Every scan adds observations. It does not erase previous memory.

### Signature Layer

Exact visual signatures are stored once.

If 500 files share the same perceptual hash, Loogal stores one signature and many references.

### Location Layer

Locations explain where a signature appears.

A location may be a normal file path, a PDF page region, a comic panel, or a screenshot region.

### Identity Cluster Layer

Clusters group related signatures.

Exact copies share a signature.

Edited copies become nearby signatures.

Loogal groups nearby signatures into visual identities using delta distance.

Clusters are derived and rebuildable.

Raw observations remain sacred.

## Current Known Issue

src/cmd_index.c currently opens the records file in overwrite mode:

    FILE *f = fopen(LOOGAL_RECORDS_PATH, "w");

This means indexing can erase previous records.

That must change.

The first v3 milestone is append-only records.

## v3.0 — Append-Only Records

Goal: make data/records.jsonl additive by default.

User promise: run index today, run index tomorrow, and Loogal remembers both.

Engineering tasks:

- change records writer from overwrite mode to append-safe behavior
- ensure data/ exists before writing
- add a test proving a second index run does not erase first-run records
- keep data/loogal.bin as rebuildable cache for now
- add a log event when records are appended

Victory condition: two separate indexing runs produce one records file containing observations from both runs.

## v3.1 — Scan State and Skip-Unchanged

Goal: make repeated indexing fast and dependable.

Loogal should skip unchanged files and rehash only new or changed files.

## v3.2 — Unique Signature Store

Goal: store exact visual signatures once.

Copied or renamed files should not create fake new identities.

## v3.3 — Location References

Goal: separate visual identity from file location.

One signature can appear in many places.

## v3.4 — Delta-Aware Clustering

Goal: use the visual delta engine to group near variants.

Exact duplicates are one class.

Near variants are another class.

True outliers remain separate.

## v3.5 — Rebuildable Binary Cache

Goal: keep speed without sacrificing truth.

data/loogal.bin is disposable and rebuildable from durable JSONL memory.

## v4 — Container Intelligence

Goal: find visual identities inside PDFs, comics, long screenshots, and article assets.

Temporary extracted assets should be deleted after identity is derived.

## v5 — Watch Mode

Goal: Loogal becomes persistent visual memory over time.

It watches directories, appends observations, skips unchanged files, and remembers moves, renames, copies, and edits.

## v6 — Desktop Discovery

Goal: drag in an image and find where it appears.

The GUI should expose exact matches, near matches, container matches, history, and locations.

## vFinal — Visual Memory Platform

Loogal becomes local-first visual search infrastructure.

It indexes visual identity across images, PDFs, comics, screenshots, generated assets, document regions, and future media pipelines.

It finds exact matches, near matches, modified variants, embedded appearances, duplicate clusters, outliers, and visual histories.

It is grep for visual memory.

## Final Doctrine

The index must be boring.

The memory must be sacred.

The cache must be disposable.

The clusters must be explainable.

The system must be rebuildable.

That is how Loogal becomes infrastructure.
