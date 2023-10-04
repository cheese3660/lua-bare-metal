#pragma once

struct tar_iterator {
    const char *start;
    const char *end;
};

struct tar_iterator *open_tar(const char *start, const char *end);
int next_file(struct tar_iterator *iter, char *name, char **data, size_t *size);
int find_file(struct tar_iterator *iter, char *to_find, char **data, size_t *size);
