# SysToy Phase 3 实现说明

## 1. 当前版本说明

本文档描述的是 SysToy 从 x86 版本过渡到 `QEMU RISC-V32 virt` 之后的 Phase 3 状态。

当前代码主线已经切换到：

1. `QEMU RISC-V32 virt`
2. RISC-V ELF 用户程序
3. RISC-V trap 与 `ecall`
4. 最小非分页内存分配
5. 最小 ELF 装载链

## 2. 当前已实现能力

### 2.1 Phase 2 等价能力

已经恢复：

1. 内核启动
2. UART 串口输出
3. 进入 U-mode
4. 用户程序触发 `ecall`
5. 内核接收 trap 并完成最小 syscall 输出

### 2.2 Phase 3 等价能力

已经恢复：

1. 用户程序构建为 `ELF32` 可执行文件
2. 内核解析 ELF Header 和 Program Header
3. 内核按 `PT_LOAD` 段装载用户程序
4. 内核通过最小分配器为 ELF 映像分配空间
5. 内核输出 ELF 大小、装载后映像大小和入口地址

## 3. 当前运行结果

当前使用以下命令验证：

```bash
wsl bash -lc "cd /mnt/e/Projects/SysToy && make -f Makefile.wsl all"
wsl bash -lc "cd /mnt/e/Projects/SysToy && timeout 5s qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf"
```

当前稳定输出为：

```text
SysToy RISC-V kernel start
User ELF size: 0x00001234
[elf] loaded image size: 0x0000001A
[elf] entry point: 0x80400000
Switching to umode user program...
[syscall] hello um
```

## 4. 当前实现边界

### 4.1 syscall 样例边界

当前用户态 syscall 已跑通，但为了先稳定 RISC-V 主链路，用户程序暂时采用“寄存器内打包字节”的最小样例，而不是“用户态传入完整字符串指针”的最终形式。

当前样例重点验证的是：

1. U-mode 切换可用
2. `ecall` 可用
3. trap 分发可用
4. syscall 输出链路可用

### 4.2 内存管理边界

当前只实现：

1. 最小线性分配
2. 按页对齐的顺序分配

当前尚不支持：

1. 释放
2. 复用
3. 碎片整理
4. 分页或虚拟内存映射

### 4.3 ELF 装载边界

当前重点是最小 `PT_LOAD` 装载，不追求完整 ELF 生态支持。

## 5. 当前问题与待补项

后续建议继续补的内容：

1. 将当前 `hello um` 恢复成完整字符串指针传参版本
2. 完善 RISC-V ELF 的自动化测试环境
3. 补充 RISC-V 专项运行说明文档
4. 在此基础上继续推进 Phase 4 调度能力

## 6. 验证说明

当前机器上，RISC-V 构建与 QEMU 运行已验证通过。

`tests/elf_loader_test.py` 已存在，但自动化测试环境仍受 WSL 中 Python 虚拟环境依赖影响，尚未完全收尾。这个问题属于环境补全项，不影响当前代码主链的构建与运行验证。
