# SysToy Phase 2 实现说明

## 1. 本次实现范围

本次代码实现覆盖 Phase 2 的最小闭环：

1. MBR 启动扇区
2. 将内核加载到内存
3. 进入保护模式
4. 进入 32 位内核
5. 内核输出调试信息
6. 建立 GDT、IDT、TSS
7. 通过 `int 0x80` 演示最小 system call
8. 从内核切换到 ring3 用户程序

## 2. 当前代码结构

1. `boot/boot.asm`
2. `kernel/kernel_entry.asm`
3. `kernel/interrupts.asm`
4. `kernel/kernel.c`
5. `kernel/console.c`
6. `kernel/gdt_idt.c`
7. `kernel/linker.ld`
8. `user/user.asm`
9. `Makefile.wsl`
10. `Makefile`

## 3. 当前实现边界

1. 当前 bootloader 采用固定扇区读取方式。
2. 当前内核仍是 Naive Kernel，不含调度和分页。
3. 当前用户程序是最小 ring3 演示程序。
4. 当前 syscall 仅演示打印字符串。
5. 当前 CPU switching 体现为从 ring0 切到 ring3 的最小切换能力。

## 4. 运行方式

在项目根目录执行：

```powershell
make
make run
```

或在 WSL2 中执行：

```bash
make -f Makefile.wsl all
make -f Makefile.wsl run
```
