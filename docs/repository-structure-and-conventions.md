# SysToy 仓库结构与规范

## 1. 目标

定义 SysToy 统一项目的仓库结构、命名方式、文档要求和编码规范，确保教师、助教和学生在同一套规则下协作。

## 2. 仓库顶层结构建议

```text
SysToy/
├─ boot/                  # Bootloader、MBR、启动相关代码
├─ kernel/                # 内核核心代码
├─ user/                  # 用户态程序
├─ tools/                 # 构建、镜像、检查和辅助脚本
├─ assets/                # 演示截图、流程图、静态资源
├─ build/                 # 本地产物目录，不提交大体积生成文件
├─ docs/                  # PRD、阶段说明、模板、Rubric、FAQ
├─ tests/                 # 自动化检查脚本、解析验证、辅助测试
└─ Makefile               # 顶层构建入口
```

## 3. `docs/` 目录约定

```text
docs/
├─ templates/                           # 模板文件
├─ phase-0-1-delivery-summary.md        # 当前阶段交付说明
├─ prd-system-lab-unified-project.md    # 统一项目 PRD
├─ llm-friendly-workbreakdown.md        # LLM 友好拆解
├─ scoring-rubric.md                    # 评分标准
├─ environment-requirements.md          # 环境要求
├─ environment-self-checklist.md        # 环境自检清单
├─ baseline-run-acceptance.md           # 基线运行验收标准
└─ repository-structure-and-conventions.md
```

## 4. 命名规范

### 4.1 文件命名

1. Markdown 文档统一使用小写英文加连字符风格，如 `phase-2-design.md`。
2. 汇编、C、头文件、脚本按语言社区常规命名。
3. 禁止出现“最终版”“新建文档”“test2”这类无法表达语义的名称。

### 4.2 目录命名

1. 顶层目录使用清晰业务语义，如 `boot`、`kernel`、`user`。
2. 阶段性文档不要散落在根目录，统一放在 `docs/` 下。

## 5. 文档规范

1. 每个阶段至少包含：
   - 阶段目标
   - 实现说明
   - 运行结果
   - 问题记录
   - 验收记录
2. 所有阶段文档必须可追溯到对应阶段。
3. 演示材料命名应能对应阶段与日期。

## 6. 编码规范

1. 所有文档、代码和说明材料统一使用 UTF-8 编码。
2. 中文内容允许使用中文标点，但同一文档内格式风格要一致。
3. 脚本和源码文件优先使用 UTF-8 无 BOM。

## 7. 提交与交付规范

1. 每个阶段的交付物应至少包括：
   - 可运行代码或镜像
   - 阶段说明文档
   - 运行证明材料
2. 教师验收前，团队应先完成自检。
3. 不允许只提交代码而没有说明文档。

## 8. 演示材料归档规范

建议按如下规则归档：

```text
assets/
├─ phase-1/
├─ phase-2/
├─ phase-3/
├─ phase-4/
├─ phase-5/
└─ phase-6/
```

每个目录中可包含：

1. 截图
2. 运行日志
3. 演示录像
4. 结构图

## 9. FAQ 维护建议

建议在 `docs/faq.md` 中持续记录：

1. 环境问题
2. 编译问题
3. QEMU 启动问题
4. 中断与系统调用问题
5. FAT32 解析问题

## 10. 最低执行要求

1. 目录结构明确。
2. 命名风格统一。
3. 所有文件使用 UTF-8。
4. 文档、代码、演示材料能互相对应。
