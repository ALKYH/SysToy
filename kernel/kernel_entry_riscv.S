.section .text
.globl _start
.globl return_to_lab_selector_from_trap

_start:
    la sp, kernel_stack_top
    la t1, trap_stack_top
    csrw mscratch, t1
    la t0, trap_entry
    csrw mtvec, t0
    call kmain

return_to_lab_selector_from_trap:
    la sp, kernel_stack_top
    j lab_selection_loop_entry

hang:
    wfi
    j hang

.section .bss
.align 4
kernel_stack:
    .space 4096
kernel_stack_top:

.align 4
trap_stack:
    .space 4096
trap_stack_top:
