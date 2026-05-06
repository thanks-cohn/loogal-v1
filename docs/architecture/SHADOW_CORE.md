# Loogal Shadow Core

Loogal is moving beyond ImageMagick-backed image hashing.

Old primitive path:

    image -> ImageMagick decode/resize/grayscale -> dhash -> search

New primitive path:

    image -> C-native decode -> C-native normalization -> visual shadow signatures -> search

Doctrine:

    JSONL is truth.
    Binary is speed.
    Routes are handoff.
    Logs are memory.
    Deltas preserve space.
    Rebuild hydrates speed.
    Shadow Core powers visual memory.

Next steps:

    1. Native similarity proof
    2. Shadow Core module
    3. Native ahash64
    4. Fused shadow score
    5. Native phash64
    6. Variants and deltas
    7. Observations for PDFs, comics, long screenshots, and embedded regions

The fastest thing around! We're electricity with nothing but bare metal in our way!
