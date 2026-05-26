# SysToy QEMU RISC-V32 Virt 重构设计

## 1. 背景

当前 SysToy 已完成基于 x86 的 Phase 2 与 Phase 3 最小闭环，实现了：

1. Bootloader 启动内核
2. 进入 32 位内核
3. 最小 syscall 演示
4. ELF 用户程序装载与运行

但现有实现依赖 x86/PC 平台专属机制，包括：

1. BIOS/MBR 启动链
2. GDT/IDT/TSS
3. `int 0x80`
4. VGA 与 x86 串口端口 I/O

为支持后续在 RISC-V 平台上继续推进内核实验，需要将当前工程重构为基于 `QEMU RISC-V virt` 的版本，并恢复与当前 x86 版本等价的最小能力闭环。

## 2. 目标

本次重构目标限定为：

1. 将平台从 x86/BIOS/MBR 重构为 `QEMU RISC-V32 virt`
2. 保留“最小教学内核”主线
3. 恢复 Phase 2-3 的最小闭环能力：
   - 内核启动
   - 串口输出
   - 用户态切换
   - `ecall` 系统调用
   - ELF 用户程序装载并运行

## 3. 非目标

本次重构不包含以下内容：

1. 同时维护 x86 与 RISC-V 双架构共存
2. OpenSBI 集成后的完整 S-mode 体系
3. 时钟中断与调度算法
4. 文件系统迁移
5. 面向 ESP32 等 MCU 平台的硬件适配

## 4. 推荐方案

采用 `riscv32 + qemu-system-riscv32 + virt` 方案。

### 选择理由

1. 更接近当前教学型最小内核的复杂度。
2. 保持 32 位地址空间，便于与现有实现迁移。
3. 可以直接在 QEMU 上验证，不引入真实硬件依赖。
4. 后续如需迁移到 `riscv64`，可在此基础上继续演进。

## 5. 架构替换关系

### 5.1 启动链

当前 x86：

1. `boot/boot.asm`
2. BIOS 读盘
3. 进入保护模式
4. 跳转到内核

重构后 RISC-V：

1. 删除 `boot/boot.asm`
2. 由 `QEMU -kernel build/kernel.elf` 直接装载内核
3. 使用新的 RISC-V 汇编入口文件完成：
   - 设置栈
   - 初始化 `bss`
   - 设置 trap 向量
   - 跳转到 `kmain`

### 5.2 特权与陷入

当前 x86：

1. GDT
2. IDT
3. TSS
4. `int 0x80`
5. `iret`

重构后 RISC-V：

1. 不再使用 GDT/IDT/TSS
2. 使用 `mtvec` 设置 trap 入口
3. 使用 `mepc` 保存/恢复用户态返回地址
4. 使用 `mstatus` 控制特权级切换
5. 使用 `ecall` 作为 syscall 入口
6. 使用 `mret` 返回用户态

### 5.3 控制台输出

当前 x86：

1. VGA 文本显存
2. COM1 端口 I/O

重构后 RISC-V：

1. 删除 VGA 路径
2. 统一改为 `QEMU virt` 平台 UART MMIO 输出
3. 所有 `console_write*()` 保持接口不变，但底层输出改为内存映射 UART

### 5.4 用户程序

当前 x86：

1. 用户程序为 x86 ELF
2. syscall 通过 `int 0x80`

重构后 RISC-V：

1. 用户程序改为 RISC-V ELF
2. syscall 通过 `ecall`
3. 参数传递遵循 RISC-V ABI：
   - `a7`：syscall 编号
   - `a0-a2`：参数

### 5.5 ELF 装载器

保留现有 [elf_loader.c](e:\Projects\SysToy\kernel\elf_loader.c) 的总体思路，但需要做以下适配：

1. 用户程序 ELF 来源改为 RISC-V 目标文件
2. 入口地址与段地址按 RISC-V 构建结果验证
3. 保持当前最小 `PT_LOAD` 装载逻辑

## 6. 文件级改动范围

### 删除或废弃

1. [boot.asm](e:\Projects\SysToy\boot\boot.asm)
2. [gdt_idt.c](e:\Projects\SysToy\kernel\gdt_idt.c)
3. x86 专用 [interrupts.asm](e:\Projects\SysToy\kernel\interrupts.asm)
4. x86 专用 [kernel_entry.asm](e:\Projects\SysToy\kernel\kernel_entry.asm)
5. x86 用户程序 [user.asm](e:\Projects\SysToy\user\user.asm)

### 重写

1. [console.c](e:\Projects\SysToy\kernel\console.c)
2. [kernel.h](e:\Projects\SysToy\kernel\kernel.h)
3. [kernel.c](e:\Projects\SysToy\kernel\kernel.c)
4. [linker.ld](e:\Projects\SysToy\kernel\linker.ld)
5. [Makefile.wsl](e:\Projects\SysToy\Makefile.wsl)

### 新增

1. `kernel/kernel_entry_riscv.S`
2. `kernel/trap_riscv.S`
3. `kernel/trap.c`
4. `user/user_riscv.S`
5. `user/user_riscv.ld`
6. `docs/phase-riscv-refactor-notes.md`

## 7. 最小实现路径

### 阶段 A：恢复 Phase 2 最小链路

目标：

1. QEMU 启动内核
2. UART 输出 `kernel start`
3. 内核切换到 U-mode
4. 用户程序通过 `ecall` 调用内核
5. 内核打印 syscall 字符串

### 阶段 B：恢复 Phase 3 最小链路

目标：

1. 用户程序改为 RISC-V ELF
2. 内核解析 ELF
3. 内核分配内存并装载 ELF 段
4. 跳转到装载后的入口运行
5. 用户程序再次通过 `ecall` 输出字符串

## 8. 接口设计

### 8.1 控制台接口

保留：

1. `console_clear()`
2. `console_write()`
3. `console_write_line()`
4. `console_write_hex()`

实现变化：

1. 不再写 VGA
2. 改为 UART MMIO

### 8.2 用户态切换接口

保留概念：

1. `enter_user_mode(entry, user_stack)`

实现变化：

1. 不再依赖 `iret`
2. 改为设置 `mepc`
3. 配置 `mstatus`
4. 使用 `mret`

### 8.3 syscall 分发接口

保留：

1. `syscall_dispatcher(...)`

实现变化：

1. 参数来源改为 RISC-V 寄存器
2. trap handler 负责读取并传给 `syscall_dispatcher`

## 9. 风险与应对

### 风险 1：本机缺少 RISC-V 工具链

影响：

1. 无法完成交叉编译

应对：

1. 先安装 `qemu-system-riscv32`
2. 安装可用的 RISC-V 交叉编译器
3. 在 Makefile 中做显式命令检测

### 风险 2：U-mode 切换路径不稳定

影响：

1. 内核能启动，但无法成功进入用户态

应对：

1. 先做“内核启动 + trap 可用”
2. 再做最小 U-mode 跳转
3. 最后接入 syscall

### 风险 3：ELF 装载偏移和入口不匹配

影响：

1. 用户程序可装载但无法执行

应对：

1. 保留现有 ELF 测试思路
2. 增加对 RISC-V ELF header/program header 的验证

## 10. 验收标准

### Phase 2 等价验收

满足以下全部条件：

1. QEMU RISC-V virt 能启动内核
2. 串口输出内核启动信息
3. 能进入 U-mode
4. 用户程序 `ecall` 后，内核打印字符串

### Phase 3 等价验收

在上述基础上进一步满足：

1. 用户程序为 ELF 产物
2. 内核显示 ELF 大小或装载信息
3. 内核完成 ELF 装载后跳转执行
4. syscall 输出依然正常

## 11. 最终建议

本次重构不建议一次性完成所有 RISC-V 迁移，而应按下面顺序推进：

1. 先把 Phase 2 的最小 RISC-V 版本跑通
2. 再恢复 Phase 3 的 ELF 装载链
3. 最后再继续 Phase 4 及之后阶段

这是风险最低、验证成本最低、也最适合当前 SysToy 代码基线的迁移路径。
