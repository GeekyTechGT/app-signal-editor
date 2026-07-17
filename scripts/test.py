#!/usr/bin/env python3
"""
test.py — test runner for this project, invoked by scripts/project_manager.py [6].

Runs CTest for the selected preset and writes text/HTML reports under the
directory configured in the ``test_report`` section of project.json.

Can also be run directly:
    uv run python scripts/test.py --preset windows-mingw64-debug

Reusable machinery (preset catalogue, executors, JUnit parsing, report
rendering) lives in project_manager.py; this file only holds the parts a
project is expected to customize.
"""

from __future__ import annotations

import argparse
import shutil
import sys
import webbrowser
from pathlib import Path
from typing import Optional

sys.path.insert(0, str(Path(__file__).resolve().parent))

from project_manager import (  # noqa: E402
    _HOST,
    CONSOLE,
    PROJECT_ROOT,
    _check_executor,
    _dir_size_bytes,
    _generate_html,
    _get_preset,
    _has_executed_tests,
    _win_to_wsl_path,
    _write_text_report,
    build_cmd,
    find_ctest,
    load_metadata,
    run_capture,
)

from rich.prompt import Confirm  # noqa: E402
from rich.rule import Rule  # noqa: E402


def _ctest_command(preset: dict, meta: dict, ctest: str, junit_xml: Path) -> list[str]:
    """CTest invocation for the preset's executor, with a JUnit path the
    executor can actually see (host, WSL mount, or Docker volume)."""
    preset_name = preset["name"]

    if preset["executor"] == "native":
        junit_arg = str(junit_xml)
        program = ctest
    elif preset["executor"] == "wsl":
        junit_arg = _win_to_wsl_path(junit_xml) if _HOST == "Windows" else str(junit_xml)
        program = "ctest"
    else:  # docker
        junit_arg = f"/workspace/build/{preset_name}/test_results.xml"
        program = "ctest"

    base = [program, "--preset", preset_name,
            "-V", "--output-on-failure", "--output-junit", junit_arg]
    return build_cmd(preset, base, meta)


def run_tests(preset: dict, meta: dict, ctest: str) -> int:
    preset_name = preset["name"]
    CONSOLE.print(Rule(f"[bold cyan]Tests — {preset_name} [{preset['executor']}]"))

    build_dir = PROJECT_ROOT / "build" / preset_name
    if not build_dir.exists():
        CONSOLE.print("[red]Build directory not found. Configure + Build first.[/]")
        return 1
    if not _check_executor(preset, meta):
        return 1

    test_report_cfg = meta.get("test_report", {})
    report_enabled = test_report_cfg.get("enabled", True)
    save_text = test_report_cfg.get("save_text", True)
    save_html = test_report_cfg.get("save_html", True)
    tail_chars = int(test_report_cfg.get("console_tail_characters", 600000))
    max_bytes = int(test_report_cfg.get("artifacts_max_bytes", 5_000_000_000))

    report_base = Path(test_report_cfg.get("directory") or "test-reports")
    if not report_base.is_absolute():
        report_base = PROJECT_ROOT / report_base
    report_dir = report_base / preset_name

    junit_xml = build_dir / "test_results.xml"

    cmd = _ctest_command(preset, meta, ctest, junit_xml)
    pname = preset_name if preset["executor"] == "native" else None
    code, output = run_capture(cmd, preset_name=pname)
    CONSOLE.print(output)

    executed_tests = _has_executed_tests(output, junit_xml)

    if not report_enabled:
        CONSOLE.print("[dim]test_report.enabled is false — report generation skipped.[/]")
    elif executed_tests:
        report_dir.mkdir(parents=True, exist_ok=True)

        if junit_xml.exists():
            shutil.copy2(junit_xml, report_dir / junit_xml.name)

        html_report: Optional[Path] = None
        if save_html:
            html_report = report_dir / "test_results.html"
            _generate_html(html_report, output, junit_xml, preset_name, meta, tail_chars)
            CONSOLE.print(f"\n[bold]HTML report:[/] {html_report}")

        if save_text:
            text_report = report_dir / "test_results.txt"
            _write_text_report(text_report, output, tail_chars)
            CONSOLE.print(f"[bold]Text report:[/] {text_report}")

        dir_size = _dir_size_bytes(report_dir)
        if dir_size > max_bytes:
            CONSOLE.print(
                f"[yellow]⚠ Report directory {report_dir} is {dir_size / 1e9:.2f} GB, "
                f"exceeding artifacts_max_bytes ({max_bytes / 1e9:.2f} GB).[/]"
            )

        if html_report is not None and Confirm.ask("Open HTML report in browser?", default=True):
            webbrowser.open(html_report.as_uri())
    else:
        CONSOLE.print("[yellow]No tests were discovered — report skipped.[/]")

    if code == 0 and executed_tests:
        CONSOLE.print("[bold green]✓ All tests passed[/]")
    elif code == 0:
        CONSOLE.print("[yellow]No tests were executed.[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Some tests failed (exit {code})[/]")

    return code


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the test suite for a CMake preset.")
    parser.add_argument("--preset", required=True, help="CMake preset name")
    args = parser.parse_args()

    meta = load_metadata()
    try:
        preset = _get_preset(args.preset)
    except KeyError:
        CONSOLE.print(f"[bold red]Unknown preset:[/] {args.preset}")
        return 2

    ctest = find_ctest(meta)
    if not ctest and preset["executor"] == "native":
        CONSOLE.print("[bold red]ctest not found.[/]")
        return 2

    return run_tests(preset, meta, ctest or "ctest")


if __name__ == "__main__":
    sys.exit(main())
