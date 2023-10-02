/* http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html */

.macro isr_no_err_code num
.globl isr\num
isr\num:
    cli
    push $0
    push $\num
    jmp isr_common_stub
    iret
.endm

.macro isr_err_code num
.globl isr\num
isr\num:
    cli
    push $\num
    jmp isr_common_stub
.endm

isr_no_err_code 0
isr_no_err_code 1
isr_no_err_code 2
isr_no_err_code 3
isr_no_err_code 4
isr_no_err_code 5
isr_no_err_code 6
isr_no_err_code 7
isr_err_code 8
isr_no_err_code 9
isr_err_code 10
isr_err_code 11
isr_err_code 12
isr_err_code 13
isr_err_code 14
isr_no_err_code 15
isr_no_err_code 16
isr_err_code 17
isr_no_err_code 18
isr_no_err_code 19
isr_no_err_code 20
isr_err_code 21
isr_no_err_code 22
isr_no_err_code 23
isr_no_err_code 24
isr_no_err_code 25
isr_no_err_code 26
isr_no_err_code 27
isr_no_err_code 28
isr_err_code 29
isr_err_code 30
isr_no_err_code 31

.extern isr_handler

isr_common_stub:
    pusha

    mov %ds, %ax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    call isr_handler

    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    popa
    add $8, %esp
    sti
    iret
