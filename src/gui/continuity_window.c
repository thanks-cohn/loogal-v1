#include <gtk/gtk.h>

static GtkWidget *preview;
static GtkWidget *hero;
static GtkWidget *cards;

static void clear_cards(void) {
    GtkWidget *child = gtk_widget_get_first_child(cards);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(cards), child);
        child = next;
    }
}

static GtkWidget *make_card(const char *title, const char *meta, const char *score) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(row, "card");
    gtk_widget_set_margin_top(row, 6);
    gtk_widget_set_margin_bottom(row, 6);
    gtk_widget_set_margin_start(row, 10);
    gtk_widget_set_margin_end(row, 10);

    GtkWidget *thumb = gtk_label_new("◉");
    gtk_widget_add_css_class(thumb, "thumb-dot");
    gtk_widget_set_size_request(thumb, 64, 64);

    GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *name = gtk_label_new(title);
    GtkWidget *sub = gtk_label_new(meta);
    gtk_label_set_xalign(GTK_LABEL(name), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0f);
    gtk_widget_add_css_class(name, "result-title");
    gtk_widget_add_css_class(sub, "muted");
    gtk_box_append(GTK_BOX(text), name);
    gtk_box_append(GTK_BOX(text), sub);

    GtkWidget *score_label = gtk_label_new(score);
    gtk_widget_add_css_class(score_label, "score");

    GtkWidget *button = gtk_button_new_with_label("Expand continuity");

    gtk_box_append(GTK_BOX(row), thumb);
    gtk_box_append(GTK_BOX(row), text);
    gtk_widget_set_hexpand(text, TRUE);
    gtk_box_append(GTK_BOX(row), score_label);
    gtk_box_append(GTK_BOX(row), button);

    return row;
}

static void add_demo_cards(void) {
    clear_cards();
    gtk_box_append(GTK_BOX(cards), make_card("Recovered original manifestation", "same visual identity · likely source image", "98.2%"));
    gtk_box_append(GTK_BOX(cards), make_card("Region witness", "matched bounded fragment · crop survives context", "94.4%"));
    gtk_box_append(GTK_BOX(cards), make_card("Polygon continuity", "shape-aware relation · partial object match", "91.8%"));
    gtk_box_append(GTK_BOX(cards), make_card("Possible derivative", "renamed/exported/screenshot lineage candidate", "87.6%"));
}

static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data) {
    (void)target;
    (void)x;
    (void)y;
    (void)data;

    GFile *file = g_value_get_object(value);
    if (!file) return FALSE;

    char *path = g_file_get_path(file);
    if (!path) return FALSE;

    gtk_label_set_text(GTK_LABEL(hero), "The computer remembered.");
    gtk_picture_set_filename(GTK_PICTURE(preview), path);
    add_demo_cards();

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
        ".thumb-dot { color: #38bdf8; font-size: 34px; }"
    );

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    load_css();

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Locus Continuity Explorer");
    gtk_window_set_default_size(GTK_WINDOW(window), 1120, 760);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_top(root, 18);
    gtk_widget_set_margin_bottom(root, 18);
    gtk_widget_set_margin_start(root, 18);
    gtk_widget_set_margin_end(root, 18);
    gtk_window_set_child(GTK_WINDOW(window), root);

    hero = gtk_label_new("Drop a fragment. Recover its world.");
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

    GtkWidget *hint = gtk_label_new("Drag an image here. Locus will stage continuity candidates.");
    gtk_widget_add_css_class(hint, "muted");
    gtk_box_append(GTK_BOX(left), hint);

    GtkDropTarget *target = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
    g_signal_connect(target, "drop", G_CALLBACK(on_drop), NULL);
    gtk_widget_add_controller(left, GTK_EVENT_CONTROLLER(target));

    cards = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_size_request(cards, 430, -1);
    gtk_box_append(GTK_BOX(body), cards);

    gtk_box_append(GTK_BOX(cards), make_card("Awaiting fragment", "drop image to begin continuity recall", "—"));

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.locus.continuity", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
