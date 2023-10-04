#pragma once

#include <stdbool.h>

struct tar_iterator {
    const char *start;
    const char *end;
};

struct tar_iterator *open_tar(const char *start, const char *end);
bool next_file(struct tar_iterator *iter, char *name, char **data, size_t *size);
bool find_file(struct tar_iterator *iter, char *to_find, char **data, size_t *size);
