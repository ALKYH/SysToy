.section .text
.globl _start

_start:
    la a0, task_message
    li a1, 22
    li a2, 0
    li a7, 1
    ecall

    la a0, schedule_message
    li a1, 18
    li a2, 0
    li a7, 1
    ecall

    li a0, 1
    li a7, 6
    ecall

hang:
    j hang

.section .rodata
task_message:
    .asciz "Lab4 TaskB tick source"
schedule_message:
    .asciz "Lab4 round robin B"
