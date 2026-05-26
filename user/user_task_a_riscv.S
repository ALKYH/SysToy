.section .text
.globl _start

_start:
loop:
    la a0, unsafe_message
    li a1, 12
    li a2, 0
    li a7, 1
    ecall

    li a0, 0
    li a7, 2
    ecall
    addi a1, a0, 10
    li a0, 0
    li a7, 3
    ecall

    la a0, safe_message
    li a1, 10
    li a2, 0
    li a7, 1
    ecall

    li a0, 0
    li a7, 4
    ecall
    li a0, 1
    li a7, 2
    ecall
    addi a1, a0, 10
    li a0, 1
    li a7, 3
    ecall
    li a0, 0
    li a7, 5
    ecall

    li t0, 20000
delay:
    addi t0, t0, -1
    bnez t0, delay
    j loop

.section .rodata
unsafe_message:
    .asciz "TaskA unsafe"
safe_message:
    .asciz "TaskA safe"
