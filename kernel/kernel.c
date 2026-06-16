#include "kernel.h"

void kmain(void) {
    serial_init();
    console_clear();
    console_write_line("SysToy RISC-V kernel start");

    trap_init();
    lab_selection_loop_entry();
}
