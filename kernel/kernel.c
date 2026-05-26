#include "kernel.h"

extern u8 _binary_build_user_a_elf_start[];
extern u8 _binary_build_user_a_elf_end[];
extern u8 _binary_build_user_b_elf_start[];
extern u8 _binary_build_user_b_elf_end[];
extern u8 _binary_build_disk_img_start[];
extern u8 _binary_build_disk_img_end[];

static const u32 HEAP_START = 0x80400000;
static const u32 HEAP_SIZE = 0x00100000;
static const u32 USER_STACK_A = 0x80600000;
static const u32 USER_STACK_B = 0x80601000;
static const u32 MSTATUS_MPIE = (1u << 7);
static const u32 SHARED_SLOT_ACCOUNT_UNSAFE = 0;
static const u32 SHARED_SLOT_ACCOUNT_SAFE = 1;
static const u32 SHARED_SLOT_SEM_ID = 0;

struct task {
    struct trap_frame frame;
    u32 mepc;
    u32 image_base;
};

struct semaphore {
    u32 value;
};

static struct task tasks[2];
static int task_count = 0;
static int current_task = 0;
static u32 shared_words[4];
static struct semaphore semaphores[2];

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

static void add_task(u32 entry, u32 user_stack, u32 image_base) {
    struct task* task = &tasks[task_count++];
    memory_zero(&task->frame, sizeof(task->frame));
    task->frame.sp = user_stack;
    task->mepc = entry;
    task->image_base = image_base;
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
        if (arg2 == 0 && arg0 < HEAP_START) {
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

    saved_user_sp = tasks[current_task].frame.sp;
    memory_copy_block(&tasks[current_task].frame, frame, sizeof(*frame));
    tasks[current_task].frame.sp = saved_user_sp;
    tasks[current_task].mepc = current_mepc;

    next_task = (current_task + 1) % task_count;
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

static void load_user_program_from_disk(
    const char* file_name_11,
    u32 user_stack,
    const char* label
) {
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

    add_task(program.entry_point, user_stack, (u32)program.image);
}

void kmain(void) {
    serial_init();
    console_clear();
    console_write_line("SysToy RISC-V kernel start");

    memory_init(HEAP_START, HEAP_SIZE);
    trap_init();
    shared_words[SHARED_SLOT_ACCOUNT_UNSAFE] = 100;
    shared_words[SHARED_SLOT_ACCOUNT_SAFE] = 100;
    semaphores[SHARED_SLOT_SEM_ID].value = 1;
    console_write("[sync] shared unsafe=");
    console_write_hex(shared_words[SHARED_SLOT_ACCOUNT_UNSAFE]);
    console_write(" safe=");
    console_write_hex(shared_words[SHARED_SLOT_ACCOUNT_SAFE]);
    console_write(" sem=");
    console_write_hex(semaphores[SHARED_SLOT_SEM_ID].value);
    console_write("\n");

    load_user_program_from_disk("TASKA   ELF", USER_STACK_A, "TaskA");
    load_user_program_from_disk("TASKB   ELF", USER_STACK_B, "TaskB");

    console_write_line("Starting timer-driven scheduler...");
    current_task = 0;
    enter_user_mode(tasks[0].mepc, tasks[0].frame.sp);

    for (;;) {
        __asm__ volatile ("wfi");
    }
}
