from __future__ import annotations

import argparse
import json
from pathlib import Path

from lab_catalog import DEFAULT_NOTEBOOK_PATH, LABS, NOTEBOOK_TITLE, REPO_ROOT


def markdown_cell(text: str) -> dict:
    return {
        "cell_type": "markdown",
        "metadata": {},
        "source": [line + "\n" for line in text.strip().splitlines()],
    }


def code_cell(code: str) -> dict:
    return {
        "cell_type": "code",
        "execution_count": None,
        "metadata": {},
        "outputs": [],
        "source": [line + "\n" for line in code.strip().splitlines()],
    }


def build_cells() -> list[dict]:
    cells: list[dict] = [
        markdown_cell(
            f"""
# Tutorial: {NOTEBOOK_TITLE}

这个 notebook 将 SysToy 课程统一项目整理为一个可阅读、可复用、可联动 CLI 演示器的六阶段教学材料。

使用方式：

1. 先阅读每个 Lab 的要求与仓库映射
2. 按需执行本 notebook 中的命令单元
3. 在终端运行 `python tools/lab_demo.py` 进入交互演示
"""
        ),
        markdown_cell(
            """
## 仓库说明

- 当前 SysToy 真实运行主线为 `QEMU RISC-V32 virt`
- 课程要求中的 `MBR / Bootloader` 在本仓库中以“教学对照 + 当前 RISC-V 启动链等价演示”的方式组织
- 所有文档、命令与脚本统一使用 UTF-8
"""
        ),
        code_cell(
            """
from pathlib import Path
import json
import subprocess
import sys

REPO_ROOT = Path.cwd()
print(REPO_ROOT)
"""
        ),
    ]

    for lab in LABS:
        requirement_lines = "\n".join(f"- {item}" for item in lab["requirements"])
        mapping_lines = "\n".join(f"- {item}" for item in lab["repo_mapping"])
        evidence_lines = "\n".join(f"- `{item}`" for item in lab["evidence_files"])
        command_lines = "\n".join(f"- `{item}`" for item in lab["commands"])

        cells.append(
            markdown_cell(
                f"""
## Lab {lab["number"]}: {lab["title"]}

**目标**

{lab["objective"]}

**课程要求拆解**

{requirement_lines}

**在 SysToy 当前仓库中的映射**

{mapping_lines}

**证据文件**

{evidence_lines}

**建议命令**

{command_lines}
"""
            )
        )

        first_file = lab["evidence_files"][0]
        cells.append(
            code_cell(
                f"""
from pathlib import Path
path = Path(r"{first_file}")
print(path.read_text(encoding="utf-8"))
"""
            )
        )

        cells.append(
            code_cell(
                f"""
import subprocess
import sys

subprocess.run(
    [sys.executable, "tools/lab_demo.py", "--lab", "{lab["number"]}", "--summary"],
    check=True,
)
"""
            )
        )

    cells.append(
        markdown_cell(
            """
## CLI 交互入口

统一演示入口：

- `python tools/lab_demo.py`
- `python tools/lab_demo.py --lab 6 --action 2`

前者进入菜单模式，后者直接执行指定 Lab 的指定动作。
"""
        )
    )

    return cells


def build_notebook() -> dict:
    return {
        "cells": build_cells(),
        "metadata": {
            "kernelspec": {
                "display_name": "Python 3",
                "language": "python",
                "name": "python3",
            },
            "language_info": {
                "name": "python",
                "version": "3.12",
            },
        },
        "nbformat": 4,
        "nbformat_minor": 5,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build the unified SysToy six-lab notebook.")
    parser.add_argument("--out", type=Path, default=DEFAULT_NOTEBOOK_PATH, help="Output notebook path.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    out_path = args.out if args.out.is_absolute() else REPO_ROOT / args.out
    out_path.parent.mkdir(parents=True, exist_ok=True)
    notebook = build_notebook()
    out_path.write_text(json.dumps(notebook, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote notebook to {out_path}")


if __name__ == "__main__":
    main()
