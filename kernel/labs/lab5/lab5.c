#include "../../kernel.h"

void run_lab5_synchronization(void) {
    console_write_line("[lab5] synchronization demo");
    console_write("[sync] shared unsafe=");
    console_write_hex(100);
    console_write(" safe=");
    console_write_hex(100);
    console_write(" sem=");
    console_write_hex(1);
    console_write("\n");
    console_write_line("[lab5] expected observation: unsafe and safe slots are updated differently");
    console_write_line("[lab5] booting dedicated Lab5 task pair");
    load_user_program_from_disk("LAB5A   ELF", USER_STACK_A, "Lab5A");
    load_user_program_from_disk("LAB5B   ELF", USER_STACK_B, "Lab5B");
    console_write_line("[lab5] starting synchronized scheduler");
    start_task_execution();
}
