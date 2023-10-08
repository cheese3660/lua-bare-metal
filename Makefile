LDFLAGS += -melf_i386 -Tkernel.ld
CFLAGS += -O2 -m32 -march=i386 -nostartfiles -nostdlib -nostdinc -fno-stack-protector -static -static-libgcc \
	-Ilibc/include -Ilibc/printf/src -Ilibc/arch/x86/include -Ilibc/openlibm/include -Ilibc/openlibm/src \
	-Ilua -Ilibmemory/include -Isrc -include common.h -Wall -g
ASFLAGS += -32 -march=i386

LUA_OBJS = lua/lapi.o lua/lcode.o lua/lctype.o lua/ldebug.o lua/ldo.o lua/ldump.o lua/lfunc.o lua/lgc.o lua/llex.o \
	lua/lmem.o lua/lobject.o lua/lopcodes.o lua/lparser.o lua/lstate.o lua/lstring.o lua/ltable.o \
	lua/ltm.o lua/lundump.o lua/lvm.o lua/lzio.o lua/ltests.o lua/lauxlib.o lua/lbaselib.o lua/ldblib.o \
	lua/lmathlib.o lua/ltablib.o lua/lstrlib.o lua/lutf8lib.o lua/lcorolib.o

OBJECTS = src/init.o src/main.o src/stubs.o src/interrupts.o src/isr.o src/rtc.o src/uuid.o src/tar.o src/ps2.o \
	src/api/computer.o src/api/component.o src/api/unicode.o src/api/os.o \
	src/component/vgatext.o src/component/gpu.o src/component/initrd.o src/component/eeprom.o src/component/vgagraphics.o \
	$(LUA_OBJS) arith64/arith64.o
BINARY = kernel

LIBC_A = libc/buildresults/src/libc.a
LIBMEMORY_A = libmemory/buildresults/src/libmemory_freelist.a
LIBOPENLIBM_A = libc/openlibm/libopenlibm.a
CROSS_INI = $(shell pwd)/cross.ini

.PHONY: all
all: $(BINARY)

$(BINARY): $(LIBC_A) $(LIBMEMORY_A) $(LIBOPENLIBM_A) $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBC_A) $(LIBMEMORY_A) $(LIBOPENLIBM_A) -o $(BINARY)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBC_A):
	cd libc && $(MAKE) OPTIONS=--cross-file="$(CROSS_INI)"

$(LIBMEMORY_A):
	cd libmemory && $(MAKE) OPTIONS=--cross-file="$(CROSS_INI)"

$(LIBOPENLIBM_A):
	cd libc/openlibm && $(MAKE) ARCH=i386 MARCH=i386 CFLAGS=-fno-stack-protector libopenlibm.a

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(BINARY)

.PHONY: clean-all
clean-all: clean
	cd libc && $(MAKE) clean
	cd libmemory && $(MAKE) clean
	cd libc/openlibm && $(MAKE) clean
