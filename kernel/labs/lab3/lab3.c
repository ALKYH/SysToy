#include "../../kernel.h"

extern u8 _binary_build_lab3_user_elf_start[];
extern u8 _binary_build_lab3_user_elf_end[];

void run_lab3_memory_and_elf(void) {
    struct embedded_program lab3_program;

    lab3_program.start = _binary_build_lab3_user_elf_start;
    lab3_program.size = (u32)(_binary_build_lab3_user_elf_end - _binary_build_lab3_user_elf_start);
    lab3_program.label = "lab3_user";

    console_write_line("[lab3] memory + ELF loader demo");
    console_write("[lab3] remaining heap before load=");
    console_write_hex(memory_remaining());
    console_write("\n");
    boot_lab_user_program(&lab3_program, USER_STACK_SINGLE);
}
