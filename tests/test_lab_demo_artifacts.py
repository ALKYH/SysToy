from pathlib import Path
import json
import subprocess
import sys


def test_lab_catalog_exposes_six_labs_with_cli_actions():
    sys.path.insert(0, str(Path("tools").resolve()))
    import lab_catalog

    assert lab_catalog.NOTEBOOK_TITLE == "SysToy Unified Six-Lab Tutorial"
    assert len(lab_catalog.LABS) == 6

    for expected_index, lab in enumerate(lab_catalog.LABS, start=1):
        assert lab["id"] == f"lab{expected_index}"
        assert lab["title"]
        assert lab["objective"]
        assert lab["repo_mapping"]
        assert lab["requirements"]
        assert lab["evidence_files"]
        assert lab["commands"]
        assert lab["cli_actions"]


def test_notebook_builder_generates_unified_ipynb_with_all_labs():
    output_path = Path("build/test-systoy-six-labs.ipynb")
    output_path.parent.mkdir(parents=True, exist_ok=True)

    subprocess.run(
        [sys.executable, "tools/build_lab_notebook.py", "--out", str(output_path)],
        check=True,
        capture_output=True,
        text=True,
    )

    notebook = json.loads(output_path.read_text(encoding="utf-8"))
    cell_text = "\n".join(
        "".join(cell.get("source", []))
        for cell in notebook["cells"]
    )

    assert notebook["metadata"]["language_info"]["name"] == "python"
    assert "SysToy Unified Six-Lab Tutorial" in cell_text
    for index in range(1, 7):
        assert f"## Lab {index}:" in cell_text
    assert "tools/lab_demo.py" in cell_text


def test_kernel_exposes_lab_selection_and_serial_input_paths():
    kernel_text = Path("kernel/kernel.c").read_text(encoding="utf-8")
    runtime_text = Path("kernel/runtime/runtime_state.c").read_text(encoding="utf-8")
    lab6_text = Path("kernel/labs/lab6/lab6.c").read_text(encoding="utf-8")
    console_text = Path("kernel/console.c").read_text(encoding="utf-8")
    makefile_text = Path("Makefile.wsl").read_text(encoding="utf-8")

    assert "lab_selection_loop_entry" in kernel_text
    assert "select_lab_from_console" in runtime_text
    assert "run_lab1_environment" in runtime_text
    assert "run_lab6_fat32" in runtime_text
    assert "selected_lab" in runtime_text
    assert "Select disk program [1-4]" in lab6_text
    assert "console_read_char" in console_text
    assert "UART_RHR" in console_text
    assert "lab2_user_riscv.S" in makefile_text
    assert "lab3_user_riscv.S" in makefile_text
    assert "lab4_task_a_riscv.S" in makefile_text
    assert "lab4_task_b_riscv.S" in makefile_text
    assert "lab5_task_a_riscv.S" in makefile_text
    assert "lab5_task_b_riscv.S" in makefile_text


def test_kernel_can_return_to_lab_selector_after_program_completion():
    runtime_text = Path("kernel/runtime/runtime_state.c").read_text(encoding="utf-8")
    trap_text = Path("kernel/trap.c").read_text(encoding="utf-8")
    entry_text = Path("kernel/kernel_entry_riscv.S").read_text(encoding="utf-8")

    assert "menu_return_requested" in runtime_text
    assert "lab_selection_loop_entry" in runtime_text
    assert "[menu] program finished, returning to lab selector" in trap_text
    assert "return_to_lab_selector_from_trap" in trap_text
    assert ".globl return_to_lab_selector_from_trap" in entry_text


def test_kernel_sources_are_split_by_runtime_and_lab_folders():
    runtime_dir = Path("kernel/runtime")
    labs_dir = Path("kernel/labs")
    makefile_text = Path("Makefile.wsl").read_text(encoding="utf-8")

    assert runtime_dir.is_dir()
    for lab_name in ["lab1", "lab2", "lab3", "lab4", "lab5", "lab6"]:
        assert (labs_dir / lab_name).is_dir()

    assert "RUNTIME_DIR := kernel/runtime" in makefile_text
    assert "LABS_DIR := kernel/labs" in makefile_text
