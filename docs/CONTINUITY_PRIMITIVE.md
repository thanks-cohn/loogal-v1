# LOCUS Continuity Primitive

> The machine should remember transformation.

LOCUS is not a gallery, a file browser, an AI photo organizer, or a vector database wrapper.

LOCUS is a continuity engine.

The primitive is simple:

```text
Current computing:
  file == identity

Continuity computing:
  identity survives transformation
```

A thing can be renamed, moved, compressed, screenshotted, cropped, embedded in a PDF, extracted from a comic page, copied into a slide, or saved as a different format without becoming unrelated to itself.

LOCUS exists to preserve that relationship.

## The Core Objects

```text
identity
  what the thing is across time

manifestation
  one visible form of an identity

location
  where a manifestation currently lives

region
  where inside a larger container a manifestation appears

container
  a PDF, page, comic, screenshot, video, archive, or document that can hold manifestations

lineage
  how manifestations relate to each other

witness
  evidence used to prove continuity
```

## What LOCUS Must Prove First

The first human threshold is not a benchmark.

It is this:

```text
user gives LOCUS a bad crop
LOCUS finds the larger source
LOCUS opens the exact page / file / region
```

No hunting.
No guessing.
No dead-end directory reveal.

The click must land.

## The Architecture Law

```text
Truth layer:
  JSONL, receipts, logs, provenance, explanations

Search layer:
  packed binary, mmap-first, fixed layout, zero JSON in hot path
```

The truth layer is for humans and tools.
The search layer is for speed.

Never confuse them.

## The Emotional Promise

You remember what it looked like.

That should be enough.

LOCUS makes it enough.
