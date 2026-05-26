from pathlib import Path
import struct


def read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def test_user_elf_header_is_valid():
    data = Path("build/user_a.elf").read_bytes()

    assert data[:4] == b"\x7fELF"
    assert data[4] == 1
    assert data[5] == 1
    assert read_u16(data, 16) == 2
    assert read_u16(data, 18) == 243


def test_user_elf_has_program_header():
    data = Path("build/user_a.elf").read_bytes()

    phoff = read_u32(data, 28)
    phentsize = read_u16(data, 42)
    phnum = read_u16(data, 44)

    assert phoff > 0
    assert phentsize == 32
    assert phnum >= 1


def test_user_elf_entry_inside_load_segment():
    data = Path("build/user_a.elf").read_bytes()

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
    for path_str, label in [
        ("user/user_task_a_riscv.S", "TaskA"),
        ("user/user_task_b_riscv.S", "TaskB"),
    ]:
        text = Path(path_str).read_text(encoding="utf-8")

        assert "la a0, unsafe_message" in text
        assert "li a2, 0" in text
        assert f'.asciz "{label} unsafe"' in text
        assert f'.asciz "{label} safe"' in text


def test_kernel_defines_shared_memory_and_semaphore_primitives():
    text = Path("kernel/kernel.c").read_text(encoding="utf-8")

    assert "struct semaphore" in text
    assert "shared_words" in text
    assert "semaphores" in text
    assert "case SYSCALL_SEM_WAIT" in text
    assert "case SYSCALL_SEM_SIGNAL" in text
    assert "case SYSCALL_SHARED_READ" in text
    assert "case SYSCALL_SHARED_WRITE" in text


def test_user_tasks_exercise_locked_and_unlocked_bank_slots():
    text_a = Path("user/user_task_a_riscv.S").read_text(encoding="utf-8")
    text_b = Path("user/user_task_b_riscv.S").read_text(encoding="utf-8")

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
    assert "TASKA.ELF" in data["entries"]
    assert "TASKB.ELF" in data["entries"]


def test_kernel_has_fat32_loader_entry_points():
    text = Path("kernel/kernel.c").read_text(encoding="utf-8")
    header = Path("kernel/kernel.h").read_text(encoding="utf-8")
    makefile = Path("Makefile.wsl").read_text(encoding="utf-8")

    assert "load_user_program_from_disk" in text
    assert "fat32_load_root_file" in text or "fat32_load_root_file" in header
    assert "disk.img" in makefile
