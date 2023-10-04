#include <assert.h>
#include <stdlib.h>
#include "uuid.h"

char *new_uuid(void) {
    char *buf = malloc(37);
    assert(buf != NULL);

    uint32_t field_a = rand() & 0xffffffff;
    uint16_t field_b = rand() & 0xffff;
    uint16_t field_c = (rand() & 0x0fff) | 0x4000;
    uint16_t field_d = (rand() & 0x3fff) | 0x8000;
    uint64_t field_e = (rand() & 0xffffffff) | ((uint64_t) (rand() & 0xffff) << 32);

    sprintf(buf, "%08lx-%04x-%04x-%04x-%012llx", field_a, field_b, field_c, field_d, field_e);

    buf[36] = 0;

    return buf;
}
