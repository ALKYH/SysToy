from __future__ import annotations

from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
NOTEBOOK_TITLE = "SysToy Unified Six-Lab Tutorial"
DEFAULT_NOTEBOOK_PATH = REPO_ROOT / "output" / "jupyter-notebook" / "systoy-unified-six-lab-tutorial.ipynb"


LABS = [
    {
        "id": "lab1",
        "number": 1,
        "title": "环境搭建",
        "objective": "在 Windows 10 + WSL2 下完成 SysToy 当前基线所需工具链检查、构建与运行验证。",
        "requirements": [
            "理解课程标准环境要求中的 WSL2、NASM、GCC、Make、QEMU。",
            "说明当前 SysToy 仓库真实构建主线基于 RISC-V 交叉工具链。",
            "能够完成环境自检、构建与基线运行。",
        ],
        "repo_mapping": [
            "课程文档保留通用环境要求。",
            "仓库真实运行路径以 `make` -> `Makefile.wsl` -> `qemu-system-riscv32` 为准。",
        ],
        "evidence_files": [
            "README.md",
            "docs/environment-requirements.md",
            "docs/environment-self-checklist.md",
            "docs/baseline-run-acceptance.md",
            "Makefile",
            "Makefile.wsl",
        ],
        "commands": [
            "wsl --status",
            "make",
            "make run",
        ],
        "cli_actions": [
            {
                "key": "1",
                "name": "查看环境要求",
                "type": "show_file",
                "path": "docs/environment-requirements.md",
            },
            {
                "key": "2",
                "name": "查看环境自检清单",
                "type": "show_file",
                "path": "docs/environment-self-checklist.md",
            },
            {
                "key": "3",
                "name": "执行基线构建",
                "type": "run",
                "command": "make",
            },
        ],
    },
    {
        "id": "lab2",
        "number": 2,
        "title": "Bootloader / Kernel / System Call",
        "objective": "理解从课程要求中的 Bootloader(MBR) 概念，过渡到当前 SysToy 的 RISC-V 启动链、trap 与 syscall 演示。",
        "requirements": [
            "说明课程要求中的 MBR / Bootloader 目标。",
            "展示当前仓库中的 RISC-V 内核入口、trap 和 syscall 分发。",
            "演示用户态程序通过系统调用输出信息。",
        ],
        "repo_mapping": [
            "当前仓库没有继续走 x86 MBR 直启链，而是切换到 `QEMU RISC-V32 virt`。",
            "教学上将 Bootloader 需求映射为“最小启动链 + 进入内核 + trap/syscall 调用链”。",
        ],
        "evidence_files": [
            "docs/phase-riscv-refactor-notes.md",
            "kernel/kernel_entry_riscv.S",
            "kernel/trap.c",
            "kernel/trap_riscv.S",
            "kernel/kernel.c",
            "user/user_task_a_riscv.S",
        ],
        "commands": [
            "python tools/lab_demo.py --lab 2 --action 2",
            "wsl bash -lc \"cd /mnt/e/Projects/SysToy && timeout 3s qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf\"",
        ],
        "cli_actions": [
            {
                "key": "1",
                "name": "查看 RISC-V 重构说明",
                "type": "show_file",
                "path": "docs/phase-riscv-refactor-notes.md",
            },
            {
                "key": "2",
                "name": "查看 trap / syscall 核心代码",
                "type": "show_snippet",
                "path": "kernel/trap.c",
                "start": 1,
                "end": 999,
            },
            {
                "key": "3",
                "name": "运行 QEMU 最小链路演示",
                "type": "run",
                "command": "wsl bash -lc \"cd /mnt/e/Projects/SysToy && timeout 3s qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf\"",
            },
        ],
    },
    {
        "id": "lab3",
        "number": 3,
        "title": "内存管理与 ELF 装载",
        "objective": "展示不带 Paging 的最小内存管理、ELF 装载与用户程序运行机制。",
        "requirements": [
            "理解 Variable Partitioning 风格的最小分配思想。",
            "展示内核将 ELF 装载到内存并定位入口地址。",
            "说明当前阶段尚未引入分页机制。",
        ],
        "repo_mapping": [
            "`memory.c` 提供线性 heap 分配。",
            "`elf_loader.c` 负责识别 ELF、分配镜像内存并拷贝 PT_LOAD 段。",
        ],
        "evidence_files": [
            "kernel/memory.c",
            "kernel/elf_loader.c",
            "docs/phase-3-implementation-notes.md",
            "tests/elf_loader_test.py",
        ],
        "commands": [
            "wsl bash -lc \"cd /mnt/e/Projects/SysToy && make -f Makefile.wsl all\"",
            "wsl bash -lc \"cd /mnt/e/Projects/SysToy && python3 -m pytest -q tests/elf_loader_test.py\"",
        ],
        "cli_actions": [
            {
                "key": "1",
                "name": "查看内存管理代码",
                "type": "show_file",
                "path": "kernel/memory.c",
            },
            {
                "key": "2",
                "name": "查看 ELF 装载代码",
                "type": "show_file",
                "path": "kernel/elf_loader.c",
            },
            {
                "key": "3",
                "name": "运行 ELF 相关测试",
                "type": "run",
                "command": "wsl bash -lc \"cd /mnt/e/Projects/SysToy && python3 -m pytest -q tests/elf_loader_test.py\"",
            },
        ],
    },
    {
        "id": "lab4",
        "number": 4,
        "title": "调度算法",
        "objective": "展示基于时钟中断的双任务调度与上下文切换。",
        "requirements": [
            "至少有两个用户程序。",
            "通过时钟中断触发调度。",
            "说明当前采用的基础轮转式切换思路。",
        ],
        "repo_mapping": [
            "`trap.c` 处理中断入口与 timer 中断。",
            "`kernel.c` 中 `scheduler_handle_timer` 保存/恢复 trap frame 并切换任务。",
        ],
        "evidence_files": [
            "kernel/trap.c",
            "kernel/kernel.c",
            "user/user_task_a_riscv.S",
            "user/user_task_b_riscv.S",
        ],
        "commands": [
            "python tools/lab_demo.py --lab 4 --action 2",
            "wsl bash -lc \"cd /mnt/e/Projects/SysToy && timeout 3s qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf\"",
        ],
        "cli_actions": [
            {
                "key": "1",
                "name": "查看两个用户程序",
                "type": "show_file",
                "path": "user/user_task_a_riscv.S",
            },
            {
                "key": "2",
                "name": "查看调度核心实现",
                "type": "show_snippet",
                "path": "kernel/kernel.c",
                "start": 175,
                "end": 223,
            },
            {
                "key": "3",
                "name": "运行调度演示",
                "type": "run",
                "command": "wsl bash -lc \"cd /mnt/e/Projects/SysToy && timeout 3s qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf\"",
            },
        ],
    },
    {
        "id": "lab5",
        "number": 5,
        "title": "同步与共享内存",
        "objective": "展示共享变量、P/V 风格信号量以及银行账户示例中的竞态与同步语义。",
        "requirements": [
            "提供共享内存读写能力。",
            "提供 P/V 风格同步机制。",
            "构造两个用户程序访问共享资源。",
        ],
        "repo_mapping": [
            "`kernel.c` 中的 `shared_words` 与 `semaphores` 是本阶段核心状态。",
            "两个用户程序都执行 unsafe 和 safe 两套更新路径。",
        ],
        "evidence_files": [
            "kernel/kernel.c",
            "kernel/kernel.h",
            "user/user_task_a_riscv.S",
            "user/user_task_b_riscv.S",
        ],
        "commands": [
            "python tools/lab_demo.py --lab 5 --action 2",
            "wsl bash -lc \"cd /mnt/e/Projects/SysToy && timeout 3s qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf\"",
        ],
        "cli_actions": [
            {
                "key": "1",
                "name": "查看共享内存与信号量定义",
                "type": "show_snippet",
                "path": "kernel/kernel.c",
                "start": 1,
                "end": 120,
            },
            {
                "key": "2",
                "name": "查看用户态加锁 / 不加锁示例",
                "type": "show_file",
                "path": "user/user_task_b_riscv.S",
            },
            {
                "key": "3",
                "name": "运行同步演示",
                "type": "run",
                "command": "wsl bash -lc \"cd /mnt/e/Projects/SysToy && timeout 3s qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf\"",
            },
        ],
    },
    {
        "id": "lab6",
        "number": 6,
        "title": "FAT32 文件系统",
        "objective": "展示 FAT32 虚拟盘构建、布局查看以及内核从 FAT32 读取 ELF 并运行的闭环。",
        "requirements": [
            "创建虚拟盘并格式化为 FAT32。",
            "展示 boot sector、FAT 区和目录项等信息。",
            "将 ELF 保存到虚拟盘并由内核装载运行。",
        ],
        "repo_mapping": [
            "`tools/make_fat32_image.py` 负责镜像构建。",
            "`tools/show_fat32_layout.py` 负责解析镜像布局。",
            "`kernel.c` 中 `load_user_program_from_disk` 负责 FAT32 -> ELF -> 任务注册。",
        ],
        "evidence_files": [
            "docs/phase-6-fat32-notes.md",
            "tools/make_fat32_image.py",
            "tools/show_fat32_layout.py",
            "kernel/kernel.c",
            "Makefile.wsl",
        ],
        "commands": [
            "python tools/make_fat32_image.py build/demo-fat32.img",
            "python tools/show_fat32_layout.py build/demo-fat32.img",
        ],
        "cli_actions": [
            {
                "key": "1",
                "name": "查看 FAT32 阶段说明",
                "type": "show_file",
                "path": "docs/phase-6-fat32-notes.md",
            },
            {
                "key": "2",
                "name": "创建并解析 FAT32 镜像",
                "type": "run_sequence",
                "commands": [
                    "python tools/make_fat32_image.py build/demo-fat32.img",
                    "python tools/show_fat32_layout.py build/demo-fat32.img",
                ],
            },
            {
                "key": "3",
                "name": "查看磁盘装载核心代码",
                "type": "show_snippet",
                "path": "kernel/kernel.c",
                "start": 225,
                "end": 273,
            },
        ],
    },
]
