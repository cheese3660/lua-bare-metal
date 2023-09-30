LDFLAGS += -melf_i386 -Tkernel.ld
CFLAGS += -O2 -m32 -march=i386 -nostartfiles -nostdlib -static
ASFLAGS += -32 -march=i386

OBJECTS = init.o main.o
BINARY = kernel

all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(BINARY)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(BINARY)
