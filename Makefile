LDFLAGS += -melf_i386 -Tkernel.ld
CFLAGS += -O2 -m32 -march=i386 -nostartfiles -nostdlib -nostdinc -fno-stack-protector -static -static-libgcc \
	-Ilibc/include -Ilibc/printf/src -Ilibc/arch/x86/include -Ilibc/openlibm/include -Ilibc/openlibm/src \
	-Ilua -Ilibmemory/include -include common.h -Wall -g
ASFLAGS += -32 -march=i386

LUA_OBJS = lua/lapi.o lua/lcode.o lua/lctype.o lua/ldebug.o lua/ldo.o lua/ldump.o lua/lfunc.o lua/lgc.o lua/llex.o \
	lua/lmem.o lua/lobject.o lua/lopcodes.o lua/lparser.o lua/lstate.o lua/lstring.o lua/ltable.o \
	lua/ltm.o lua/lundump.o lua/lvm.o lua/lzio.o lua/ltests.o lua/lauxlib.o lua/lbaselib.o

MATH_OBJS = libc/openlibm/src/e_fmod.o libc/openlibm/src/e_pow.o libc/openlibm/src/s_frexp.o libc/openlibm/src/s_scalbn.o libc/openlibm/src/e_sqrtf.o

OBJECTS = init.o main.o stubs.o interrupts.o isr.o $(LUA_OBJS) arith64/arith64.o $(MATH_OBJS)
BINARY = kernel

LIBC_A = libc/buildresults/src/libc.a
LIBMEMORY_A = libmemory/buildresults/src/libmemory_freelist.a
CROSS_INI = $(shell pwd)/cross.ini

TO_LINK = $(OBJECTS) $(LIBC_A) $(LIBMEMORY_A)

.PHONY: all
all: $(BINARY)

$(BINARY): $(TO_LINK)
	$(LD) $(LDFLAGS) $(TO_LINK) -o $(BINARY)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBC_A):
	cd libc && make OPTIONS=--cross-file="$(CROSS_INI)"

$(LIBMEMORY_A):
	cd libmemory && make OPTIONS=--cross-file="$(CROSS_INI)"

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(BINARY)
