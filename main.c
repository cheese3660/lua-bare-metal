#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static inline unsigned char inb(unsigned short addr) {
    unsigned char result;

    __asm__ __volatile__ (
        "inb %1, %0"
        : "=a" (result)
        : "dN" (addr)
    );

    return result;
}

static inline void outb(unsigned short addr, unsigned char value) {
    __asm__ __volatile__ (
        "outb %1, %0"
        :
        : "dN" (addr), "a" (value)
    );
}

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

size_t ptr = 4; /* can't be 0 */

void *malloc(size_t size) {
    size_t addr = ptr;
    printf("alloc size %d @ %08x\n", size, ptr);
    ptr += size;
    return (void *) addr;
}

void free(void *ptr) {
    printf("should free %08x\n", (size_t) ptr);
}

void kmain(void) {
    const char *buff = "print(\"UwU OwO\")\n";
    int error;
    const luaL_Reg *lib;
    lua_State *L = luaL_newstate();

    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
    lua_pop(L, 1);

    error = luaL_loadbuffer(L, buff, strlen(buff), "line") || lua_pcall(L, 0, 0, 0);
    if (error) {
        printf("%s", lua_tostring(L, -1));
        lua_pop(L, 1);  /* pop error message from the stack */
    }

    lua_close(L);

    while (1);
}
