.section .text
.globl _start

_start:
    la a0, world_message
    li a1, 5
    li a2, 0
    li a7, 1
    ecall

    la a0, bang_message
    li a1, 1
    li a2, 0
    li a7, 1
    ecall

    la a0, unsafe_message
    li a1, 13
    li a2, 0
    li a7, 1
    ecall

    li a0, 0
    li a7, 2
    ecall
    addi a1, a0, 7
    li a0, 0
    li a7, 3
    ecall

    la a0, safe_message
    li a1, 11
    li a2, 0
    li a7, 1
    ecall

    li a0, 0
    li a7, 4
    ecall
    li a0, 1
    li a7, 2
    ecall
    addi a1, a0, 7
    li a0, 1
    li a7, 3
    ecall
    li a0, 0
    li a7, 5
    ecall

    li a0, 1
    li a7, 6
    ecall

hang:
    j hang

.section .rodata
world_message:
    .asciz "World"
bang_message:
    .asciz "!"
unsafe_message:
    .asciz "unsafe +7"
safe_message:
    .asciz "safe +7"
