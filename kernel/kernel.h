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

struct embedded_program {
    const u8* start;
    u32 size;
    const char* label;
};

struct fat32_file_view {
    const u8* data;
    u32 size;
};

struct fat32_dir_listing {
    char names[16][12];
    u32 count;
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

enum {
    KERNEL_HEAP_START = 0x80400000u,
    KERNEL_HEAP_SIZE = 0x00100000u,
    USER_STACK_A = 0x80600000u,
    USER_STACK_B = 0x80601000u,
    USER_STACK_SINGLE = 0x80602000u,
};

void console_clear(void);
void serial_init(void);
void console_write(const char* text);
void console_write_line(const char* text);
void console_write_hex(u32 value);
char console_read_char(void);

void memory_init(u32 start, u32 size);
void* memory_alloc(u32 size);
u32 memory_remaining(void);

int elf_load_image(const u8* elf_data, u32 elf_size, struct loaded_program* program);
int fat32_load_root_file(const u8* image, const char* name_11, struct fat32_file_view* file);
int fat32_list_root_files(const u8* image, struct fat32_dir_listing* listing);
u32 syscall_dispatcher(struct trap_frame* frame, u32 current_mepc);
void enter_user_mode(u32 entry_point, u32 user_stack);
void start_task_execution(void);
void trap_init(void);
void timer_schedule_next(void);
u32 scheduler_handle_timer(struct trap_frame* frame, u32 current_mepc);
int consume_menu_return_request(void);
void lab_selection_loop_entry(void);
void boot_lab_user_program(const struct embedded_program* program, u32 user_stack);
void load_user_program_from_disk(const char* file_name_11, u32 user_stack, const char* label);
int select_disk_program_from_console(void);
void run_lab1_environment(void);
void run_lab2_boot_and_syscall(void);
void run_lab3_memory_and_elf(void);
void run_lab4_scheduler(void);
void run_lab5_synchronization(void);
void run_lab6_fat32(void);

#endif
