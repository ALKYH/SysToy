# SysToy RISC-V 重构说明

## 1. 目标

将 SysToy 从原先的 x86/MBR/BIOS 主线重构到 `QEMU RISC-V32 virt`，并恢复最小教学内核链路。

## 2. 当前已完成内容

### 2.1 启动与平台

1. 已删除 x86 的 `boot.asm`
2. 已删除 x86 的 GDT/IDT/TSS 依赖
3. 已新增 RISC-V 内核入口
4. 已将平台切换为 `qemu-system-riscv32 -machine virt`

### 2.2 控制台输出

1. 已移除 VGA 文本显存输出
2. 已改为 `virt` 平台 UART MMIO 输出

### 2.3 trap 与 syscall

1. 已实现 RISC-V trap 入口
2. 已实现 `ecall` -> trap -> syscall 分发链路
3. 已验证最小 syscall 输出

### 2.4 用户程序与 ELF

1. 已将用户程序改为 RISC-V ELF
2. 已将 ELF 装载器适配为 RISC-V machine type
3. 已验证最小 ELF 装载链

## 3. 当前运行方式

### 构建

```bash
wsl bash -lc "cd /mnt/e/Projects/SysToy && make -f Makefile.wsl all"
```

### 运行

```bash
wsl bash -lc "cd /mnt/e/Projects/SysToy && timeout 5s qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf"
```

## 4. 当前输出

```text
SysToy RISC-V kernel start
User ELF size: 0x00001234
[elf] loaded image size: 0x0000001A
[elf] entry point: 0x80400000
Switching to umode user program...
[syscall] hello um
```

## 5. 当前边界

1. 当前 syscall 演示样例为最小形式
2. 当前 ELF 装载器只覆盖最小 `PT_LOAD`
3. 当前还未进入时钟中断、调度和同步机制

## 6. 建议下一步

1. 继续补齐 RISC-V Phase 3 的正式字符串传参版本
2. 补齐测试环境与 RISC-V 自动化验证
3. 开始 Phase 4：时钟中断与调度
