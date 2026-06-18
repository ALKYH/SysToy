# SysToy 六阶段统一 Notebook 与 CLI 演示说明

## 1. 目标

将课程要求中的 6 个项目统一整理为：

1. 一个可阅读、可执行的 Jupyter Notebook
2. 一个可在终端内交互演示的 CLI 工具

两者都基于当前 SysToy 仓库真实实现，不额外伪造不存在的独立工程。

## 2. 产物位置

1. Notebook 生成脚本：`tools/build_lab_notebook.py`
2. CLI 演示入口：`tools/lab_demo.py`
3. 共享 Lab 元数据：`tools/lab_catalog.py`
4. 默认 Notebook 输出：`output/jupyter-notebook/systoy-unified-six-lab-tutorial.ipynb`
5. 内核内 Lab 选择入口：`kernel/kernel.c`

## 3. 使用方式

### 3.1 生成 Notebook

```powershell
python tools/build_lab_notebook.py
```

### 3.2 打开 CLI 交互演示

```powershell
python tools/lab_demo.py
```

### 3.3 直接查看某个 Lab 摘要

```powershell
python tools/lab_demo.py --lab 3 --summary
```

### 3.4 直接执行某个 Lab 的动作

```powershell
python tools/lab_demo.py --lab 6 --action 2
```

## 4. 教学组织方式

### Lab 1

聚焦环境搭建、自检与基线构建。

### Lab 2

保留课程中的 Bootloader / MBR 需求语义，但在当前仓库中以 RISC-V 启动链、trap、syscall 作为等价教学主线。

### Lab 3

聚焦不带 Paging 的最小内存管理与 ELF 装载。

### Lab 4

聚焦基于 timer interrupt 的双任务调度。

### Lab 5

聚焦共享内存、P/V 风格信号量与银行账户示例。

### Lab 6

聚焦 FAT32 镜像构建、布局查看与 FAT32 中的 ELF 装载闭环。

## 5. 边界说明

1. 当前仓库真实主线是 `QEMU RISC-V32 virt`，不是原始 x86 MBR 运行链。
2. 因此 Notebook 和 CLI 会明确区分“课程要求原始表述”和“当前 SysToy 仓库中的实现映射”。
3. 这样既保留课程语义，也避免把当前仓库说成它并没有实现的状态。

## 6. 内核内 Lab 选择

当前版本已经把“选择不同 Lab 并运行对应程序”的主入口下沉到内核内。

启动后，串口会显示 1 到 6 的选择菜单：

1. `Lab1`：环境说明
2. `Lab2`：最小用户程序 + syscall 路径
3. `Lab3`：内存与 ELF 装载路径
4. `Lab4`：双任务调度
5. `Lab5`：共享内存与同步
6. `Lab6`：FAT32 中加载 ELF

当前实现中：

1. `Lab2` 使用单独的 `user/lab2_user_riscv.S`
2. `Lab3` 使用单独的 `user/lab3_user_riscv.S`
3. `Lab4` 使用独立的 `lab4_task_a_riscv.S` 与 `lab4_task_b_riscv.S`
4. `Lab5` 使用独立的 `lab5_task_a_riscv.S` 与 `lab5_task_b_riscv.S`
5. `Lab6` 进入 FAT32 二级菜单后，可手动选择加载 `LAB2/LAB3/LAB4/LAB5` 对应的 ELF 组合

## 7. 参考资料

本仓库当前 `Lab5` / `kernel/runtime/runtime_state.c` 中涉及的原子同步实现，主要参考以下官方 RISC-V 规范资料：

1. RISC-V Unprivileged ISA 总入口：
   https://docs.riscv.org/reference/isa/unpriv/unpriv-index.html
2. `"A" Extension for Atomic Instructions, Version 2.1`：
   https://docs.riscv.org/reference/isa/unpriv/a-st-ext.html
3. `RVWMO Memory Consistency Model, Version 2.0`：
   https://docs.riscv.org/reference/isa/unpriv/rvwmo.html

其中：

1. `lr.w` / `sc.w` 的配对语义可直接参考 `"A"` 扩展章节。
2. `amoadd.w` 的原子读改写语义同样可参考 `"A"` 扩展章节。
3. `fence rw, rw` 的顺序约束语义，可结合 Unprivileged ISA 总说明与 `RVWMO` 章节一起理解。
