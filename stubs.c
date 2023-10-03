#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include "io.h"

/* stubs for otherwise unimplemented functions to either add the most basic required functionality or to fail gracefully */

void putchar_(char c) {
    while ((inb(0x3f8 + 5) & 0x20) == 0);
    outb(0x3f8, c);
    outb(0xe9, c);
}

void write_string(const char *string, int len) {
    for (; len; len --)
        putchar_(*string ++);
}

time_t time(time_t*) {
    return 0;
}

/* TODO: steal this from somewhere better than GCC */
uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem_p) {
    uint64_t quot = 0, qbit = 1;

    if (den == 0) {
        return 1 / ((unsigned) den); /* Intentional divide by zero, without
                                    triggering a compiler warning which
                                    would abort the build */
    }

    /* Left-justify denominator and count shift */
    while ((int64_t) den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }

    while (qbit) {
        if (den <= num) {
            num -= den;
            quot += qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }

    if (rem_p)
        *rem_p = num;

    return quot;
}

int errno = 0;

int feof(FILE*) {
    errno = 1;
    return 0;
}

size_t fread(void* restrict, size_t, size_t, FILE* restrict) {
    errno = 1;
    return 0;
}

int fputs(const char* restrict, FILE* restrict) {
    errno = 1;
    return 0;
}

int fflush(FILE *) {
    errno = 1;
    return 0;
}

size_t fwrite(const void* __restrict, size_t, size_t, FILE* __restrict) {
    errno = 1;
    return 0;
}

int getc(FILE*) {
    errno = 1;
    return 0;
}

FILE* fopen(const char* __restrict, const char* __restrict) {
    errno = 1;
    return NULL;
}

int ferror(FILE*) {
    errno = 1;
    return 0;
}

int fclose(FILE*) {
    errno = 1;
    return 0;
}

FILE* freopen(const char* __restrict, const char* __restrict, FILE* __restrict) {
    errno = 1;
    return NULL;
}

char* fgets(char* __restrict, int, FILE* __restrict) {
    errno = 1;
    return NULL;
}

clock_t clock(void) {
    return 0;
}

void __assert_fail(const char* assertion, const char* file, unsigned int line, const char *function) {
    printf("assertion \"%s\" failed at %s:%d in %s\n", assertion, file, line, function);

    while (1)
        __asm__ __volatile__ ("cli; hlt");
}
