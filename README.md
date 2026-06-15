# SysToy

面向操作系统课程的统一实验项目仓库。项目目标不是做一个“功能很全”的操作系统，而是提供一条可教学、可演进、可验收的最小系统主线，让团队能在同一个工程里逐步完成从启动、内核、用户程序到调度、同步、文件系统的阶段式建设。

当前仓库的运行主线已经从早期的 x86/MBR/BIOS 方案切换到 `QEMU RISC-V32 virt`。如果你是第一次接手这个仓库，建议把它当成“课程统一项目 + 当前 RISC-V 实现基线”来理解。

## 1. 你可以从这个仓库得到什么

- 一套统一的课程项目主线，而不是零散实验。
- 一组面向教学组织的文档：PRD、阶段说明、评分标准、环境要求、自检清单。
- 一条当前可运行的 RISC-V 内核最小链路：启动、UART 输出、trap/syscall、最小 ELF 装载。
- 一套适合继续推进 Phase 4 到 Phase 6 的代码与文档骨架。

## 2. 第一次进入仓库先看什么

推荐阅读顺序：

1. [docs/prd-system-lab-unified-project.md](docs/prd-system-lab-unified-project.md)
2. [docs/repository-structure-and-conventions.md](docs/repository-structure-and-conventions.md)
3. [docs/llm-friendly-workbreakdown.md](docs/llm-friendly-workbreakdown.md)
4. [docs/phase-riscv-refactor-notes.md](docs/phase-riscv-refactor-notes.md)
5. [docs/scoring-rubric.md](docs/scoring-rubric.md)

如果你只想快速上手运行，先看本 README 的“快速启动”部分，再回来看上面几篇文档。

## 3. 当前项目状态

按仓库内现有文档，当前主线可以这样理解：

- `Phase 0`：仓库规范、交付约定、文档框架已建立。
- `Phase 1`：环境搭建要求、自检清单、基线验收标准已形成。
- `Phase 2`：RISC-V 等价的最小启动链、控制台输出、trap/syscall 已恢复。
- `Phase 3`：最小 RISC-V ELF 用户程序装载链已恢复。
- `Phase 4`：处于推进中，仓库内已有双用户程序、trap/timer/scheduling 相关主线说明。
- `Phase 5`：未形成稳定主线交付。
- `Phase 6`：仓库中已经出现 FAT32 相关工具与说明，但仍建议按阶段文档逐步推进，不要把“存在脚手架”等同于“阶段已完成”。

一句话概括：现在最适合把这个仓库当作“RISC-V 教学内核基线 + 后续阶段持续演进平台”。

## 4. 快速启动

### 4.1 环境前提

当前实际运行路径依赖以下环境：

- Windows 10
- WSL2
- `make`
- `python3`
- `qemu-system-riscv32`
- `riscv64-unknown-elf-gcc`
- `riscv64-unknown-elf-ld`

补充说明：

- 课程总文档中仍保留了 `NASM / GCC / QEMU` 的通用要求，因为那是统一课程项目的历史主线描述。
- 但就当前仓库代码来说，真实构建入口已经是 RISC-V 交叉工具链，不再依赖 x86 `boot.asm` 路径。

### 4.2 在 Windows 侧直接启动

仓库根目录的 `Makefile` 已经封装了 WSL 调用，通常直接执行即可：

```powershell
make
make run
```

对应行为：

- `make` 会转发到 WSL 中执行 `make -f Makefile.wsl all`
- `make run` 会转发到 WSL 中执行 `make -f Makefile.wsl run`

### 4.3 在 WSL 内手动启动

如果你想显式控制命令，使用下面这组：

```bash
cd /mnt/e/Projects/SysToy
make -f Makefile.wsl all
make -f Makefile.wsl run
```

当前 `run` 目标实际调用的是：

```bash
qemu-system-riscv32 -machine virt -nographic -bios none -kernel build/kernel.elf
```

### 4.4 预期看到的最小结果

按 [docs/phase-riscv-refactor-notes.md](docs/phase-riscv-refactor-notes.md) 的当前说明，最小运行输出应接近：

```text
SysToy RISC-V kernel start
User ELF size: ...
[elf] loaded image size: ...
[elf] entry point: ...
Switching to umode user program...
[syscall] hello um
```

如果你看不到这类输出，优先检查：

1. 交叉编译工具链是否存在
2. WSL2 是否正常
3. `qemu-system-riscv32` 是否可执行
4. `build/kernel.elf` 是否成功生成

## 5. 仓库结构速览

当前仓库里最关键的目录如下：

```text
SysToy/
├─ boot/        # 启动相关历史或阶段性内容
├─ kernel/      # 当前 RISC-V 内核主线代码
├─ user/        # 用户态程序与链接脚本
├─ tools/       # FAT32 镜像构建、布局查看等辅助工具
├─ tests/       # 最小自动化验证
├─ docs/        # PRD、阶段说明、规范、Rubric、拆解文档
├─ build/       # 本地产物
├─ outputs/     # 产出物与外部素材
├─ projects/    # 其他项目化材料，例如演示视频工程
├─ Makefile
└─ Makefile.wsl
```

如果你只关心当前代码主线，优先看：

- [kernel/kernel.c](kernel/kernel.c)
- [kernel/trap.c](kernel/trap.c)
- [kernel/trap_riscv.S](kernel/trap_riscv.S)
- [kernel/elf_loader.c](kernel/elf_loader.c)
- [user/user_task_a_riscv.S](user/user_task_a_riscv.S)
- [user/user_task_b_riscv.S](user/user_task_b_riscv.S)
- [tools/make_fat32_image.py](tools/make_fat32_image.py)

## 6. 文档导航

### 项目级文档

- [docs/prd-system-lab-unified-project.md](docs/prd-system-lab-unified-project.md)
  - 统一项目的背景、目标、六阶段范围、验收与治理机制。
- [docs/scoring-rubric.md](docs/scoring-rubric.md)
  - 教师和助教可直接参考的阶段评分口径。
- [docs/repository-structure-and-conventions.md](docs/repository-structure-and-conventions.md)
  - 仓库命名、UTF-8、交付结构、演示材料归档要求。

### 启动与环境文档

- [docs/environment-requirements.md](docs/environment-requirements.md)
  - 课程标准环境要求与支持边界。
- [docs/environment-self-checklist.md](docs/environment-self-checklist.md)
  - 学生团队自检清单。
- [docs/baseline-run-acceptance.md](docs/baseline-run-acceptance.md)
  - “环境已跑通”的统一验收标准。

### 当前实现与推进文档

- [docs/phase-riscv-refactor-notes.md](docs/phase-riscv-refactor-notes.md)
  - 从 x86 主线切换到 RISC-V 主线的当前说明。
- [docs/phase-2-implementation-notes.md](docs/phase-2-implementation-notes.md)
- [docs/phase-3-implementation-notes.md](docs/phase-3-implementation-notes.md)
- [docs/phase-6-fat32-notes.md](docs/phase-6-fat32-notes.md)
- [docs/llm-friendly-workbreakdown.md](docs/llm-friendly-workbreakdown.md)
  - 适合继续拆分任务、交给 LLM/Agent 接力推进的工作包规划。

## 7. 推荐的接手方式

### 如果你是学生项目组

建议路径：

1. 先做环境自检
2. 先跑通当前 RISC-V 基线
3. 再读对应阶段说明
4. 每推进一个阶段，都补充运行结果和问题记录

### 如果你是助教或教师

建议路径：

1. 先看 PRD 和 Rubric
2. 再看当前主线实现状态
3. 最后按阶段验收“可运行、可说明、可复现”

### 如果你是继续开发这个仓库的人

建议路径：

1. 先确认当前代码真实入口，不要直接按早期 x86 经验修改
2. 优先保持阶段边界清晰
3. 一次只推进一个机制，例如 trap、调度、同步、FAT32 读取
4. 代码修改与阶段文档同步提交，避免“代码能跑但文档失真”

## 8. 当前构建链简述

当前 `Makefile.wsl` 的主线大致如下：

1. 编译 `kernel/` 下的 RISC-V 内核对象文件
2. 编译 `user/` 下的两个 RISC-V 用户程序
3. 将用户 ELF 和 FAT32 镜像转成可链接对象
4. 链接生成 `build/kernel.elf`
5. 使用 `qemu-system-riscv32 -machine virt` 运行

这意味着仓库已经不仅是“打印一行字的最小内核”，而是在为后续用户程序装载、调度和文件系统演进保留接口。

## 9. 协作约定

请尽量遵守以下规则：

- 所有文档、代码、说明材料统一使用 UTF-8 编码
- 阶段文档放在 `docs/` 下，不要散落在根目录
- 演示截图、日志、录像等证明材料要和阶段对应
- 不要用“最终版”“新版”“test2”这类无语义命名
- 每次推进都要说明“做了什么、怎么验证、当前边界在哪里”

对于中文相关的 PowerShell 读写，仓库协作默认按 UTF-8 处理。

## 10. 下一步做什么

如果你现在要继续推进主线，最推荐的顺序是：

1. 收尾并固化 Phase 4 的调度验证口径
2. 建立 Phase 5 的共享内存与 P/V 同步抽象
3. 再把 Phase 6 的 FAT32 读取与 ELF 装载闭环做完整

如果你只是需要一个“今天就能跑起来”的入口，那就先执行：

```powershell
make
make run
```

跑通以后，再回头按文档体系逐步理解这个项目。
