#include "../../kernel.h"

void run_lab4_scheduler(void) {
    console_write_line("[lab4] scheduler demo");
    console_write_line("[lab4] booting dedicated Lab4 task pair");
    load_user_program_from_disk("LAB4A   ELF", USER_STACK_A, "Lab4A");
    load_user_program_from_disk("LAB4B   ELF", USER_STACK_B, "Lab4B");
    console_write_line("[lab4] expected observation: timer interrupt drives round-robin switching");
    console_write_line("[lab4] starting timer-driven scheduler");
    start_task_execution();
}
