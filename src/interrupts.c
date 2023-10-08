#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "interrupts.h"
#include "io.h"
#include "rtc.h"
#include "ps2.h"

/* http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html */
struct idt_entry {
    uint16_t base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
    uint16_t sel;                 // Kernel segment selector.
    uint8_t  always0;             // This must always be zero.
    uint8_t  flags;               // More flags. See documentation.
    uint16_t base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;      // The address of the first element in our idt_entry_t array.
} __attribute__((packed));

struct int_registers {
    uint32_t ds;                  // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
};

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr32();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr38();
extern void isr39();
extern void isr40();
extern void isr41();
extern void isr42();
extern void isr43();
extern void isr44();
extern void isr45();
extern void isr46();
extern void isr47();

#define IDT_ENTRIES 256

/* http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html */
static void make_idt_entry(struct idt_entry *entry, uint32_t base, uint16_t sel, uint8_t flags) {
    entry->base_lo = base & 0xFFFF;
    entry->base_hi = (base >> 16) & 0xFFFF;

    entry->sel     = sel;
    //entry->always0 = 0;
    entry->flags   = flags;
} 

void init_idt() {
    // reset PICs
    outb(0x20, 0x11);
    outb(0xa0, 0x11);

    // map primary PIC to interrupt 0x20-0x27
    outb(0x21, 0x20);

    // map secondary PIC to interrupt 0x28-0x2f
    outb(0xa1, 0x28);

    // set up cascading
    outb(0x21, 0x04);
    outb(0xa1, 0x02);

    outb(0x21, 0x01);
    outb(0xa1, 0x01);

    outb(0x21, 0x0);
    outb(0xa1, 0x0);

    struct idt_entry *idt = malloc(sizeof(struct idt_entry) * IDT_ENTRIES);

    if (idt == NULL) {
        printf("couldn't allocate memory!");
        while(1);
    }

    printf("allocated IDT at %08x\n", (uint32_t) idt);

    memset(idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);

    make_idt_entry(&idt[0], (uint32_t) isr0, 0x08, 0x8e);
    make_idt_entry(&idt[1], (uint32_t) isr1, 0x08, 0x8e);
    make_idt_entry(&idt[2], (uint32_t) isr2, 0x08, 0x8e);
    make_idt_entry(&idt[3], (uint32_t) isr3, 0x08, 0x8e);
    make_idt_entry(&idt[4], (uint32_t) isr4, 0x08, 0x8e);
    make_idt_entry(&idt[5], (uint32_t) isr5, 0x08, 0x8e);
    make_idt_entry(&idt[6], (uint32_t) isr6, 0x08, 0x8e);
    make_idt_entry(&idt[7], (uint32_t) isr7, 0x08, 0x8e);
    make_idt_entry(&idt[8], (uint32_t) isr8, 0x08, 0x8e);
    make_idt_entry(&idt[9], (uint32_t) isr9, 0x08, 0x8e);
    make_idt_entry(&idt[10], (uint32_t) isr10, 0x08, 0x8e);
    make_idt_entry(&idt[11], (uint32_t) isr11, 0x08, 0x8e);
    make_idt_entry(&idt[12], (uint32_t) isr12, 0x08, 0x8e);
    make_idt_entry(&idt[13], (uint32_t) isr13, 0x08, 0x8e);
    make_idt_entry(&idt[14], (uint32_t) isr14, 0x08, 0x8e);
    make_idt_entry(&idt[15], (uint32_t) isr15, 0x08, 0x8e);
    make_idt_entry(&idt[16], (uint32_t) isr16, 0x08, 0x8e);
    make_idt_entry(&idt[17], (uint32_t) isr17, 0x08, 0x8e);
    make_idt_entry(&idt[18], (uint32_t) isr18, 0x08, 0x8e);
    make_idt_entry(&idt[19], (uint32_t) isr19, 0x08, 0x8e);
    make_idt_entry(&idt[20], (uint32_t) isr20, 0x08, 0x8e);
    make_idt_entry(&idt[21], (uint32_t) isr21, 0x08, 0x8e);
    make_idt_entry(&idt[22], (uint32_t) isr22, 0x08, 0x8e);
    make_idt_entry(&idt[23], (uint32_t) isr23, 0x08, 0x8e);
    make_idt_entry(&idt[24], (uint32_t) isr24, 0x08, 0x8e);
    make_idt_entry(&idt[25], (uint32_t) isr25, 0x08, 0x8e);
    make_idt_entry(&idt[26], (uint32_t) isr26, 0x08, 0x8e);
    make_idt_entry(&idt[27], (uint32_t) isr27, 0x08, 0x8e);
    make_idt_entry(&idt[28], (uint32_t) isr28, 0x08, 0x8e);
    make_idt_entry(&idt[29], (uint32_t) isr29, 0x08, 0x8e);
    make_idt_entry(&idt[30], (uint32_t) isr30, 0x08, 0x8e);
    make_idt_entry(&idt[31], (uint32_t) isr31, 0x08, 0x8e);
    make_idt_entry(&idt[32], (uint32_t) isr32, 0x08, 0x8e);
    make_idt_entry(&idt[33], (uint32_t) isr33, 0x08, 0x8e);
    make_idt_entry(&idt[34], (uint32_t) isr34, 0x08, 0x8e);
    make_idt_entry(&idt[35], (uint32_t) isr35, 0x08, 0x8e);
    make_idt_entry(&idt[36], (uint32_t) isr36, 0x08, 0x8e);
    make_idt_entry(&idt[37], (uint32_t) isr37, 0x08, 0x8e);
    make_idt_entry(&idt[38], (uint32_t) isr38, 0x08, 0x8e);
    make_idt_entry(&idt[39], (uint32_t) isr39, 0x08, 0x8e);
    make_idt_entry(&idt[40], (uint32_t) isr40, 0x08, 0x8e);
    make_idt_entry(&idt[41], (uint32_t) isr41, 0x08, 0x8e);
    make_idt_entry(&idt[42], (uint32_t) isr42, 0x08, 0x8e);
    make_idt_entry(&idt[43], (uint32_t) isr43, 0x08, 0x8e);
    make_idt_entry(&idt[44], (uint32_t) isr44, 0x08, 0x8e);
    make_idt_entry(&idt[45], (uint32_t) isr45, 0x08, 0x8e);
    make_idt_entry(&idt[46], (uint32_t) isr46, 0x08, 0x8e);
    make_idt_entry(&idt[47], (uint32_t) isr47, 0x08, 0x8e);

    struct idt_ptr idt_ptr = {
        .limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1,
        .base = (uint32_t) idt
    };

    __asm__ __volatile__ (
        "lidt %0"
        :
        : "m" (idt_ptr)
    );
}

/* https://wiki.osdev.org/Exceptions */
const char *interrupt_names[32] = {
    "division error",
    "debug",
    "non-maskable interrupt",
    "breakpoint",
    "overflow",
    "bound range exceeded",
    "invalid opcode",
    "device not available",
    "double fault",
    "coprocessor segment overrun",
    "invalid TSS",
    "segment not present",
    "stack segment fault",
    "general protection fault",
    "page fault",
    "reserved",
    "x87 floating point exception",
    "alignment check",
    "machine check",
    "SIMD floating point exception",
    "virtualization exception",
    "control protection exception",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "hypervisor injection exception",
    "VMM communication exception",
    "security exception",
    "reserved"
};

const char *get_exception_name(uint32_t exception) {
    if (exception < 32)
        return interrupt_names[exception];
    else
        return "unknown";
}

void isr_handler(struct int_registers registers) {
    switch (registers.int_no) {
        case 3:
            printf("caught breakpoint interrupt!\n");
            break;
        case 33:
            ps2_interrupt();
        case 32:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
            outb(0x20, 0x20); // reset primary interrupt controller
            break;
        case 40:
            timer_tick();
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
            outb(0xa0, 0x20); // reset secondary interrupt controller
            outb(0x20, 0x20);
            break;
        default:
            printf("fatal exception %d (%s) at %08x, error code %08x\n", registers.int_no, get_exception_name(registers.int_no), registers.eip, registers.err_code);
            printf("eax = %08x, ebx = %08x, ecx = %08x, edx = %08x\n", registers.eax, registers.ebx, registers.ecx, registers.edx);
            printf("esi = %08x, edi = %08x, ebp = %08x, esp = %08x\n", registers.esi, registers.edi, registers.ebp, registers.useresp);
            printf("eip = %08x, eflags = %08x\n", registers.eip, registers.eflags);
            printf("cs = %04x, ds = %04x, ss = %04x\n", registers.cs & 0xffff, registers.ds & 0xffff, registers.ss & 0xffff);

            while (1)
                __asm__ __volatile__ ("cli; hlt");
    }
}
