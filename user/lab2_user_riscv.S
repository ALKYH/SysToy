.section .text
.globl _start

_start:
    la a0, phase2_message
    li a1, 32
    li a2, 0
    li a7, 1
    ecall

    la a0, syscall_message
    li a1, 26
    li a2, 0
    li a7, 1
    ecall

    li a0, 0
    li a7, 6
    ecall

hang:
    j hang

.section .rodata
phase2_message:
    .asciz "Lab2 user program entered U-mode"
syscall_message:
    .asciz "Lab2 syscall path is alive"
