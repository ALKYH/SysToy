#include "../../kernel.h"

extern u8 _binary_build_disk_img_start[];
extern u8 _binary_build_disk_img_end[];

int select_disk_program_from_console(void) {
    char selected;

    console_write_line("[lab6] choose FAT32 program");
    console_write_line("1) LAB2.ELF");
    console_write_line("2) LAB3.ELF");
    console_write_line("3) PRINT.ELF");
    console_write_line("4) LAB4A.ELF + LAB4B.ELF");
    console_write_line("5) LAB5A.ELF + LAB5B.ELF");
    console_write_line("Select disk program [1-5]: ");

    for (;;) {
        selected = console_read_char();
        if (selected >= '1' && selected <= '5') {
            console_write("[lab6] selected disk program ");
            {
                char buffer[2];
                buffer[0] = selected;
                buffer[1] = '\0';
                console_write(buffer);
            }
            console_write("\n");
            return (int)(selected - '0');
        }
    }
}

void run_lab6_fat32(void) {
    int selected_program;

    console_write_line("[lab6] FAT32 loader demo");
    console_write_line("[lab6] core elements: root directory, file selection, ELF load");
    console_write("[lab6] disk image bytes=");
    console_write_hex((u32)(_binary_build_disk_img_end - _binary_build_disk_img_start));
    console_write("\n");
    console_write_line("[lab6] split runtime active via kernel/labs/lab6/");
    selected_program = select_disk_program_from_console();

    switch (selected_program) {
    case 1:
        load_user_program_from_disk("LAB2    ELF", USER_STACK_SINGLE, "Lab2");
        break;
    case 2:
        load_user_program_from_disk("LAB3    ELF", USER_STACK_SINGLE, "Lab3");
        break;
    case 3:
        load_user_program_from_disk("PRINT   ELF", USER_STACK_SINGLE, "Print");
        break;
    case 4:
        load_user_program_from_disk("LAB4A   ELF", USER_STACK_A, "Lab4A");
        load_user_program_from_disk("LAB4B   ELF", USER_STACK_B, "Lab4B");
        break;
    case 5:
        load_user_program_from_disk("LAB5A   ELF", USER_STACK_A, "Lab5A");
        load_user_program_from_disk("LAB5B   ELF", USER_STACK_B, "Lab5B");
        break;
    default:
        console_write_line("[lab6] invalid disk selection");
        return;
    }

    start_task_execution();
}
