from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from lab_catalog import LABS, REPO_ROOT


def get_lab(number: int) -> dict:
    for lab in LABS:
        if lab["number"] == number:
            return lab
    raise SystemExit(f"Unknown lab number: {number}")


def print_lab_summary(lab: dict) -> None:
    print(f"Lab {lab['number']}: {lab['title']}")
    print(f"目标: {lab['objective']}")
    print("课程要求:")
    for item in lab["requirements"]:
        print(f"  - {item}")
    print("仓库映射:")
    for item in lab["repo_mapping"]:
        print(f"  - {item}")
    print("证据文件:")
    for item in lab["evidence_files"]:
        print(f"  - {item}")


def show_file(path_str: str, start: int | None = None, end: int | None = None) -> None:
    path = REPO_ROOT / path_str
    text = path.read_text(encoding="utf-8")
    if start is None and end is None:
        print(text)
        return

    lines = text.splitlines()
    start_index = max((start or 1) - 1, 0)
    end_index = min(end or len(lines), len(lines))
    for idx in range(start_index, end_index):
        print(f"{idx + 1:04d}: {lines[idx]}")


def run_command(command: str) -> None:
    print(f"$ {command}")
    subprocess.run(command, cwd=REPO_ROOT, shell=True, check=True)


def perform_action(lab: dict, action_key: str) -> None:
    action = next((item for item in lab["cli_actions"] if item["key"] == action_key), None)
    if action is None:
        raise SystemExit(f"Unknown action {action_key} for lab {lab['number']}")

    action_type = action["type"]
    if action_type == "show_file":
        show_file(action["path"])
        return
    if action_type == "show_snippet":
        show_file(action["path"], action["start"], action["end"])
        return
    if action_type == "run":
        run_command(action["command"])
        return
    if action_type == "run_sequence":
        for command in action["commands"]:
            run_command(command)
        return

    raise SystemExit(f"Unsupported action type: {action_type}")


def interactive_mode() -> None:
    while True:
        print("\nSysToy Unified Lab Demo")
        for lab in LABS:
            print(f"{lab['number']}. {lab['title']}")
        print("q. 退出")
        choice = input("选择 lab: ").strip().lower()
        if choice == "q":
            return
        if not choice.isdigit():
            print("请输入 1-6 或 q。")
            continue

        lab = get_lab(int(choice))
        print()
        print_lab_summary(lab)
        print("可执行动作:")
        for action in lab["cli_actions"]:
            print(f"  {action['key']}. {action['name']}")
        print("  b. 返回上一级")

        while True:
            action_choice = input("选择动作: ").strip().lower()
            if action_choice == "b":
                break
            try:
                perform_action(lab, action_choice)
            except subprocess.CalledProcessError as exc:
                print(f"命令执行失败，退出码: {exc.returncode}")
            except Exception as exc:
                print(f"执行失败: {exc}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Interactive CLI demos for the six SysToy labs.")
    parser.add_argument("--lab", type=int, help="Directly choose a lab number.")
    parser.add_argument("--action", help="Directly run a lab action key.")
    parser.add_argument("--summary", action="store_true", help="Only print the selected lab summary.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.lab is None:
        interactive_mode()
        return

    lab = get_lab(args.lab)
    if args.summary:
        print_lab_summary(lab)
        return

    if args.action is None:
        print_lab_summary(lab)
        print("可执行动作:")
        for action in lab["cli_actions"]:
            print(f"  {action['key']}. {action['name']}")
        return

    perform_action(lab, args.action)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as exc:
        print(f"命令执行失败: {exc.cmd}", file=sys.stderr)
        raise SystemExit(exc.returncode) from exc
