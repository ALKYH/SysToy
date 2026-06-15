#ifndef SYSTOY_KERNEL_H
#define SYSTOY_KERNEL_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

struct loaded_program {
    u8* image;
    u32 entry_point;
    u32 size;
};

struct fat32_file_view {
    const u8* data;
    u32 size;
};

struct trap_frame {
    u32 ra;
    u32 gp;
    u32 tp;
    u32 t0;
    u32 t1;
    u32 t2;
    u32 s0;
    u32 s1;
    u32 a0;
    u32 a1;
    u32 a2;
    u32 a3;
    u32 a4;
    u32 a5;
    u32 a6;
    u32 a7;
    u32 s2;
    u32 s3;
    u32 s4;
    u32 s5;
    u32 s6;
    u32 s7;
    u32 s8;
    u32 s9;
    u32 s10;
    u32 s11;
    u32 t3;
    u32 t4;
    u32 t5;
    u32 t6;
    u32 sp;
};

enum {
    SYSCALL_PRINT = 1,
    SYSCALL_SHARED_READ = 2,
    SYSCALL_SHARED_WRITE = 3,
    SYSCALL_SEM_WAIT = 4,
    SYSCALL_SEM_SIGNAL = 5,
    SYSCALL_DEMO_DONE = 6,
};

void console_clear(void);
void serial_init(void);
void console_write(const char* text);
void console_write_line(const char* text);
void console_write_hex(u32 value);

void memory_init(u32 start, u32 size);
void* memory_alloc(u32 size);
u32 memory_remaining(void);

int elf_load_image(const u8* elf_data, u32 elf_size, struct loaded_program* program);
int fat32_load_root_file(const u8* image, const char* name_11, struct fat32_file_view* file);
u32 syscall_dispatcher(struct trap_frame* frame, u32 current_mepc);
void enter_user_mode(u32 entry_point, u32 user_stack);
void trap_init(void);
void timer_schedule_next(void);
u32 scheduler_handle_timer(struct trap_frame* frame, u32 current_mepc);

#endif
