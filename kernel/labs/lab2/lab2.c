#include "../../kernel.h"

extern u8 _binary_build_lab2_user_elf_start[];
extern u8 _binary_build_lab2_user_elf_end[];

void run_lab2_boot_and_syscall(void) {
    struct embedded_program lab2_program;

    lab2_program.start = _binary_build_lab2_user_elf_start;
    lab2_program.size = (u32)(_binary_build_lab2_user_elf_end - _binary_build_lab2_user_elf_start);
    lab2_program.label = "lab2_user";

    console_write_line("[lab2] phase2 equivalent path on current RISC-V baseline");
    console_write_line("[lab2] boot chain -> trap -> syscall -> user return");
    boot_lab_user_program(&lab2_program, USER_STACK_SINGLE);
}
