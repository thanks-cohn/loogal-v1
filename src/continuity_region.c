#include "continuity_core.h"
#include "loogal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static double overlap_area(
    int ax,
    int ay,
    int aw,
    int ah,
    int bx,
    int by,
    int bw,
    int bh
) {
    int x1 = ax > bx ? ax : bx;
    int y1 = ay > by ? ay : by;
    int x2 = (ax + aw) < (bx + bw) ? (ax + aw) : (bx + bw);
    int y2 = (ay + ah) < (by + bh) ? (ay + ah) : (by + bh);

    int w = x2 - x1;
    int h = y2 - y1;

    if (w <= 0 || h <= 0) return 0.0;

    double intersection = (double)(w * h);
    double union_area =
        (double)(aw * ah) +
        (double)(bw * bh) -
        intersection;

    if (union_area <= 0.0) return 0.0;

    return intersection / union_area;
}

double continuity_region_score(
    uint64_t query_dhash,
    uint64_t target_dhash,
    int qx,
    int qy,
    int qw,
    int qh,
    int tx,
    int ty,
    int tw,
    int th,
    double lineage_weight
) {
    double geometry_overlap = overlap_area(
        qx, qy, qw, qh,
        tx, ty, tw, th
    );

    return continuity_score(
        query_dhash,
        target_dhash,
        geometry_overlap,
        lineage_weight
    );
}
