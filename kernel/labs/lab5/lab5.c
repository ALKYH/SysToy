#include "../../kernel.h"

void run_lab5_synchronization(void) {
    int scheduler_mode;

    console_write_line("[lab5] synchronization demo");
    console_write_line("[lab5] core elements: shared memory, semaphore, paired tasks");
    scheduler_mode = select_scheduler_mode_from_console();
    console_write("[sync] shared unsafe=0x64 safe=0x64 sem=0x1");
    console_write("\n");
    console_write_line("[lab5] expected observation: unsafe and safe slots are updated differently");
    console_write_line("[lab5] booting dedicated Lab5 task pair");
    load_user_program_from_disk("LAB5A   ELF", USER_STACK_A, "Lab5A");
    load_user_program_from_disk("LAB5B   ELF", USER_STACK_B, "Lab5B");
    if (scheduler_mode == 2) {
        console_write_line("[lab5] trace-only mode: inspect semaphore and shared slots");
    }
    console_write_line("[lab5] starting synchronized scheduler");
    start_task_execution();
}
