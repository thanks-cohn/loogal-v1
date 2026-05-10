#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESULTS 24

static GtkWidget *preview;
static GtkWidget *hero;
static GtkWidget *cards;
static GtkWidget *lightbox;
static GtkWidget *lightbox_title;
static GtkWidget *lightbox_image;
static GtkWidget *lightbox_caption;
static int lightbox_index = 0;
static int result_count = 0;
static char result_paths[MAX_RESULTS][2048];
static char result_scores[MAX_RESULTS][64];

static void shell_quote(const char *in, char *out, size_t out_sz) {
    size_t j = 0;
    if (j < out_sz) out[j++] = '\'';
    for (size_t i = 0; in[i] && j + 5 < out_sz; i++) {
        if (in[i] == '\'') {
            out[j++] = '\'';
            out[j++] = '\\';
            out[j++] = '\'';
            out[j++] = '\'';
        } else {
            out[j++] = in[i];
        }
    }
    if (j < out_sz) out[j++] = '\'';
    if (j < out_sz) out[j] = 0;
    else out[out_sz - 1] = 0;
}

static void json_get_string(const char *line, const char *key, char *out, size_t out_sz) {
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(line, pat);
    if (!p) return;
    p = strchr(p, ':');
    if (!p) return;
    p = strchr(p, '"');
    if (!p) return;
    p++;
    size_t j = 0;
    while (*p && *p != '"' && j + 1 < out_sz) {
        if (*p == '\\' && p[1]) p++;
        out[j++] = *p++;
    }
    out[j] = 0;
}

static void json_get_number(const char *line, const char *key, char *out, size_t out_sz) {
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(line, pat);
    if (!p) return;
    p = strchr(p, ':');
    if (!p) return;
    p++;
    while (*p == ' ') p++;
    snprintf(out, out_sz, "%.1f%%", atof(p));
}

static int run_region_search(const char *query_path) {
    char quoted[4096];
    char cmd[8192];
    shell_quote(query_path, quoted, sizeof(quoted));
    snprintf(cmd, sizeof(cmd), "./loogal region-search %s --memory --json --limit 24 --min 40", quoted);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return 0;

    char line[16384];
    int in_result = 0;
    result_count = 0;
    memset(result_paths, 0, sizeof(result_paths));
    memset(result_scores, 0, sizeof(result_scores));

    while (fgets(line, sizeof(line), pipe)) {
        if (strstr(line, "\"rank\"")) in_result = 1;
        if (!in_result) continue;

        if (strstr(line, "\"path\"")) {
            json_get_string(line, "path", result_paths[result_count], sizeof(result_paths[result_count]));
        }
        if (strstr(line, "\"score_percent\"")) {
            json_get_number(line, "score_percent", result_scores[result_count], sizeof(result_scores[result_count]));
        }
        if (strstr(line, "    }") || strstr(line, "    },")) {
            if (result_paths[result_count][0]) {
                if (!result_scores[result_count][0]) snprintf(result_scores[result_count], sizeof(result_scores[result_count]), "match");
                result_count++;
                if (result_count >= MAX_RESULTS) break;
            }
            in_result = 0;
        }
    }

    pclose(pipe);
    return result_count;
}

static void update_lightbox(void) {
    if (result_count <= 0) return;
    char title[512];
    char caption[4096];
    const char *path = result_paths[lightbox_index];
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;

    snprintf(title, sizeof(title), "%s", base);
    snprintf(caption, sizeof(caption),
             "continuity score: %s\nnode %d of %d\n%s",
             result_scores[lightbox_index],
             lightbox_index + 1,
             result_count,
             path);

    gtk_label_set_text(GTK_LABEL(lightbox_title), title);
    gtk_label_set_text(GTK_LABEL(lightbox_caption), caption);
    gtk_picture_set_filename(GTK_PICTURE(lightbox_image), path);
}

static void lightbox_next(GtkButton *button, gpointer data) {
    (void)button; (void)data;
    if (result_count <= 0) return;
    lightbox_index = (lightbox_index + 1) % result_count;
    update_lightbox();
}

static void lightbox_prev(GtkButton *button, gpointer data) {
    (void)button; (void)data;
    if (result_count <= 0) return;
    lightbox_index = (lightbox_index + result_count - 1) % result_count;
    update_lightbox();
}

static void open_lightbox(GtkButton *button, gpointer data) {
    (void)button;
    lightbox_index = GPOINTER_TO_INT(data);
    update_lightbox();
    gtk_widget_set_visible(lightbox, TRUE);
}

static void close_lightbox(GtkButton *button, gpointer data) {
    (void)button; (void)data;
    gtk_widget_set_visible(lightbox, FALSE);
}

static void clear_cards(void) {
    GtkWidget *child = gtk_widget_get_first_child(cards);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(cards), child);
        child = next;
    }
}

static GtkWidget *make_card(const char *path, const char *score, int index) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(row, "card");
    gtk_widget_set_margin_top(row, 6);
    gtk_widget_set_margin_bottom(row, 6);
    gtk_widget_set_margin_start(row, 10);
    gtk_widget_set_margin_end(row, 10);

    GtkWidget *thumb = gtk_picture_new_for_filename(path);
    gtk_picture_set_content_fit(GTK_PICTURE(thumb), GTK_CONTENT_FIT_COVER);
    gtk_widget_set_size_request(thumb, 74, 74);

    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;

    GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *name = gtk_label_new(base);
    GtkWidget *sub = gtk_label_new(path);
    gtk_label_set_ellipsize(GTK_LABEL(name), PANGO_ELLIPSIZE_END);
    gtk_label_set_ellipsize(GTK_LABEL(sub), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_xalign(GTK_LABEL(name), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0f);
    gtk_widget_add_css_class(name, "result-title");
    gtk_widget_add_css_class(sub, "muted");
    gtk_box_append(GTK_BOX(text), name);
    gtk_box_append(GTK_BOX(text), sub);

    GtkWidget *score_label = gtk_label_new(score);
    gtk_widget_add_css_class(score_label, "score");

    GtkWidget *button = gtk_button_new_with_label("Open lightbox");
    g_signal_connect(button, "clicked", G_CALLBACK(open_lightbox), GINT_TO_POINTER(index));

    gtk_box_append(GTK_BOX(row), thumb);
    gtk_box_append(GTK_BOX(row), text);
    gtk_widget_set_hexpand(text, TRUE);
    gtk_box_append(GTK_BOX(row), score_label);
    gtk_box_append(GTK_BOX(row), button);

    return row;
}

static void show_results(void) {
    clear_cards();
    if (result_count <= 0) {
        GtkWidget *row = gtk_label_new("No indexed shard matches yet. Run: ./loogal index <your-image-folder>");
        gtk_widget_add_css_class(row, "muted");
        gtk_box_append(GTK_BOX(cards), row);
        return;
    }
    for (int i = 0; i < result_count; i++) {
        gtk_box_append(GTK_BOX(cards), make_card(result_paths[i], result_scores[i], i));
    }
}

static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data) {
    (void)target; (void)x; (void)y; (void)data;

    GFile *file = g_value_get_object(value);
    if (!file) return FALSE;

    char *path = g_file_get_path(file);
    if (!path) return FALSE;

    gtk_label_set_text(GTK_LABEL(hero), "Searching memory for this shard…");
    gtk_picture_set_filename(GTK_PICTURE(preview), path);

    int hits = run_region_search(path);
    if (hits > 0) gtk_label_set_text(GTK_LABEL(hero), "The computer remembered.");
    else gtk_label_set_text(GTK_LABEL(hero), "No shard matches yet. Index a folder, then drop again.");
    show_results();

    g_free(path);
    return TRUE;
}

static void load_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background: #101114; }"
        ".hero { font-size: 28px; font-weight: 800; color: #f4f4f5; }"
        ".muted { color: #a1a1aa; }"
        ".drop { border: 2px dashed #52525b; border-radius: 18px; padding: 22px; background: #18181b; }"
        ".card { background: #18181b; border-radius: 14px; padding: 12px; }"
        ".result-title { color: #fafafa; font-size: 16px; font-weight: 700; }"
        ".score { color: #a7f3d0; font-size: 18px; font-weight: 800; }"
        ".lightbox { background: #09090b; border-radius: 22px; padding: 18px; }"
        ".lightbox-title { color: #fafafa; font-size: 24px; font-weight: 900; }"
    );
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static GtkWidget *build_lightbox(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(box, "lightbox");
    gtk_widget_set_visible(box, FALSE);

    GtkWidget *top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    lightbox_title = gtk_label_new("Continuity lightbox");
    gtk_widget_add_css_class(lightbox_title, "lightbox-title");
    gtk_label_set_xalign(GTK_LABEL(lightbox_title), 0.0f);
    gtk_widget_set_hexpand(lightbox_title, TRUE);
    GtkWidget *close = gtk_button_new_with_label("Close");
    g_signal_connect(close, "clicked", G_CALLBACK(close_lightbox), NULL);
    gtk_box_append(GTK_BOX(top), lightbox_title);
    gtk_box_append(GTK_BOX(top), close);

    lightbox_image = gtk_picture_new();
    gtk_picture_set_content_fit(GTK_PICTURE(lightbox_image), GTK_CONTENT_FIT_CONTAIN);
    gtk_widget_set_size_request(lightbox_image, 560, 360);

    GtkWidget *nav = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *prev = gtk_button_new_with_label("← Previous");
    GtkWidget *next = gtk_button_new_with_label("Next →");
    g_signal_connect(prev, "clicked", G_CALLBACK(lightbox_prev), NULL);
    g_signal_connect(next, "clicked", G_CALLBACK(lightbox_next), NULL);
    gtk_box_append(GTK_BOX(nav), prev);
    gtk_box_append(GTK_BOX(nav), next);

    lightbox_caption = gtk_label_new("Traverse real indexed matches.");
    gtk_widget_add_css_class(lightbox_caption, "muted");
    gtk_label_set_xalign(GTK_LABEL(lightbox_caption), 0.0f);

    gtk_box_append(GTK_BOX(box), top);
    gtk_box_append(GTK_BOX(box), lightbox_image);
    gtk_box_append(GTK_BOX(box), nav);
    gtk_box_append(GTK_BOX(box), lightbox_caption);
    return box;
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    load_css();

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Locus Continuity Explorer");
    gtk_window_set_default_size(GTK_WINDOW(window), 1180, 820);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_top(root, 18);
    gtk_widget_set_margin_bottom(root, 18);
    gtk_widget_set_margin_start(root, 18);
    gtk_widget_set_margin_end(root, 18);
    gtk_window_set_child(GTK_WINDOW(window), root);

    hero = gtk_label_new("Drop a shard. Recover where it lives.");
    gtk_widget_add_css_class(hero, "hero");
    gtk_label_set_xalign(GTK_LABEL(hero), 0.0f);
    gtk_box_append(GTK_BOX(root), hero);

    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_vexpand(body, TRUE);
    gtk_box_append(GTK_BOX(root), body);

    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(left, "drop");
    gtk_widget_set_hexpand(left, TRUE);
    gtk_widget_set_vexpand(left, TRUE);
    gtk_box_append(GTK_BOX(body), left);

    preview = gtk_picture_new();
    gtk_picture_set_content_fit(GTK_PICTURE(preview), GTK_CONTENT_FIT_CONTAIN);
    gtk_widget_set_vexpand(preview, TRUE);
    gtk_box_append(GTK_BOX(left), preview);

    GtkWidget *hint = gtk_label_new("First run ./loogal index <folder>. Then drag a shard here.");
    gtk_widget_add_css_class(hint, "muted");
    gtk_box_append(GTK_BOX(left), hint);

    GtkDropTarget *target = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
    g_signal_connect(target, "drop", G_CALLBACK(on_drop), NULL);
    gtk_widget_add_controller(left, GTK_EVENT_CONTROLLER(target));

    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(right, 520, -1);
    gtk_box_append(GTK_BOX(body), right);

    cards = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_append(GTK_BOX(right), cards);
    GtkWidget *await = gtk_label_new("Awaiting shard. Real indexed matches will appear here.");
    gtk_widget_add_css_class(await, "muted");
    gtk_box_append(GTK_BOX(cards), await);

    lightbox = build_lightbox();
    gtk_box_append(GTK_BOX(right), lightbox);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.locus.continuity", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
