.section .text
.globl _start

_start:
    la a0, memory_message
    li a1, 31
    li a2, 0
    li a7, 1
    ecall

    la a0, elf_message
    li a1, 33
    li a2, 0
    li a7, 1
    ecall

    li a0, 0
    li a7, 6
    ecall

hang:
    j hang

.section .rodata
memory_message:
    .asciz "Lab3 memory allocator is ready"
elf_message:
    .asciz "Lab3 ELF loader entered user mode"
