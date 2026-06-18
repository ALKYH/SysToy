#include "../kernel.h"

extern u8 _binary_build_disk_img_start[];
extern u8 _binary_build_disk_img_end[];

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

static struct task tasks[2];
static int task_count = 0;
static int current_task = 0;
static int demo_done[2];
static int demo_reported = 0;
static int selected_lab = 0;
static int menu_return_requested = 0;
static int scheduler_mode = 1;
static u32 shared_words[4];
static struct semaphore semaphores[2];

static void report_demo_summary(void);
static void reset_runtime_state(void);
static void request_menu_return(void);
static void add_task(u32 entry, u32 user_stack, u32 image_base, const char* label);
static void console_write_u32(u32 value);
static void print_lab_menu(void);
static int select_lab_from_console(void);
int select_scheduler_mode_from_console(void);
static void memory_zero(void* dst, u32 size);
static void memory_copy_block(void* dst, const void* src, u32 size);
static u32 semaphore_wait(u32 sem_id, u32 current_mepc);
static u32 semaphore_signal(u32 sem_id, u32 current_mepc);
static int semaphore_try_wait_atomic(u32 sem_id);
static void semaphore_signal_atomic(u32 sem_id);
static inline void csr_write_mepc(u32 value);
static inline u32 csr_read_mstatus(void);
static inline void csr_write_mstatus(u32 value);
static inline void csr_write_pmpaddr0(u32 value);
static inline void csr_write_pmpcfg0(u32 value);

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
    memory_init(KERNEL_HEAP_START, KERNEL_HEAP_SIZE);
    task_count = 0;
    current_task = 0;
    demo_reported = 0;
    selected_lab = 0;
    menu_return_requested = 0;
    scheduler_mode = 1;
    demo_done[0] = 0;
    demo_done[1] = 0;
    shared_words[SHARED_SLOT_ACCOUNT_UNSAFE] = 100;
    shared_words[SHARED_SLOT_ACCOUNT_SAFE] = 100;
    semaphores[SHARED_SLOT_SEM_ID].value = 1;
}

static void request_menu_return(void) {
    menu_return_requested = 1;
}

int consume_menu_return_request(void) {
    int requested = menu_return_requested;
    menu_return_requested = 0;
    return requested;
}

int scheduler_mode_is_trace(void) {
    return scheduler_mode == 2;
}

static int semaphore_try_wait_atomic(u32 sem_id) {
    volatile u32* value_ptr = &semaphores[sem_id].value;
    u32 loaded;
    u32 updated;
    u32 store_result;

    __asm__ volatile ("fence rw, rw" : : : "memory");
    do {
        __asm__ volatile ("lr.w %0, (%1)" : "=r"(loaded) : "r"(value_ptr) : "memory");
        if (loaded == 0) {
            __asm__ volatile ("fence rw, rw" : : : "memory");
            return 0;
        }

        updated = loaded - 1;
        __asm__ volatile (
            "sc.w %0, %1, (%2)"
            : "=r"(store_result)
            : "r"(updated), "r"(value_ptr)
            : "memory"
        );
    } while (store_result != 0);
    __asm__ volatile ("fence rw, rw" : : : "memory");
    return 1;
}

static void semaphore_signal_atomic(u32 sem_id) {
    volatile u32* value_ptr = &semaphores[sem_id].value;
    u32 increment = 1;
    u32 previous;

    __asm__ volatile ("fence rw, rw" : : : "memory");
    __asm__ volatile (
        "amoadd.w %0, %1, (%2)"
        : "=r"(previous)
        : "r"(increment), "r"(value_ptr)
        : "memory"
    );
    (void)previous;
    __asm__ volatile ("fence rw, rw" : : : "memory");
}

static u32 semaphore_wait(u32 sem_id, u32 current_mepc) {
    if (sem_id >= 2) {
        console_write("[sync] bad sem wait id=");
        console_write_hex(sem_id);
        console_write("\n");
        return current_mepc + 4;
    }

    if (!semaphore_try_wait_atomic(sem_id)) {
        return current_mepc;
    }

    return current_mepc + 4;
}

static u32 semaphore_signal(u32 sem_id, u32 current_mepc) {
    if (sem_id >= 2) {
        console_write("[sync] bad sem signal id=");
        console_write_hex(sem_id);
        console_write("\n");
        return current_mepc + 4;
    }

    semaphore_signal_atomic(sem_id);
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
        if (task_count > 0 && arg2 == 0 && arg0 < KERNEL_HEAP_START) {
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
            request_menu_return();
            return current_mepc + 4;
        }

        if (arg0 < 2) {
            demo_done[arg0] = 1;
            console_write("[demo] task done: ");
            console_write(arg0 == 0 ? "TaskA" : "TaskB");
            console_write("\n");

            if (demo_done[0] && demo_done[1]) {
                report_demo_summary();
                request_menu_return();
                return current_mepc + 4;
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
    mstatus |= (1u << 7);
    csr_write_mstatus(mstatus);

    __asm__ volatile (
        "mv sp, %0\n\t"
        "mret\n\t"
        :
        : "r"(user_stack)
        : "memory"
    );
}

void boot_lab_user_program(const struct embedded_program* program, u32 user_stack) {
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

void load_user_program_from_disk(const char* file_name_11, u32 user_stack, const char* label) {
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

void start_task_execution(void) {
    if (task_count == 0) {
        console_write_line("[runtime] no task loaded");
        return;
    }

    current_task = 0;
    enter_user_mode(tasks[0].mepc, tasks[0].frame.sp);
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

int select_scheduler_mode_from_console(void) {
    char selected;

    console_write_line("[sched] choose mode");
    console_write_line("1) timer-driven round robin");
    console_write_line("2) scheduler trace inspection");
    console_write_line("Select scheduler mode [1-2]: ");

    for (;;) {
        selected = console_read_char();
        if (selected == '1' || selected == '2') {
            scheduler_mode = (selected == '2') ? 2 : 1;
            console_write("[sched] mode ");
            console_write(selected == '2' ? "scheduler trace" : "timer-driven");
            console_write("\n");
            return scheduler_mode;
        }
    }
}

void lab_selection_loop_entry(void) {
    for (;;) {
        reset_runtime_state();
        selected_lab = select_lab_from_console();

        switch (selected_lab) {
        case 1:
            run_lab1_environment();
            console_write_line("[menu] lab complete, returning to lab selector");
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
    }
}
