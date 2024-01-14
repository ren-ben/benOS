[BITS 32]

global _start
extern c_start
extern benos_exit

section .asm

_start:
    call c_start
    call benos_exit
    ret