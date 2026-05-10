# Shard Result Contract

The shard finder exists to answer one question:

```text
Where does this visual fragment live?
```

It must return the parent image first when the query is a crop or tile from a larger indexed image.

If the exact crop file also exists in the indexed directory, it should be returned as a secondary exact-copy result, not allowed to hide the parent.

## Correct result order

Given:

```text
parent: /images/teacher.png
crop:   /images/crop.png
```

and the crop visually comes from the parent, the ideal output is:

```text
1. parent containing image
   path: /images/teacher.png
   reason: shard-contained-in-parent
   tile: x,y,w,h
   confidence: high

2. exact crop file, if indexed
   path: /images/crop.png
   reason: exact-query-file-or-exact-copy
   confidence: exact
```

The parent is the prize.

The crop itself is evidence, not the final answer.

## Search modes

The default finder should run hybrid retrieval:

```text
query shard
-> exact duplicate check
-> overlapping tile containment check
-> parent-first ranking
-> exact crop secondary ranking
```

## Ranking law

For shard queries:

```text
parent containment beats exact query echo
```

The finder must skip the query file itself unless the user requests:

```bash
--include-self
```

## Future command

```bash
locus-shard find crop.png
```

Expected behavior:

```text
BAM:
- parent image path
- matched tile coordinates
- confidence
- optional exact crop duplicate
```

## GUI contract

The GUI must show:

```text
DROP SHARD
-> PARENT FOUND
-> highlighted tile location
-> lightbox traversal
-> exact crop duplicate below parent if present
```

## Speed contract

Query must not decode indexed parent images.

All parent recovery comes from precomputed tile witnesses:

```text
data/shards.bin
```

The parent image is opened only after it wins.
