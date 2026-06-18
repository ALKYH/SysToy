#include "../../kernel.h"

void run_lab4_scheduler(void) {
    int scheduler_mode;

    console_write_line("[lab4] scheduler demo");
    console_write_line("[lab4] core elements: timer, trap, switch, task pair");
    scheduler_mode = select_scheduler_mode_from_console();
    console_write_line("[lab4] booting mixed task pair: Lab4A + Print");
    load_user_program_from_disk("LAB4A   ELF", USER_STACK_A, "Lab4A");
    load_user_program_from_disk("PRINT   ELF", USER_STACK_B, "Print");
    if (scheduler_mode == 2) {
        console_write_line("[lab4] trace-only mode: inspect task pair loading and timer hooks");
    }
    console_write_line("[lab4] expected observation: timer interrupt interleaves asm and C user tasks");
    console_write_line("[lab4] starting timer-driven scheduler");
    start_task_execution();
}
