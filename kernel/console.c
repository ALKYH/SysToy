#include "kernel.h"

#define UART0_BASE 0x10000000u
#define UART_THR   0x00
#define UART_LSR   0x05
#define UART_LSR_EMPTY 0x20

static inline void mmio_write(u32 reg, u8 value) {
    *(volatile u8*)(UART0_BASE + reg) = value;
}

static inline u8 mmio_read(u32 reg) {
    return *(volatile u8*)(UART0_BASE + reg);
}

static void put_char(char c) {
    while ((mmio_read(UART_LSR) & UART_LSR_EMPTY) == 0) {
    }

    if (c == '\n') {
        mmio_write(UART_THR, '\r');
    }
    mmio_write(UART_THR, (u8)c);
}

void serial_init(void) {
}

void console_clear(void) {
}

void console_write(const char* text) {
    while (*text) {
        put_char(*text++);
    }
}

void console_write_line(const char* text) {
    console_write(text);
    put_char('\n');
}

void console_write_hex(u32 value) {
    static const char digits[] = "0123456789ABCDEF";
    int shift;

    console_write("0x");
    for (shift = 28; shift >= 0; shift -= 4) {
        put_char(digits[(value >> shift) & 0xF]);
    }
}
