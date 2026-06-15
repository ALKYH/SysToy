#include "kernel.h"

extern u8 _binary_build_lab2_user_elf_start[];
extern u8 _binary_build_lab2_user_elf_end[];
extern u8 _binary_build_lab3_user_elf_start[];
extern u8 _binary_build_lab3_user_elf_end[];
extern u8 _binary_build_lab4_task_a_elf_start[];
extern u8 _binary_build_lab4_task_a_elf_end[];
extern u8 _binary_build_lab4_task_b_elf_start[];
extern u8 _binary_build_lab4_task_b_elf_end[];
extern u8 _binary_build_lab5_task_a_elf_start[];
extern u8 _binary_build_lab5_task_a_elf_end[];
extern u8 _binary_build_lab5_task_b_elf_start[];
extern u8 _binary_build_lab5_task_b_elf_end[];
extern u8 _binary_build_disk_img_start[];
extern u8 _binary_build_disk_img_end[];

static const u32 HEAP_START = 0x80400000;
static const u32 HEAP_SIZE = 0x00100000;
static const u32 USER_STACK_A = 0x80600000;
static const u32 USER_STACK_B = 0x80601000;
static const u32 USER_STACK_SINGLE = 0x80602000;
static const u32 MSTATUS_MPIE = (1u << 7);
static const u32 SHARED_SLOT_ACCOUNT_UNSAFE = 0;
static const u32 SHARED_SLOT_ACCOUNT_SAFE = 1;
static const u32 SHARED_SLOT_SEM_ID = 0;

struct task {
    struct trap_frame frame;
    u32 mepc;
    u32 image_base;
    const char* label;
};

struct semaphore {
    u32 value;
};

struct embedded_program {
    const u8* start;
    u32 size;
    const char* label;
};

static struct task tasks[2];
static int task_count = 0;
static int current_task = 0;
static int demo_done[2];
static int demo_reported = 0;
static int selected_lab = 0;
static u32 shared_words[4];
static struct semaphore semaphores[2];

static void report_demo_summary(void);
static void reset_runtime_state(void);
static void add_task(u32 entry, u32 user_stack, u32 image_base, const char* label);
static void console_write_u32(u32 value);
static void boot_lab_user_program(const struct embedded_program* program, u32 user_stack);
static void load_user_program_from_disk(const char* file_name_11, u32 user_stack, const char* label);
static void print_lab_menu(void);
static int select_lab_from_console(void);
static int select_disk_program_from_console(void);
static void print_disk_listing(void);
static void run_lab1_environment(void);
static void run_lab2_boot_and_syscall(void);
static void run_lab3_memory_and_elf(void);
static void run_lab4_scheduler(void);
static void run_lab5_synchronization(void);
static void run_lab6_fat32(void);

static void memory_zero(void* dst, u32 size) {
    u8* bytes = (u8*)dst;
    u32 i;
    for (i = 0; i < size; ++i) {
        bytes[i] = 0;
    }
}

static void memory_copy_block(void* dst, const void* src, u32 size) {
    u8* out = (u8*)dst;
    const u8* in = (const u8*)src;
    u32 i;
    for (i = 0; i < size; ++i) {
        out[i] = in[i];
    }
}

static inline void csr_write_mepc(u32 value) {
    __asm__ volatile ("csrw mepc, %0" : : "r"(value));
}

static inline u32 csr_read_mstatus(void) {
    u32 value;
    __asm__ volatile ("csrr %0, mstatus" : "=r"(value));
    return value;
}

static inline void csr_write_mstatus(u32 value) {
    __asm__ volatile ("csrw mstatus, %0" : : "r"(value));
}

static inline void csr_write_pmpaddr0(u32 value) {
    __asm__ volatile ("csrw pmpaddr0, %0" : : "r"(value));
}

static inline void csr_write_pmpcfg0(u32 value) {
    __asm__ volatile ("csrw pmpcfg0, %0" : : "r"(value));
}

static void add_task(u32 entry, u32 user_stack, u32 image_base, const char* label) {
    struct task* task = &tasks[task_count++];
    memory_zero(&task->frame, sizeof(task->frame));
    task->frame.sp = user_stack;
    task->mepc = entry;
    task->image_base = image_base;
    task->label = label;
}

static void console_write_u32(u32 value) {
    char buffer[11];
    int index = 10;

    buffer[index] = '\0';
    if (value == 0) {
        buffer[--index] = '0';
    } else {
        while (value > 0 && index > 0) {
            buffer[--index] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    console_write(&buffer[index]);
}

static void reset_runtime_state(void) {
    task_count = 0;
    current_task = 0;
    demo_reported = 0;
    demo_done[0] = 0;
    demo_done[1] = 0;
    shared_words[SHARED_SLOT_ACCOUNT_UNSAFE] = 100;
    shared_words[SHARED_SLOT_ACCOUNT_SAFE] = 100;
    semaphores[SHARED_SLOT_SEM_ID].value = 1;
}

static u32 semaphore_wait(u32 sem_id, u32 current_mepc) {
    if (sem_id >= 2) {
        console_write("[sync] bad sem wait id=");
        console_write_hex(sem_id);
        console_write("\n");
        return current_mepc + 4;
    }

    if (semaphores[sem_id].value == 0) {
        return current_mepc;
    }

    semaphores[sem_id].value -= 1;
    return current_mepc + 4;
}

static u32 semaphore_signal(u32 sem_id, u32 current_mepc) {
    if (sem_id >= 2) {
        console_write("[sync] bad sem signal id=");
        console_write_hex(sem_id);
        console_write("\n");
        return current_mepc + 4;
    }

    semaphores[sem_id].value += 1;
    return current_mepc + 4;
}

u32 syscall_dispatcher(struct trap_frame* frame, u32 current_mepc) {
    u32 call_id = frame->a7;
    u32 arg0 = frame->a0;
    u32 arg1 = frame->a1;
    u32 arg2 = frame->a2;
    u32 i;
    const char* text = (const char*)arg0;

    switch (call_id) {
    case SYSCALL_PRINT:
        if (task_count > 0 && arg2 == 0 && arg0 < HEAP_START) {
            text = (const char*)(tasks[current_task].image_base + arg0);
        }
        console_write("[syscall] ");
        if (arg2 == 0) {
            for (i = 0; i < arg1; ++i) {
                if (text[i] == '\0') {
                    break;
                }
                {
                    char buffer[2];
                    buffer[0] = text[i];
                    buffer[1] = '\0';
                    console_write(buffer);
                }
            }
        } else {
            u32 packed[2];
            packed[0] = arg0;
            packed[1] = arg1;

            for (i = 0; i < arg2 && i < sizeof(packed); ++i) {
                char c = ((char*)packed)[i];
                char buffer[2];
                if (c == '\0') {
                    break;
                }
                buffer[0] = c;
                buffer[1] = '\0';
                console_write(buffer);
            }
        }
        console_write("\n");
        return current_mepc + 4;

    case SYSCALL_SHARED_READ:
        if (arg0 < 4) {
            frame->a0 = shared_words[arg0];
        } else {
            frame->a0 = 0;
        }
        return current_mepc + 4;

    case SYSCALL_SHARED_WRITE:
        if (arg0 < 4) {
            shared_words[arg0] = arg1;
        }
        return current_mepc + 4;

    case SYSCALL_SEM_WAIT:
        return semaphore_wait(arg0, current_mepc);

    case SYSCALL_SEM_SIGNAL:
        return semaphore_signal(arg0, current_mepc);

    case SYSCALL_DEMO_DONE:
        if (task_count < 2) {
            console_write("[demo] single program done: ");
            if (task_count > 0 && tasks[0].label) {
                console_write(tasks[0].label);
            } else {
                console_write("unknown");
            }
            console_write("\n");
            for (;;) {
                __asm__ volatile ("wfi");
            }
        }

        if (arg0 < 2) {
            demo_done[arg0] = 1;
            console_write("[demo] task done: ");
            console_write(arg0 == 0 ? "TaskA" : "TaskB");
            console_write("\n");

            if (demo_done[0] && demo_done[1]) {
                report_demo_summary();
                for (;;) {
                    __asm__ volatile ("wfi");
                }
            }
        }
        return current_mepc + 4;

    default:
        console_write("[syscall] unknown call: ");
        console_write_hex(call_id);
        console_write("\n");
        return current_mepc + 4;
    }
}

u32 scheduler_handle_timer(struct trap_frame* frame, u32 current_mepc) {
    int next_task;
    u32 saved_user_sp;

    if (task_count < 2) {
        return current_mepc;
    }

    if (demo_done[current_task]) {
        next_task = (current_task + 1) % task_count;
        if (demo_done[next_task]) {
            return current_mepc;
        }
        console_write("[sched] resume ");
        console_write(tasks[next_task].label);
        console_write("\n");
        current_task = next_task;
        memory_copy_block(frame, &tasks[current_task].frame, sizeof(*frame));
        return tasks[current_task].mepc;
    }

    saved_user_sp = tasks[current_task].frame.sp;
    memory_copy_block(&tasks[current_task].frame, frame, sizeof(*frame));
    tasks[current_task].frame.sp = saved_user_sp;
    tasks[current_task].mepc = current_mepc;

    next_task = (current_task + 1) % task_count;
    console_write("[sched] ");
    console_write(tasks[current_task].label);
    console_write(" -> ");
    console_write(tasks[next_task].label);
    console_write("\n");
    current_task = next_task;

    memory_copy_block(frame, &tasks[current_task].frame, sizeof(*frame));
    return tasks[current_task].mepc;
}

void enter_user_mode(u32 entry_point, u32 user_stack) {
    u32 mstatus = csr_read_mstatus();

    csr_write_pmpaddr0(0xFFFFFFFFu);
    csr_write_pmpcfg0(0x0Fu);
    csr_write_mepc(entry_point);
    mstatus &= ~(3u << 11);
    mstatus |= MSTATUS_MPIE;
    csr_write_mstatus(mstatus);

    __asm__ volatile (
        "mv sp, %0\n\t"
        "mret\n\t"
        :
        : "r"(user_stack)
        : "memory"
    );
}

static void boot_lab_user_program(const struct embedded_program* program, u32 user_stack) {
    struct loaded_program loaded;
    int load_result;

    console_write("[lab] loading embedded program: ");
    console_write(program->label);
    console_write("\n");

    load_result = elf_load_image(program->start, program->size, &loaded);
    if (load_result != 0) {
        console_write("[elf] embedded load failed: ");
        console_write_hex((u32)load_result);
        console_write("\n");
        for (;;) {
            __asm__ volatile ("wfi");
        }
    }

    add_task(loaded.entry_point, user_stack, (u32)loaded.image, program->label);
    enter_user_mode(tasks[0].mepc, tasks[0].frame.sp);
}

static void load_user_program_from_disk(const char* file_name_11, u32 user_stack, const char* label) {
    struct fat32_file_view file;
    struct loaded_program program;
    int load_result;
    int file_result;
    u32 disk_size = (u32)(_binary_build_disk_img_end - _binary_build_disk_img_start);

    console_write(label);
    console_write(" disk image size: ");
    console_write_hex(disk_size);
    console_write("\n");

    file_result = fat32_load_root_file(_binary_build_disk_img_start, file_name_11, &file);
    if (file_result != 0) {
        console_write("[fat32] file lookup failed: ");
        console_write_hex((u32)file_result);
        console_write("\n");
        for (;;) {
            __asm__ volatile ("wfi");
        }
    }

    console_write(label);
    console_write(" FAT32 file size: ");
    console_write_hex(file.size);
    console_write("\n");

    load_result = elf_load_image(file.data, file.size, &program);
    if (load_result != 0) {
        console_write("[elf] disk load failed: ");
        console_write_hex((u32)load_result);
        console_write("\n");
        for (;;) {
            __asm__ volatile ("wfi");
        }
    }

    console_write(label);
    console_write(" FAT32 entry: ");
    console_write_hex(program.entry_point);
    console_write("\n");

    add_task(program.entry_point, user_stack, (u32)program.image, label);
}

static void report_demo_summary(void) {
    if (demo_reported) {
        return;
    }
    demo_reported = 1;

    console_write("[demo] ");
    console_write("unsafe final=");
    console_write_u32(shared_words[SHARED_SLOT_ACCOUNT_UNSAFE]);
    console_write(" safe final=");
    console_write_u32(shared_words[SHARED_SLOT_ACCOUNT_SAFE]);
    console_write("\n");

    console_write("[demo] ");
    console_write("features: boot -> trap -> syscall -> FAT32 -> ELF -> schedule -> shared memory -> semaphore");
    console_write("\n");
}

static void print_lab_menu(void) {
    console_write_line("");
    console_write_line("SysToy unified lab selector");
    console_write_line("1) Lab1 environment overview");
    console_write_line("2) Lab2 boot / syscall user program");
    console_write_line("3) Lab3 memory + ELF loader");
    console_write_line("4) Lab4 scheduler");
    console_write_line("5) Lab5 synchronization");
    console_write_line("6) Lab6 FAT32 loader");
    console_write_line("Select lab [1-6]: ");
}

static int select_lab_from_console(void) {
    char selected;

    print_lab_menu();
    for (;;) {
        selected = console_read_char();
        if (selected >= '1' && selected <= '6') {
            console_write("[menu] selected lab ");
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

static int select_disk_program_from_console(void) {
    char selected;

    print_disk_listing();
    console_write_line("[lab6] choose FAT32 program");
    console_write_line("1) LAB2.ELF");
    console_write_line("2) LAB3.ELF");
    console_write_line("3) LAB4A.ELF + LAB4B.ELF");
    console_write_line("4) LAB5A.ELF + LAB5B.ELF");
    console_write_line("Select disk program [1-4]: ");

    for (;;) {
        selected = console_read_char();
        if (selected >= '1' && selected <= '4') {
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

static void print_disk_listing(void) {
    struct fat32_dir_listing listing;
    u32 idx;
    int result;

    result = fat32_list_root_files(_binary_build_disk_img_start, &listing);
    if (result != 0) {
        console_write("[lab6] list root files failed: ");
        console_write_hex((u32)result);
        console_write("\n");
        return;
    }

    console_write_line("[lab6] FAT32 root directory:");
    for (idx = 0; idx < listing.count; ++idx) {
        console_write("  - ");
        console_write(listing.names[idx]);
        console_write("\n");
    }
}

static void run_lab1_environment(void) {
    console_write_line("[lab1] environment overview");
    console_write_line("[lab1] Windows 10 + WSL2 + Make + QEMU + RISC-V GCC");
    console_write("[lab1] heap start=");
    console_write_hex(HEAP_START);
    console_write(" size=");
    console_write_hex(HEAP_SIZE);
    console_write("\n");
    console_write_line("[lab1] build entry: make -> Makefile.wsl -> qemu-system-riscv32");
}

static void run_lab2_boot_and_syscall(void) {
    struct embedded_program lab2_program;

    lab2_program.start = _binary_build_lab2_user_elf_start;
    lab2_program.size = (u32)(_binary_build_lab2_user_elf_end - _binary_build_lab2_user_elf_start);
    lab2_program.label = "lab2_user";

    console_write_line("[lab2] phase2 equivalent path on current RISC-V baseline");
    console_write_line("[lab2] boot chain -> trap -> syscall -> user return");
    boot_lab_user_program(&lab2_program, USER_STACK_SINGLE);
}

static void run_lab3_memory_and_elf(void) {
    struct embedded_program lab3_program;

    lab3_program.start = _binary_build_lab3_user_elf_start;
    lab3_program.size = (u32)(_binary_build_lab3_user_elf_end - _binary_build_lab3_user_elf_start);
    lab3_program.label = "lab3_user";

    console_write_line("[lab3] memory + ELF loader demo");
    console_write("[lab3] remaining heap before load=");
    console_write_hex(memory_remaining());
    console_write("\n");
    boot_lab_user_program(&lab3_program, USER_STACK_SINGLE);
}

static void run_lab4_scheduler(void) {
    console_write_line("[lab4] scheduler demo");
    console_write_line("[lab4] booting dedicated Lab4 task pair");
    load_user_program_from_disk("LAB4A   ELF", USER_STACK_A, "Lab4A");
    load_user_program_from_disk("LAB4B   ELF", USER_STACK_B, "Lab4B");
    console_write_line("[lab4] expected observation: timer interrupt drives round-robin switching");
    console_write_line("[lab4] starting timer-driven scheduler");
    current_task = 0;
    enter_user_mode(tasks[0].mepc, tasks[0].frame.sp);
}

static void run_lab5_synchronization(void) {
    console_write_line("[lab5] synchronization demo");
    console_write("[sync] shared unsafe=");
    console_write_hex(shared_words[SHARED_SLOT_ACCOUNT_UNSAFE]);
    console_write(" safe=");
    console_write_hex(shared_words[SHARED_SLOT_ACCOUNT_SAFE]);
    console_write(" sem=");
    console_write_hex(semaphores[SHARED_SLOT_SEM_ID].value);
    console_write("\n");
    console_write_line("[lab5] expected observation: unsafe and safe slots are updated differently");
    console_write_line("[lab5] booting dedicated Lab5 task pair");
    load_user_program_from_disk("LAB5A   ELF", USER_STACK_A, "Lab5A");
    load_user_program_from_disk("LAB5B   ELF", USER_STACK_B, "Lab5B");
    console_write_line("[lab5] starting synchronized scheduler");
    current_task = 0;
    enter_user_mode(tasks[0].mepc, tasks[0].frame.sp);
}

static void run_lab6_fat32(void) {
    int selected_program;

    console_write_line("[lab6] FAT32 loader demo");
    console_write("[lab6] disk image bytes=");
    console_write_hex((u32)(_binary_build_disk_img_end - _binary_build_disk_img_start));
    console_write("\n");
    selected_program = select_disk_program_from_console();

    switch (selected_program) {
    case 1:
        load_user_program_from_disk("LAB2    ELF", USER_STACK_SINGLE, "Lab2");
        break;
    case 2:
        load_user_program_from_disk("LAB3    ELF", USER_STACK_SINGLE, "Lab3");
        break;
    case 3:
        load_user_program_from_disk("LAB4A   ELF", USER_STACK_A, "Lab4A");
        load_user_program_from_disk("LAB4B   ELF", USER_STACK_B, "Lab4B");
        current_task = 0;
        enter_user_mode(tasks[0].mepc, tasks[0].frame.sp);
        return;
    case 4:
        load_user_program_from_disk("LAB5A   ELF", USER_STACK_A, "Lab5A");
        load_user_program_from_disk("LAB5B   ELF", USER_STACK_B, "Lab5B");
        current_task = 0;
        enter_user_mode(tasks[0].mepc, tasks[0].frame.sp);
        return;
    default:
        console_write_line("[lab6] invalid disk selection");
        return;
    }

    enter_user_mode(tasks[0].mepc, tasks[0].frame.sp);
}

void kmain(void) {
    serial_init();
    console_clear();
    console_write_line("SysToy RISC-V kernel start");

    memory_init(HEAP_START, HEAP_SIZE);
    trap_init();
    reset_runtime_state();
    selected_lab = select_lab_from_console();

    switch (selected_lab) {
    case 1:
        run_lab1_environment();
        break;
    case 2:
        run_lab2_boot_and_syscall();
        break;
    case 3:
        run_lab3_memory_and_elf();
        break;
    case 4:
        run_lab4_scheduler();
        break;
    case 5:
        run_lab5_synchronization();
        break;
    case 6:
        run_lab6_fat32();
        break;
    default:
        console_write_line("[lab] invalid selection");
        break;
    }

    for (;;) {
        __asm__ volatile ("wfi");
    }
}
