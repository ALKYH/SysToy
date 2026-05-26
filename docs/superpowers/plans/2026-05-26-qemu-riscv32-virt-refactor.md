# SysToy QEMU RISC-V32 Virt Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将当前 x86 版 SysToy 重构为基于 QEMU RISC-V32 virt 的最小内核，并恢复 Phase 2-3 等价能力。

**Architecture:** 先删除 x86 专属启动链和中断机制，建立 RISC-V 的内核入口、trap 与 UART 输出，再恢复 U-mode 切换和 `ecall`。最后将现有内存管理和 ELF 装载链适配到 RISC-V 用户程序。

**Tech Stack:** QEMU RISC-V virt, RISC-V GCC/binutils, RISC-V assembly, ELF32, UART MMIO, machine mode trap handling

---

### Task 1: 安装并验证 RISC-V 工具链

**Files:**
- Modify: `docs/environment-requirements.md`
- Modify: `docs/environment-self-checklist.md`
- Test: local toolchain commands

- [ ] **Step 1: 检查当前是否已有 RISC-V 工具链**

Run:

```bash
wsl bash -lc "command -v qemu-system-riscv32 || true; command -v riscv64-unknown-elf-gcc || true; command -v riscv64-linux-gnu-gcc || true"
```

Expected: 当前为空或缺失，证明需要安装。

- [ ] **Step 2: 安装 QEMU 和 RISC-V 交叉工具链**

Run:

```bash
wsl bash -lc "sudo apt-get update && sudo apt-get install -y qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf"
```

Expected: 安装成功，命令可用。

- [ ] **Step 3: 重新验证工具链**

Run:

```bash
wsl bash -lc "qemu-system-riscv32 --version && riscv64-unknown-elf-gcc --version && riscv64-unknown-elf-ld --version"
```

Expected: 三条命令均返回版本信息。

### Task 2: 移除 x86 启动链并建立 RISC-V 内核入口

**Files:**
- Delete: `boot/boot.asm`
- Delete: `kernel/kernel_entry.asm`
- Create: `kernel/kernel_entry_riscv.S`
- Modify: `kernel/linker.ld`
- Modify: `Makefile.wsl`

- [ ] **Step 1: 删除 x86 启动入口文件依赖**

Goal: Makefile 不再引用 `boot.asm` 和 `kernel_entry.asm`。

- [ ] **Step 2: 新建 RISC-V 汇编入口**

Goal: 设置栈、清 BSS、跳转到 `kmain`。

- [ ] **Step 3: 重写内核链接脚本**

Goal: 适配 QEMU RISC-V virt 的内核装载地址和 section 布局。

- [ ] **Step 4: 更新构建命令**

Goal: 生成可供 `qemu-system-riscv32 -machine virt -kernel build/kernel.elf` 直接启动的 ELF。

### Task 3: 建立 UART 控制台输出

**Files:**
- Modify: `kernel/console.c`
- Modify: `kernel/kernel.h`

- [ ] **Step 1: 删除 VGA 和 x86 端口 I/O 输出路径**

Goal: 清除 `0xB8000`、`inb/outb`、COM1 依赖。

- [ ] **Step 2: 接入 QEMU virt UART MMIO**

Goal: 提供字符输出。

- [ ] **Step 3: 保持现有 console 接口不变**

Goal: `console_write()`、`console_write_line()`、`console_write_hex()` 仍能被现有内核调用。

### Task 4: 建立 RISC-V trap 与 syscall 路径

**Files:**
- Delete: `kernel/gdt_idt.c`
- Delete: `kernel/interrupts.asm`
- Create: `kernel/trap_riscv.S`
- Create: `kernel/trap.c`
- Modify: `kernel/kernel.h`

- [ ] **Step 1: 新建 trap 入口汇编**

Goal: 保存寄存器、调用 C trap 分发函数、恢复寄存器并 `mret`。

- [ ] **Step 2: 新建 trap 分发 C 文件**

Goal: 识别 `ecall from U-mode`，转发给 `syscall_dispatcher()`。

- [ ] **Step 3: 更新 syscall 分发约定**

Goal: 使用 `a7` 作为 syscall 编号，`a0-a2` 作为参数。

### Task 5: 实现 U-mode 切换

**Files:**
- Modify: `kernel/kernel.c`
- Modify: `kernel/kernel.h`
- Implement in: `kernel/trap.c` or `kernel/kernel_entry_riscv.S`

- [ ] **Step 1: 删除 x86 的 `enter_user_mode()` 依赖**

Goal: 不再使用 `iret` 语义。

- [ ] **Step 2: 新建 RISC-V 用户态切换逻辑**

Goal: 设置 `mepc`、用户栈和 `mstatus` 后使用 `mret` 进入 U-mode。

- [ ] **Step 3: 先恢复最小 Phase 2 闭环**

Goal: 内核启动后进入用户态，用户态 `ecall` 回到内核输出字符串。

### Task 6: 将用户程序改为 RISC-V 用户态程序

**Files:**
- Delete: `user/user.asm`
- Delete: `user/user.S`
- Delete: `user/user.ld`
- Create: `user/user_riscv.S`
- Create: `user/user_riscv.ld`
- Modify: `Makefile.wsl`

- [ ] **Step 1: 新建 RISC-V 用户程序入口**

Goal: 使用 RISC-V 寄存器约定准备参数并执行 `ecall`。

- [ ] **Step 2: 新建 RISC-V 用户程序链接脚本**

Goal: 生成适合 ELF 装载的最小用户态 ELF。

- [ ] **Step 3: 更新 Makefile 生成 `build/user.elf`**

Goal: 用户程序构建路径改为 RISC-V 版。

### Task 7: 适配 ELF 装载链

**Files:**
- Modify: `kernel/elf_loader.c`
- Modify: `kernel/kernel.c`
- Modify: `kernel/memory.c`
- Modify: `tests/elf_loader_test.py`

- [ ] **Step 1: 适配 RISC-V ELF 校验逻辑**

Goal: 测试中识别 RISC-V machine type 和 program header。

- [ ] **Step 2: 保持最小非分页内存分配器**

Goal: 使用现有线性分配器为 ELF 装载提供内存。

- [ ] **Step 3: 恢复 Phase 3 闭环**

Goal: 内核打印 ELF 信息、装载映像并切换到用户态运行。

### Task 8: 文档与验证

**Files:**
- Create: `docs/phase-riscv-refactor-notes.md`
- Modify: `docs/phase-3-implementation-notes.md`

- [ ] **Step 1: 记录新的运行方式**

Run target:

```bash
wsl bash -lc "cd /mnt/e/Projects/SysToy && make -f Makefile.wsl all && timeout 5s qemu-system-riscv32 -machine virt -nographic -kernel build/kernel.elf"
```

- [ ] **Step 2: 验证 Phase 2 等价能力**

Expected output:

```text
kernel start
[syscall] ...
```

- [ ] **Step 3: 验证 Phase 3 等价能力**

Expected output:

```text
user elf size ...
[elf] loaded image size ...
[syscall] ...
```
