from pathlib import Path
import struct


def read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def test_user_elf_header_is_valid():
    data = Path("build/lab2_user.elf").read_bytes()

    assert data[:4] == b"\x7fELF"
    assert data[4] == 1
    assert data[5] == 1
    assert read_u16(data, 16) == 2
    assert read_u16(data, 18) == 243


def test_user_elf_has_program_header():
    data = Path("build/lab2_user.elf").read_bytes()

    phoff = read_u32(data, 28)
    phentsize = read_u16(data, 42)
    phnum = read_u16(data, 44)

    assert phoff > 0
    assert phentsize == 32
    assert phnum >= 1


def test_user_elf_entry_inside_load_segment():
    data = Path("build/lab2_user.elf").read_bytes()

    entry = read_u32(data, 24)
    phoff = read_u32(data, 28)
    phentsize = read_u16(data, 42)
    phnum = read_u16(data, 44)

    found = False
    for idx in range(phnum):
        base = phoff + idx * phentsize
        p_type = read_u32(data, base)
        p_offset = read_u32(data, base + 4)
        p_vaddr = read_u32(data, base + 8)
        p_filesz = read_u32(data, base + 16)

        if p_type != 1:
            continue

        assert p_offset + p_filesz <= len(data)
        if p_vaddr <= entry < p_vaddr + p_filesz:
            found = True

    assert found



def test_trap_entry_restores_original_sp_before_mret():
    text = Path("kernel/trap_riscv.S").read_text(encoding="utf-8")

    assert "csrrw t0, mscratch, t0" in text
    assert "sw sp, 0(t0)" not in text
    assert "mv t0, sp" not in text
    assert "sw sp, -4(t0)" in text
    assert "sw t1, -8(t0)" in text
    assert "csrr t1, mscratch" in text
    assert "sw t1, 12(sp)" in text
    assert "lw t1, -4(t0)" in text
    assert "sw t1, 120(sp)" in text
    restore_tail = text.split("call trap_dispatch", 1)[1]
    assert "lw t1, 120(sp)" in restore_tail
    assert "csrw mscratch, t1" in restore_tail
    assert "csrrw sp, mscratch, sp" in restore_tail
    assert "mret" in restore_tail


def test_trap_dispatch_uses_riscv_syscall_abi():
    text = Path("kernel/trap.c").read_text(encoding="utf-8")

    assert "if (mcause == MCAUSE_ECALL_UMODE)" in text
    assert "csr_write_mepc(syscall_dispatcher(frame, csr_read_mepc()));" in text


def test_user_tasks_use_string_pointer_syscall_payload():
    for path_str, fragments in [
        ("user/lab5_task_a_riscv.S", ["unsafe +5", "safe +5"]),
        ("user/lab5_task_b_riscv.S", ["unsafe +7", "safe +7"]),
    ]:
        text = Path(path_str).read_text(encoding="utf-8")

        assert "la a0, unsafe_message" in text
        assert "li a2, 0" in text
        for fragment in fragments:
            assert f'.asciz "{fragment}"' in text


def test_kernel_defines_shared_memory_and_semaphore_primitives():
    text = Path("kernel/runtime/runtime_state.c").read_text(encoding="utf-8")

    assert "struct semaphore" in text
    assert "shared_words" in text
    assert "semaphores" in text
    assert "case SYSCALL_SEM_WAIT" in text
    assert "case SYSCALL_SEM_SIGNAL" in text
    assert "case SYSCALL_SHARED_READ" in text
    assert "case SYSCALL_SHARED_WRITE" in text


def test_semaphore_updates_use_atomic_kernel_helpers():
    text = Path("kernel/runtime/runtime_state.c").read_text(encoding="utf-8")

    assert "static int semaphore_try_wait_atomic" in text
    assert "static void semaphore_signal_atomic" in text
    assert "lr.w" in text
    assert "sc.w" in text
    assert "fence rw, rw" in text
    assert "semaphores[sem_id].value -= 1;" not in text
    assert "semaphores[sem_id].value += 1;" not in text


def test_user_tasks_exercise_locked_and_unlocked_bank_slots():
    text_a = Path("user/lab5_task_a_riscv.S").read_text(encoding="utf-8")
    text_b = Path("user/lab5_task_b_riscv.S").read_text(encoding="utf-8")

    for text in (text_a, text_b):
        assert "li a7, 4" in text
        assert "li a7, 5" in text
        assert "li a7, 2" in text
        assert "li a7, 3" in text
        assert "li a0, 0" in text
        assert "li a0, 1" in text


def test_phase6_docs_and_tools_exist():
    assert Path("docs/phase-6-fat32-notes.md").exists()
    assert Path("tools/make_fat32_image.py").exists()
    assert Path("tools/show_fat32_layout.py").exists()


def test_fat32_notes_cover_work_packages_before_6_4():
    text = Path("docs/phase-6-fat32-notes.md").read_text(encoding="utf-8")

    assert "6.1" in text
    assert "6.2" in text
    assert "6.3" in text
    assert "6.4" in text
    assert "6.5" in text
    assert "6.6" in text
    assert "boot sector" in text.lower()
    assert "fat 区" in text.lower()
    assert "目录项" in text


def test_fat32_tool_can_build_and_parse_image():
    import json
    import subprocess
    import sys

    image_path = Path("build/test-fat32.img")
    image_path.parent.mkdir(parents=True, exist_ok=True)

    subprocess.run(
        ["wsl", "bash", "-lc", "cd /mnt/e/Projects/SysToy && make -f Makefile.wsl clean all"],
        check=True,
        capture_output=True,
        text=True,
    )

    subprocess.run(
        [sys.executable, "tools/make_fat32_image.py", str(image_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    result = subprocess.run(
        [sys.executable, "tools/show_fat32_layout.py", str(image_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    data = json.loads(result.stdout)

    assert data["bytes_per_sector"] == 512
    assert data["sectors_per_cluster"] == 1
    assert data["fat_count"] == 2
    assert data["root_cluster"] == 2
    assert "LAB2.ELF" in data["entries"]
    assert "LAB3.ELF" in data["entries"]
    assert "LAB4A.ELF" in data["entries"]
    assert "LAB4B.ELF" in data["entries"]
    assert "LAB5A.ELF" in data["entries"]
    assert "LAB5B.ELF" in data["entries"]
    assert "PRINT.ELF" in data["entries"]


def test_kernel_has_fat32_loader_entry_points():
    text = Path("kernel/runtime/runtime_state.c").read_text(encoding="utf-8")
    header = Path("kernel/kernel.h").read_text(encoding="utf-8")
    loader = Path("kernel/elf_loader.c").read_text(encoding="utf-8")
    makefile = Path("Makefile.wsl").read_text(encoding="utf-8")

    assert "load_user_program_from_disk" in text
    assert "fat32_load_root_file" in text or "fat32_load_root_file" in header
    assert "fat32_list_root_files" in loader
    assert "disk.img" in makefile


def test_kernel_has_serial_lab_selector_and_phase_runners():
    kernel_text = Path("kernel/kernel.c").read_text(encoding="utf-8")
    runtime_text = Path("kernel/runtime/runtime_state.c").read_text(encoding="utf-8")
    lab2_text = Path("kernel/labs/lab2/lab2.c").read_text(encoding="utf-8")
    lab3_text = Path("kernel/labs/lab3/lab3.c").read_text(encoding="utf-8")
    lab4_text = Path("kernel/labs/lab4/lab4.c").read_text(encoding="utf-8")
    lab5_text = Path("kernel/labs/lab5/lab5.c").read_text(encoding="utf-8")
    lab6_text = Path("kernel/labs/lab6/lab6.c").read_text(encoding="utf-8")
    console_text = Path("kernel/console.c").read_text(encoding="utf-8")

    assert "lab_selection_loop_entry" in kernel_text
    assert "select_lab_from_console" in runtime_text
    assert "run_lab1_environment" in runtime_text
    assert "run_lab2_boot_and_syscall" in runtime_text
    assert "run_lab3_memory_and_elf" in runtime_text
    assert "run_lab4_scheduler" in runtime_text
    assert "run_lab5_synchronization" in runtime_text
    assert "run_lab6_fat32" in runtime_text
    assert "selected_lab = select_lab_from_console()" in runtime_text
    assert "PRINT   ELF" in lab4_text
    assert "Select disk program [1-5]" in lab6_text
    assert "[sched] " in runtime_text
    assert "boot_lab_user_program" in lab2_text
    assert "memory_remaining()" in lab3_text
    assert "start_task_execution()" in lab4_text
    assert "start_task_execution()" in lab5_text
    assert "console_read_char" in console_text


def test_lab_selection_flow_is_wired_into_kernel_and_user_tasks():
    runtime_text = Path("kernel/runtime/runtime_state.c").read_text(encoding="utf-8")
    lab4_kernel_text = Path("kernel/labs/lab4/lab4.c").read_text(encoding="utf-8")
    lab6_kernel_text = Path("kernel/labs/lab6/lab6.c").read_text(encoding="utf-8")
    lab2_text = Path("user/lab2_user_riscv.S").read_text(encoding="utf-8")
    lab3_text = Path("user/lab3_user_riscv.S").read_text(encoding="utf-8")
    task_a_text = Path("user/lab4_task_a_riscv.S").read_text(encoding="utf-8")
    print_user_text = Path("user/print_user.c").read_text(encoding="utf-8")
    sync_a_text = Path("user/lab5_task_a_riscv.S").read_text(encoding="utf-8")
    sync_b_text = Path("user/lab5_task_b_riscv.S").read_text(encoding="utf-8")

    assert "SysToy unified lab selector" in runtime_text
    assert "run_lab2_boot_and_syscall" in runtime_text
    assert "run_lab6_fat32" in runtime_text
    assert "LAB4A   ELF" in lab4_kernel_text
    assert "PRINT   ELF" in lab4_kernel_text
    assert "LAB2    ELF" in lab6_kernel_text
    assert "LAB3    ELF" in lab6_kernel_text
    assert "LAB4A   ELF" in lab6_kernel_text
    assert "LAB5A   ELF" in lab6_kernel_text
    assert "PRINT   ELF" in lab6_kernel_text
    assert "shared_words[SHARED_SLOT_ACCOUNT_UNSAFE]" in runtime_text
    assert "shared_words[SHARED_SLOT_ACCOUNT_SAFE]" in runtime_text
    assert "Lab4 TaskA" in task_a_text
    assert "SYSCALL_DEMO_DONE" in runtime_text
    assert "task done" in runtime_text
    assert '.asciz "Lab2 user program entered U-mode"' in lab2_text
    assert '.asciz "Lab2 syscall path is alive"' in lab2_text
    assert "li a7, 6" in lab2_text
    assert '.asciz "Lab3 memory allocator is ready"' in lab3_text
    assert '.asciz "Lab3 ELF loader entered user mode"' in lab3_text

    assert '.asciz "Lab4 TaskA tick source"' in task_a_text
    assert '.asciz "Lab4 round robin A"' in task_a_text
    assert "li a7, 6" in task_a_text

    assert "print syscall from C user program" in print_user_text
    assert "second line verifies repeated ecall" in print_user_text
    assert "SYSCALL_DEMO_DONE" in print_user_text

    assert '.asciz "unsafe +5"' in sync_a_text
    assert '.asciz "safe +5"' in sync_a_text
    assert '.asciz "unsafe +7"' in sync_b_text
    assert '.asciz "safe +7"' in sync_b_text


def test_print_syscall_user_program_is_packaged_for_lab6():
    makefile_text = Path("Makefile.wsl").read_text(encoding="utf-8")
    fat32_tool_text = Path("tools/make_fat32_image.py").read_text(encoding="utf-8")
    print_user_text = Path("user/print_user.c").read_text(encoding="utf-8")

    assert "build/print_user.elf" in fat32_tool_text
    assert '"PRINT   ELF"' in fat32_tool_text
    assert "$(BUILD_DIR)/print_user.elf" in makefile_text
    assert "$(BUILD_DIR)/print_user_program.o" in makefile_text
    assert "SYSCALL_PRINT" in print_user_text
    assert "SYSCALL_DEMO_DONE" in print_user_text
    assert "print syscall from C user program" in print_user_text
    assert "string_length" not in print_user_text
    assert "do_syscall_print(message_one, 33)" not in print_user_text
