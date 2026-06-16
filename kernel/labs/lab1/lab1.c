#include "../../kernel.h"

void run_lab1_environment(void) {
    console_write_line("[lab1] environment overview");
    console_write_line("[lab1] Windows 10 + WSL2 + Make + QEMU + RISC-V GCC");
    console_write("[lab1] heap start=");
    console_write_hex(KERNEL_HEAP_START);
    console_write(" size=");
    console_write_hex(KERNEL_HEAP_SIZE);
    console_write("\n");
    console_write_line("[lab1] build entry: make -> Makefile.wsl -> qemu-system-riscv32");
}
