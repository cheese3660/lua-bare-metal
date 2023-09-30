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

static void serial_putb(unsigned char b) {
    while ((inb(0x3f8 + 5) & 0x20) == 0);
    outb(0x3f8, b);
    outb(0xe9, b);
}

static void serial_puts(char *str) {
    while (*str)
        serial_putb(*str ++);
}

void kmain(void) {
    serial_puts(":3c\n");
    while (1);
}
