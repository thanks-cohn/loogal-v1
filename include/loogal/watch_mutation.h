#ifndef LOOGAL_WATCH_MUTATION_H
#define LOOGAL_WATCH_MUTATION_H

int loogal_watch_handle_created(const char *path);
int loogal_watch_handle_modified(const char *path);
int loogal_watch_handle_deleted(const char *path);
int loogal_watch_handle_moved(const char *old_path, const char *new_path);

#endif
