#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 6) {
        fprintf(stderr, "usage: yolo_to_locus_csv <labels.txt> <image> <width> <height> <category>\n");
        return 1;
    }

    const char *labels = argv[1];
    const char *image = argv[2];
    int iw = atoi(argv[3]);
    int ih = atoi(argv[4]);
    const char *category = argv[5];

    if (iw <= 0 || ih <= 0) {
        fprintf(stderr, "image width and height must be positive\n");
        return 1;
    }

    FILE *f = fopen(labels, "r");
    if (!f) {
        perror(labels);
        return 1;
    }

    puts("image,bbox,name,kind,category,tags,clusters,family,parent,child");

    int cls;
    double cx, cy, w, h;

    while (fscanf(f, "%d %lf %lf %lf %lf", &cls, &cx, &cy, &w, &h) == 5) {
        int bw = (int)(w * iw + 0.5);
        int bh = (int)(h * ih + 0.5);
        int x = (int)(cx * iw - bw / 2.0 + 0.5);
        int y = (int)(cy * ih - bh / 2.0 + 0.5);

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + bw > iw) bw = iw - x;
        if (y + bh > ih) bh = ih - y;
        if (bw <= 0 || bh <= 0) continue;

        printf("%s,\"%d,%d,%d,%d\",class-%d,region,%s,yolo,,,%s,\n",
               image, x, y, bw, bh, cls, category, "");
    }

    fclose(f);
    return 0;
}
