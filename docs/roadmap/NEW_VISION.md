# Loogal New Vision

Loogal is a local-first visual identity and provenance engine for filesystems.

The filesystem stores files.
Loogal remembers visual identities.
Binary indexes make that memory fast.
JSONL makes that memory inspectable.
Routes connect that memory to future tools.

## Core Direction

Loogal must move from file-level image memory to visual-thing memory.

A visual thing may be:

- a whole image file
- a resized copy
- a compressed copy
- a cropped region
- a panel inside a comic
- a figure inside a PDF
- a diagram inside an article
- a strip inside a long screenshot
- an embedded image inside a container
- a modified descendant of an original image

Loogal should answer:

How close is this visual thing to something I have seen before, and where exactly does it live?

## Architecture Doctrine

JSONL is truth.
Binary is speed.
Routes are handoff.
Logs are memory.
Rebuild makes it recoverable.

Search should use binary indexes first, then hydrate ranked results from JSONL truth.

## Milestones

### v0.1 Visual Fingerprint

Goal:

loogal add ./test_images
loogal find ./query.png

Required behavior:

100.0% original.png
98.x% resized.jpg
96.x% contrast_shifted.png
92.x% squashed.png

Minimum internals:

- native image decode or small C image library
- grayscale normalization
- fixed-grid resize
- average hash
- difference hash
- ranked Hamming score
- binary visual index
- JSONL observation truth

### v0.2 Region Mesh

Goal:

loogal add ./long_image.png
loogal find ./crop.png

Required behavior:

Find a crop/region inside a larger image and return coordinates.

Minimum internals:

- tile long images
- hash strips/tiles
- store coordinates
- search partial matches
- rank candidates

### v0.3 Container Search

Goal:

loogal add ./papers
loogal find ./figure.png

Required behavior:

Find figures/panels/regions inside PDFs, comics, articles, or screenshots.

Minimum internals:

- container records
- route manifest
- temporary extraction
- fingerprint temporary regions
- delete temporary crops
- keep identity and provenance

## Dependency Doctrine

No hidden dependencies in the primitive.
Adapters are allowed.
Core must stand alone.

The baseline image add/find path should not require ImageMagick.

Optional adapters may use heavier tools for PDFs, OCR, CLIP, ffmpeg, or specialized extraction.
