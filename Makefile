LDFLAGS += -melf_i386 -Tkernel.ld
CFLAGS += -O2 -m32 -march=i386 -nostartfiles -nostdlib -nostdinc -fno-stack-protector -static -static-libgcc \
	-Ilibc/include -Ilibc/printf/src -Ilibc/arch/x86/include -Ilibc/openlibm/include -Ilibc/openlibm/src \
	-Ilua -include common.h
ASFLAGS += -32 -march=i386

LUA_OBJS = lua/lapi.o lua/lcode.o lua/lctype.o lua/ldebug.o lua/ldo.o lua/ldump.o lua/lfunc.o lua/lgc.o lua/llex.o \
	lua/lmem.o lua/lobject.o lua/lopcodes.o lua/lparser.o lua/lstate.o lua/lstring.o lua/ltable.o \
	lua/ltm.o lua/lundump.o lua/lvm.o lua/lzio.o lua/ltests.o lua/lauxlib.o lua/lbaselib.o

MATH_OBJS = libc/openlibm/src/e_fmod.o libc/openlibm/src/e_pow.o libc/openlibm/src/s_frexp.o libc/openlibm/src/s_scalbn.o libc/openlibm/src/e_sqrtf.o

OBJECTS = init.o main.o $(LUA_OBJS) arith64/arith64.o $(MATH_OBJS)
BINARY = kernel

all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) libc/buildresults/src/libc.a -o $(BINARY)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(BINARY)
