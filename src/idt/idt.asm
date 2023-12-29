section .asm

extern int21h_handler
extern no_interrupt_handler
global int21h
global no_interrupt
global idt_load
global enable_interrupts
global disable_interrupts

disable_interrupts:
    cli
    ret

enable_interrupts:
    sti
    ret

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]
    lidt [ebx]

    pop ebp
    ret

int21h:
    cli
    pushad ; push all registers
    call int21h_handler
    popad ; pop all registers
    sti
    iret

no_interrupt:
    cli
    pushad ; push all registers
    call no_interrupt_handler
    popad ; pop all registers
    sti
    iret