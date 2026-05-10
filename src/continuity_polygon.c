#include "continuity_core.h"
#include "loogal.h"

#include <stdint.h>
#include <stdio.h>

static double polygon_bounds_overlap(
    int aminx,
    int aminy,
    int amaxx,
    int amaxy,
    int bminx,
    int bminy,
    int bmaxx,
    int bmaxy
) {
    int x1 = aminx > bminx ? aminx : bminx;
    int y1 = aminy > bminy ? aminy : bminy;
    int x2 = amaxx < bmaxx ? amaxx : bmaxx;
    int y2 = amaxy < bmaxy ? amaxy : bmaxy;

    int w = x2 - x1;
    int h = y2 - y1;

    if (w <= 0 || h <= 0) return 0.0;

    double intersection = (double)(w * h);

    double a_area =
        (double)((amaxx - aminx) * (amaxy - aminy));

    double b_area =
        (double)((bmaxx - bminx) * (bmaxy - bminy));

    double union_area = a_area + b_area - intersection;

    if (union_area <= 0.0) return 0.0;

    return intersection / union_area;
}

double continuity_polygon_score(
    uint64_t query_dhash,
    uint64_t target_dhash,
    int aminx,
    int aminy,
    int amaxx,
    int amaxy,
    int bminx,
    int bminy,
    int bmaxx,
    int bmaxy,
    double lineage_weight
) {
    double geometry_overlap = polygon_bounds_overlap(
        aminx,
        aminy,
        amaxx,
        amaxy,
        bminx,
        bminy,
        bmaxx,
        bmaxy
    );

    return continuity_score(
        query_dhash,
        target_dhash,
        geometry_overlap,
        lineage_weight
    );
}
