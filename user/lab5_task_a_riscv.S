.section .text
.globl _start

_start:
    la a0, unsafe_message
    li a1, 13
    li a2, 0
    li a7, 1
    ecall

    li a0, 0
    li a7, 2
    ecall
    addi a1, a0, 5
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
    addi a1, a0, 5
    li a0, 1
    li a7, 3
    ecall
    li a0, 0
    li a7, 5
    ecall

    li a0, 0
    li a7, 6
    ecall

hang:
    j hang

.section .rodata
unsafe_message:
    .asciz "unsafe +5"
safe_message:
    .asciz "safe +5"
