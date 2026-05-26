#include "kernel.h"

#define MIE_MTIE (1u << 7)
#define MCAUSE_ECALL_UMODE 8u
#define MCAUSE_TIMER_INTERRUPT 0x80000007u
#define CLINT_MTIMECMP 0x02004000u
#define CLINT_MTIME    0x0200BFF8u
#define TIMER_INTERVAL 50000u

static inline u32 csr_read_mcause(void) {
    u32 value;
    __asm__ volatile ("csrr %0, mcause" : "=r"(value));
    return value;
}

static inline u32 csr_read_mepc(void) {
    u32 value;
    __asm__ volatile ("csrr %0, mepc" : "=r"(value));
    return value;
}

static inline u32 csr_read_mtval(void) {
    u32 value;
    __asm__ volatile ("csrr %0, mtval" : "=r"(value));
    return value;
}

static inline void csr_write_mepc(u32 value) {
    __asm__ volatile ("csrw mepc, %0" : : "r"(value));
}

static inline void csr_set_mie(u32 bits) {
    __asm__ volatile ("csrs mie, %0" : : "r"(bits));
}

static inline void csr_clear_mie(u32 bits) {
    __asm__ volatile ("csrc mie, %0" : : "r"(bits));
}

static inline u32 csr_read_mie(void) {
    u32 value;
    __asm__ volatile ("csrr %0, mie" : "=r"(value));
    return value;
}

static u64 clint_read_mtime(void) {
    volatile u32* lo = (volatile u32*)CLINT_MTIME;
    volatile u32* hi = (volatile u32*)(CLINT_MTIME + 4);
    u32 hi1;
    u32 lo_val;
    u32 hi2;

    do {
        hi1 = *hi;
        lo_val = *lo;
        hi2 = *hi;
    } while (hi1 != hi2);

    return ((u64)hi2 << 32) | lo_val;
}

void timer_schedule_next(void) {
    volatile u32* cmp_lo = (volatile u32*)CLINT_MTIMECMP;
    volatile u32* cmp_hi = (volatile u32*)(CLINT_MTIMECMP + 4);
    u64 next = clint_read_mtime() + TIMER_INTERVAL;

    *cmp_hi = 0xFFFFFFFFu;
    *cmp_lo = (u32)(next & 0xFFFFFFFFu);
    *cmp_hi = (u32)(next >> 32);
}

void trap_init(void) {
    csr_set_mie(MIE_MTIE);
    timer_schedule_next();
    console_write("[trap_init] mie=");
    console_write_hex(csr_read_mie());
    console_write("\n");
}

void trap_dispatch(struct trap_frame* frame) {
    u32 mcause = csr_read_mcause();

    if (mcause == MCAUSE_ECALL_UMODE) {
        csr_clear_mie(MIE_MTIE);
        console_write("[trap] ecall\n");
        csr_write_mepc(syscall_dispatcher(frame, csr_read_mepc()));
        csr_set_mie(MIE_MTIE);
        return;
    }

    if (mcause == MCAUSE_TIMER_INTERRUPT) {
        console_write("[trap] timer\n");
        csr_write_mepc(scheduler_handle_timer(frame, csr_read_mepc()));
        timer_schedule_next();
        return;
    }

    console_write("[trap] mcause=");
    console_write_hex(mcause);
    console_write(" mepc=");
    console_write_hex(csr_read_mepc());
    console_write(" mtval=");
    console_write_hex(csr_read_mtval());
    console_write("\n");

    for (;;) {
        __asm__ volatile ("wfi");
    }
}
