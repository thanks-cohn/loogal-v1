# OVERHAUL 001 — CONTINUITY SPINE

This is the first major LOCUS overhaul.

The purpose is not feature accumulation.

The purpose is to transform LOCUS from:

```text
local image search tool
```

into:

```text
continuity infrastructure
```

---

# THE NEW REPOSITORY DIRECTION

LOCUS is now organized around these concepts:

```text
identity
manifestation
location
region
container
lineage
witness
```

These are not marketing terms.

They are the primitive.

---

# FIRST TARGET

The first major threshold is:

```text
crop → original source → exact landing
```

Example:

```bash
locus region-search crop.png ~/archive --land
```

LOCUS should:

- find the original image
- find screenshots
- find PDF manifestations
- find comic panels
- jump directly to the exact region

This is the first:

```text
HOLY SHIT.
```

moment.

---

# MODULES TO ADD

## shardscan

Purpose:

```text
image inside image recovery
```

Responsibilities:

- region extraction
- multi-scale matching
- crop recovery
- topology scoring
- bbox generation

---

## docmesh

Purpose:

```text
document continuity
```

Responsibilities:

- PDF region extraction
- figure indexing
- OCR witnesses
- page lineage
- embedded image continuity

---

## comica bridge

Purpose:

```text
comic/manga continuity
```

Responsibilities:

- panel extraction interoperability
- OCR topology
- panel lineage
- speech bubble witnesses

---

## continuityd

Purpose:

```text
continuous additive indexing
```

Responsibilities:

- watch folders
- skip unchanged files
- append continuity events
- rebuild hot binary layers

---

# NON-NEGOTIABLE LAWS

## LAW 1

No JSON in the hot search path.

## LAW 2

No silent failure.

## LAW 3

Every result must explain itself.

## LAW 4

Every meaningful push ships visible value.

## LAW 5

Exact landing matters more than pretty search.

---

# THE FINAL EMOTIONAL TARGET

```text
How was this NOT already part of computing?
```
