#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "tar.h"

struct tar_header {
    char name[100];
    char mode[8];
    char owner_uid[8];
    char owner_gid[8];
    char file_size[12];
    char mod_time[12];
    char checksum[8];
    char kind;
    char link_name[100];
    char ustar_indicator[6];
    char ustar_version[2];
    char owner_user_name[32];
    char owner_group_name[32];
    char device_major[8];
    char device_minor[8];
    char filename_prefix[155];
};

struct tar_iterator *open_tar(const char *start, const char *end) {
    struct tar_iterator *iter = malloc(sizeof(struct tar_iterator));
    assert(iter != NULL);

    iter->start = start;
    iter->end = end;

    return iter;
}

/* https://wiki.osdev.org/USTAR */
static int oct2bin(unsigned char *str, int size) {
    int n = 0;
    unsigned char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

bool next_file(struct tar_iterator *iter, char *name, char **data, size_t *size) {
    if (iter->start >= iter->end)
        return false;

    struct tar_header *header = (struct tar_header *) iter->start;
    header->name[99] = 0;
    header->ustar_indicator[5] = 0;
    header->filename_prefix[154] = 0;

    if (strcmp(header->ustar_indicator, "ustar"))
        return false;

    sprintf(name, "%s%s", header->name, header->filename_prefix);
    *data = iter->start + 512;
    *size = oct2bin(&header->file_size, 11);

    iter->start += 512 + ((*size + 511) & ~511);

    return true;
}

bool find_file(struct tar_iterator *iter, char *to_find, char **data, size_t *size) {
    const char name[256];

    if (*(to_find ++) != '/')
        return false;

    while (1) {
        const char *name_ptr = &name;
        if (!next_file(iter, name_ptr, data, size))
            break;

        if (*name_ptr == '/')
            name_ptr++;

        if (strstr(name_ptr, "./") == name_ptr)
            name_ptr += 2;

        if (!strcmp(name, to_find))
            return true;
    }

    return false;
}
