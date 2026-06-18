enum {
    SYSCALL_PRINT = 1,
    SYSCALL_DEMO_DONE = 6,
};

static void do_syscall_print(const char* text, long length) {
    register const char* a0 __asm__("a0") = text;
    register long a1 __asm__("a1") = length;
    register long a2 __asm__("a2") = 0;
    register long a7 __asm__("a7") = SYSCALL_PRINT;

    __asm__ volatile (
        "ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a7)
        : "memory"
    );
}

static void do_syscall_done(void) {
    register long a0 __asm__("a0") = 1;
    register long a7 __asm__("a7") = SYSCALL_DEMO_DONE;

    __asm__ volatile (
        "ecall"
        : "+r"(a0)
        : "r"(a7)
        : "memory"
    );
}

void _start(void) {
    static const char message_one[] = "print syscall from C user program";
    static const char message_two[] = "second line verifies repeated ecall";

    do_syscall_print(message_one, sizeof(message_one) - 1);
    do_syscall_print(message_two, sizeof(message_two) - 1);
    do_syscall_done();

    for (;;) {
        __asm__ volatile ("j .");
    }
}
