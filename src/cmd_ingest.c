#define _XOPEN_SOURCE 700

#include "loogal.h"
#include "jsonout.h"
#include "ingest.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ftw.h>
#include <sys/stat.h>

#define LOOGAL_ROUTES_PATH "data/routes.jsonl"

typedef struct {
char root[LOOGAL_PATH_MAX];
int mode_all;
int mode_pdf;
int mode_comics;
int mode_images_only;
int dry_run;
int as_json;

unsigned long long files_seen;
unsigned long long dirs_seen;

unsigned long long images_found;
unsigned long long pdfs_found;
unsigned long long comic_page_images_found;
unsigned long long unknown_found;

unsigned long long route_image_core;
unsigned long long route_article2assets;
unsigned long long route_comic2panel;
unsigned long long route_unknown_skip;
} IngestPlan;

static IngestPlan *g_plan = NULL;

static int has_ext(const char *path, const char *ext) {
const char *dot = strrchr(path, '.');

if (!dot) return 0;

return strcasecmp(dot + 1, ext) == 0;
}

static int is_image_path(const char *path) {
return has_ext(path, "png") ||
has_ext(path, "jpg") ||
has_ext(path, "jpeg") ||
has_ext(path, "webp") ||
has_ext(path, "bmp") ||
has_ext(path, "tif") ||
has_ext(path, "tiff");
}

static int is_pdf_path(const char *path) {
return has_ext(path, "pdf");
}

static int looks_like_page_image(const char *path) {
const char *base = strrchr(path, '/');

base = base ? base + 1 : path;

if (!is_image_path(path)) return 0;

if (strstr(base, "page_")) return 1;
if (strstr(base, "page-")) return 1;
if (strstr(base, "p001")) return 1;
if (strstr(base, "001")) return 1;
if (strstr(base, "002")) return 1;

return 0;
}

static void write_route_manifest_line(const char *path, const char *route, const char *reason) {
FILE *f = fopen(LOOGAL_ROUTES_PATH, "a");

if (!f) {
loogal_log("ingest.route_manifest", "error", "could not open data/routes.jsonl");
return;
}

fprintf(
f,
"{\"type\":\"ingest.route\",\"path\":\"%s\",\"route\":\"%s\",\"reason\":\"%s\",\"status\":\"planned\"}\n",
path,
route,
reason
);

fclose(f);
}

static void log_route(const char *path, const char *route, const char *reason) {
char msg[LOOGAL_PATH_MAX + 256];

snprintf(
msg,
sizeof(msg),
"path=%.300s route=%s reason=%s manifest=%s",
path,
route,
reason,
LOOGAL_ROUTES_PATH
);

loogal_log("ingest.classify", "dry_run", msg);
write_route_manifest_line(path, route, reason);
}

static int ingest_walk_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
(void)sb;
(void)ftwbuf;

if (!g_plan) return 1;

if (typeflag == FTW_D) {
g_plan->dirs_seen++;
return 0;
}

if (typeflag != FTW_F) {
return 0;
}

g_plan->files_seen++;

if (is_image_path(fpath)) {
g_plan->images_found++;

if ((g_plan->mode_all || g_plan->mode_comics) && looks_like_page_image(fpath)) {
g_plan->comic_page_images_found++;
}

if (g_plan->mode_images_only || g_plan->mode_all || (!g_plan->mode_pdf && !g_plan->mode_comics)) {
g_plan->route_image_core++;
log_route(fpath, "loogal-c-core", "image_extension");
}

return 0;
}

if (is_pdf_path(fpath)) {
g_plan->pdfs_found++;

if (g_plan->mode_comics) {
g_plan->route_comic2panel++;
log_route(fpath, "comic2panel", "forced_comics_mode_pdf");
} else if (g_plan->mode_pdf || g_plan->mode_all) {
g_plan->route_article2assets++;
log_route(fpath, "Article2Assets", "pdf_mode_or_all_pdf");
} else {
g_plan->route_unknown_skip++;
log_route(fpath, "skip", "pdf_requires_pdf_or_all_mode");
}

return 0;
}

g_plan->unknown_found++;
g_plan->route_unknown_skip++;
log_route(fpath, "skip", "unsupported_extension");

return 0;
}

static void print_human_plan(IngestPlan *p, double elapsed_ms) {
puts("LOOGAL INGEST DRY RUN");
puts("────────────────────────");
printf("Root: %s\n", p->root);
puts("");

puts("Discovery");
printf("  Directories seen       : %llu\n", p->dirs_seen);
printf("  Files seen             : %llu\n", p->files_seen);
printf("  Images found           : %llu\n", p->images_found);
printf("  PDFs found             : %llu\n", p->pdfs_found);
printf("  Page-like images       : %llu\n", p->comic_page_images_found);
printf("  Unknown files          : %llu\n", p->unknown_found);
puts("");

puts("Planned Routes");
printf("  Loogal C image core    : %llu\n", p->route_image_core);
printf("  Article2Assets         : %llu\n", p->route_article2assets);
printf("  comic2panel            : %llu\n", p->route_comic2panel);
printf("  Skipped / unknown      : %llu\n", p->route_unknown_skip);
puts("");

puts("Mode");
printf("  --all                  : %s\n", p->mode_all ? "yes" : "no");
printf("  --pdf                  : %s\n", p->mode_pdf ? "yes" : "no");
printf("  --comics               : %s\n", p->mode_comics ? "yes" : "no");
printf("  --images-only          : %s\n", p->mode_images_only ? "yes" : "no");
printf("  --dry-run              : %s\n", p->dry_run ? "yes" : "no");
puts("");

puts("Indexes");
puts("  No indexes changed in dry-run mode.");
puts("  This command only discovers, classifies, routes, and logs.");
printf("  Route manifest         : %s\n", LOOGAL_ROUTES_PATH);
puts("");

printf("Duration: %.3f ms\n", elapsed_ms);
}

static void print_json_plan(IngestPlan *p, double elapsed_ms) {
puts("{");
printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
printf("  "); loogal_json_kv_string(stdout, "type", "ingest.plan", 1);
printf("  "); loogal_json_kv_string(stdout, "status", "ok", 1);
printf("  "); loogal_json_kv_string(stdout, "root", p->root, 1);
printf("  "); loogal_json_kv_string(stdout, "route_manifest", LOOGAL_ROUTES_PATH, 1);
printf("  "); loogal_json_kv_string(stdout, "mode", p->mode_all ? "all" : (p->mode_pdf ? "pdf" : (p->mode_comics ? "comics" : (p->mode_images_only ? "images-only" : "images"))), 1);
printf("  "); loogal_json_kv_int(stdout, "dry_run", p->dry_run, 1);
printf("  "); loogal_json_kv_int(stdout, "directories_seen", (long long)p->dirs_seen, 1);
printf("  "); loogal_json_kv_int(stdout, "files_seen", (long long)p->files_seen, 1);
printf("  "); loogal_json_kv_int(stdout, "images_found", (long long)p->images_found, 1);
printf("  "); loogal_json_kv_int(stdout, "pdfs_found", (long long)p->pdfs_found, 1);
printf("  "); loogal_json_kv_int(stdout, "page_like_images", (long long)p->comic_page_images_found, 1);
printf("  "); loogal_json_kv_int(stdout, "unknown_files", (long long)p->unknown_found, 1);
printf("  \"routes\": {\n");
printf("    \"loogal_c_core\": %llu,\n", p->route_image_core);
printf("    \"article2assets\": %llu,\n", p->route_article2assets);
printf("    \"comic2panel\": %llu,\n", p->route_comic2panel);
printf("    \"skipped_unknown\": %llu\n", p->route_unknown_skip);
printf("  },\n");
printf("  \"duration_ms\": %.3f\n", elapsed_ms);
puts("}");
}

int cmd_ingest(int argc, char **argv) {
if (argc < 1) {
puts("LOOGAL INGEST");
puts("Usage:");
puts("  loogal ingest <path> [--all|--pdf|--comics|--images-only] --dry-run [--json]");
loogal_log("ingest", "error", "missing path");
return 1;
}

IngestPlan plan;

memset(&plan, 0, sizeof(plan));

snprintf(plan.root, sizeof(plan.root), "%s", argv[0]);

for (int i = 1; i < argc; i++) {
if (strcmp(argv[i], "--all") == 0) {
plan.mode_all = 1;
continue;
}

if (strcmp(argv[i], "--pdf") == 0) {
plan.mode_pdf = 1;
continue;
}

if (strcmp(argv[i], "--comics") == 0) {
plan.mode_comics = 1;
continue;
}

if (strcmp(argv[i], "--images-only") == 0) {
plan.mode_images_only = 1;
continue;
}

if (strcmp(argv[i], "--dry-run") == 0) {
plan.dry_run = 1;
continue;
}

if (strcmp(argv[i], "--json") == 0) {
plan.as_json = 1;
continue;
}
}

if (!plan.mode_all && !plan.mode_pdf && !plan.mode_comics && !plan.mode_images_only) {
plan.mode_images_only = 1;
}

if (!plan.dry_run) {
puts("LOOGAL INGEST");
puts("────────────────────────");
puts("[ERR] ingest currently requires --dry-run");
puts("");
puts("This module is the safe planner layer.");
puts("Heavy extractor execution comes after the route contract is proven.");
loogal_log("ingest", "error", "missing --dry-run");
return 1;
}

struct stat st;

if (stat(plan.root, &st) != 0) {
puts("LOOGAL INGEST");
puts("────────────────────────");
printf("[ERR] root not found: %s\n", plan.root);
loogal_log("ingest", "error", "root not found");
return 1;
}

remove(LOOGAL_ROUTES_PATH);

double start_ms = loogal_now_ms();

g_plan = &plan;

int rc = nftw(plan.root, ingest_walk_cb, 32, FTW_PHYS);

g_plan = NULL;

double elapsed_ms = loogal_now_ms() - start_ms;

if (rc != 0) {
loogal_log("ingest.walk", "error", plan.root);
return 1;
}

if (plan.as_json) {
print_json_plan(&plan, elapsed_ms);
} else {
print_human_plan(&plan, elapsed_ms);
}

char msg[512];

snprintf(
msg,
sizeof(msg),
"root=%.240s files=%llu images=%llu pdfs=%llu article2assets=%llu comic2panel=%llu image_core=%llu skipped=%llu dry_run=%d route_manifest=data/routes.jsonl duration_ms=%.3f",
plan.root,
plan.files_seen,
plan.images_found,
plan.pdfs_found,
plan.route_article2assets,
plan.route_comic2panel,
plan.route_image_core,
plan.route_unknown_skip,
plan.dry_run,
elapsed_ms
);

loogal_log("ingest.complete", "ok", msg);

return 0;
}
