#!/usr/bin/env python3
"""
project_manager.py — interactive build manager for Signal Editor.

Activate the virtual environment first, then run:
    python scripts/project_manager.py

Supports three execution modes:
  native  — cmake runs directly in the current process
  wsl     — cmake runs inside WSL (wsl.exe on Windows, native on Linux)
  docker  — cmake runs inside Docker (docker run …)
"""

from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess
import sys
import textwrap
import webbrowser
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Bootstrap check
# ---------------------------------------------------------------------------
try:
    from rich.console import Console
    from rich.panel import Panel
    from rich.table import Table
    from rich.prompt import Prompt, Confirm
    from rich.rule import Rule
    from rich import box
except ImportError:
    print(
        "\n[ERROR] 'rich' is not installed.\n"
        "Activate the virtual environment first:\n"
        "  Windows : pyvenv\\Scripts\\activate\n"
        "  Linux   : source pyvenv/bin/activate\n"
        "Then: pip install -r requirements.txt\n"
    )
    sys.exit(1)

# ---------------------------------------------------------------------------
# Globals
# ---------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parent.parent
PROJECT_JSON = PROJECT_ROOT / "project.json"
CONSOLE = Console()
_HOST = platform.system()   # "Windows" or "Linux"

# CMake/CTest fallback paths on Windows
_WIN_CMAKE = [
    "C:/eng_apps/cmake/cmake-4.2.0/bin/cmake.exe",
    "C:/Program Files/CMake/bin/cmake.exe",
]
_WIN_CTEST = [
    "C:/eng_apps/cmake/cmake-4.2.0/bin/ctest.exe",
    "C:/Program Files/CMake/bin/ctest.exe",
]

# ---------------------------------------------------------------------------
# Preset catalogue
# ---------------------------------------------------------------------------
PRESETS: list[dict] = [
    {
        "name":           "windows-mingw64-debug",
        "display":        "Windows Qt6 GCC/MinGW64  [Debug]",
        "executor":       "native",
        "arch":           "64-bit",
        "build_type":     "Debug",
        "host":           "Windows",
        "shared_variant": "windows-mingw64-debug",
    },
    {
        "name":           "windows-mingw64-release",
        "display":        "Windows Qt6 GCC/MinGW64  [Release]",
        "executor":       "native",
        "arch":           "64-bit",
        "build_type":     "Release",
        "host":           "Windows",
        "shared_variant": "windows-mingw64-release",
    },
    {
        "name":           "linux-gcc-debug",
        "display":        "Linux/WSL Qt6 GCC  [Debug]",
        "executor":       "wsl",
        "arch":           "native",
        "build_type":     "Debug",
        "host":           "any",
        "shared_variant": "linux-gcc-debug",
    },
    {
        "name":           "linux-gcc-release",
        "display":        "Linux/WSL Qt6 GCC  [Release]",
        "executor":       "wsl",
        "arch":           "native",
        "build_type":     "Release",
        "host":           "any",
        "shared_variant": "linux-gcc-release",
    },
    {
        "name":           "docker-gcc-debug",
        "display":        "Docker Qt6 GCC  [Debug]",
        "executor":       "docker",
        "arch":           "native",
        "build_type":     "Debug",
        "host":           "any",
        "shared_variant": "linux-gcc-debug",
    },
    {
        "name":           "docker-gcc-release",
        "display":        "Docker Qt6 GCC  [Release]",
        "executor":       "docker",
        "arch":           "native",
        "build_type":     "Release",
        "host":           "any",
        "shared_variant": "linux-gcc-release",
    },
]


def preset_is_available(p: dict) -> bool:
    if p["host"] == "Windows" and _HOST != "Windows":
        return False
    if p["executor"] == "wsl" and _HOST == "Windows":
        return shutil.which("wsl") is not None or shutil.which("wsl.exe") is not None
    if p["executor"] == "docker":
        return shutil.which("docker") is not None
    return True


def executor_label(p: dict) -> str:
    if p["executor"] == "native":
        return "[cyan]native[/]"
    if p["executor"] == "wsl":
        return "[cyan]native[/]" if _HOST == "Linux" else "[yellow]WSL[/]"
    if p["executor"] == "docker":
        return "[magenta]Docker[/]"
    return p["executor"]


# ---------------------------------------------------------------------------
# Tool discovery
# ---------------------------------------------------------------------------

def _find_exe(name: str, extra: list[str] | None = None) -> Optional[str]:
    found = shutil.which(name)
    if found:
        return found
    for c in (extra or []):
        if Path(c).is_file():
            return c
    return None


def find_cmake() -> Optional[str]:
    return _find_exe("cmake", _WIN_CMAKE if _HOST == "Windows" else [])


def find_ctest() -> Optional[str]:
    return _find_exe("ctest", _WIN_CTEST if _HOST == "Windows" else [])


# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------

def _win_to_wsl_path(p: Path) -> str:
    parts = p.parts
    drive = parts[0].rstrip(":\\").lower()
    rest  = "/".join(parts[1:])
    return f"/mnt/{drive}/{rest}"


def _win_to_docker_volume(p: Path) -> str:
    return str(p).replace("\\", "/")


# ---------------------------------------------------------------------------
# Command builders
# ---------------------------------------------------------------------------

def _build_native_cmd(base_cmd: list[str]) -> list[str]:
    return base_cmd


def _build_wsl_cmd(base_cmd: list[str]) -> list[str]:
    if _HOST == "Linux":
        return base_cmd
    wsl_path   = _win_to_wsl_path(PROJECT_ROOT)
    shell_cmd  = " ".join(f'"{a}"' if " " in a else a for a in base_cmd)
    return ["wsl.exe", "bash", "-lc", f"cd '{wsl_path}' && {shell_cmd}"]


def _build_docker_cmd(base_cmd: list[str], meta: dict) -> list[str]:
    image  = meta.get("docker_image", "signal-editor-builder")
    volume = _win_to_docker_volume(PROJECT_ROOT) if _HOST == "Windows" else str(PROJECT_ROOT)
    return ["docker", "run", "--rm", "-v", f"{volume}:/workspace",
            "-w", "/workspace", image] + base_cmd


def build_cmd(preset: dict, base_cmd: list[str], meta: dict) -> list[str]:
    ex = preset["executor"]
    if ex == "native":
        return _build_native_cmd(base_cmd)
    if ex == "wsl":
        return _build_wsl_cmd(base_cmd)
    if ex == "docker":
        return _build_docker_cmd(base_cmd, meta)
    return base_cmd


# ---------------------------------------------------------------------------
# Project metadata
# ---------------------------------------------------------------------------

def load_metadata() -> dict:
    if not PROJECT_JSON.exists():
        CONSOLE.print(f"[bold red]ERROR[/] project.json not found at {PROJECT_JSON}")
        sys.exit(1)
    with open(PROJECT_JSON, encoding="utf-8") as fh:
        return json.load(fh)


# ---------------------------------------------------------------------------
# Dependency validation
# ---------------------------------------------------------------------------

def _validate_dependencies(preset: dict, meta: dict) -> bool:
    """
    Verify that every declared dependency is present.

    Two modes, controlled by ``use_external_deps`` in project.json:

    * ``false`` (default): dependencies are pre-built binaries in
      ``shared_lib_base/<name>/<version>/<shared_variant>/``
    * ``true``: dependencies are vendored inside
      ``external/<name>/`` relative to the project root

    Returns True if all dependencies are satisfied; prints a verbose error and
    returns False otherwise.
    """
    dependencies     = meta.get("dependencies", [])
    use_external     = meta.get("use_external_deps", False)

    if not dependencies:
        return True

    all_ok = True

    if use_external:
        # ── external/ mode ────────────────────────────────────────────────────
        for dep in dependencies:
            dep_name = dep["name"]
            dep_path = PROJECT_ROOT / "external" / dep_name
            if not dep_path.exists():
                CONSOLE.print()
                CONSOLE.print(Panel(
                    f"[bold red]DEPENDENCY ERROR  (use_external_deps = true)[/]\n\n"
                    f"Dependency   : [yellow]{dep_name}[/]\n"
                    f"Expected at  : [dim]{dep_path}[/]\n\n"
                    f"[bold]This path does not exist![/]\n\n"
                    f"[bold cyan]How to fix:[/]\n"
                    f"  Copy or link the pre-built [yellow]{dep_name}[/] tree to:\n"
                    f"    [dim]{dep_path}[/]\n"
                    f"  Required layout: include/ lib/ bin/\n\n"
                    f"  Or set [yellow]use_external_deps[/] to [yellow]false[/] in project.json\n"
                    f"  to use the shared_lib folder instead.",
                    title="[bold red]Cannot configure — external dependency missing[/]",
                    border_style="red",
                    expand=False,
                ))
                CONSOLE.print()
                all_ok = False
    else:
        # ── shared_lib mode ───────────────────────────────────────────────────
        shared_lib_base = Path(meta.get("shared_lib_base", ""))
        shared_variant  = preset.get("shared_variant", preset["name"])

        for dep in dependencies:
            dep_name    = dep["name"]
            dep_version = dep["version"]
            dep_path    = shared_lib_base / dep_name / dep_version / shared_variant

            if not dep_path.exists():
                dep_base  = shared_lib_base / dep_name / dep_version
                available = sorted(d.name for d in dep_base.iterdir() if d.is_dir()) \
                    if dep_base.exists() else []

                CONSOLE.print()
                CONSOLE.print(Panel(
                    f"[bold red]DEPENDENCY ERROR[/]\n\n"
                    f"Dependency   : [yellow]{dep_name} {dep_version}[/]\n"
                    f"Required for : [yellow]{preset['name']}[/]  "
                    f"(shared variant: [yellow]{shared_variant}[/])\n\n"
                    f"Expected path:\n  [dim]{dep_path}[/]\n\n"
                    f"[bold]This path does not exist![/]\n" +
                    (f"\nAvailable variants for {dep_name} {dep_version}:\n" +
                     "\n".join(f"  [green]✓[/] {v}" for v in available)
                     if available else
                     f"\n[red]No variants deployed at all for {dep_name} {dep_version}.[/]") +
                    f"\n\n[bold cyan]How to fix:[/]\n"
                    f"  a) Build & deploy [yellow]{dep_name}[/] for variant "
                    f"'[yellow]{shared_variant}[/]'\n"
                    f"     using that library's project_manager.py\n"
                    f"  b) Switch to a preset whose shared_variant matches an available deployment\n"
                    f"  c) Set [yellow]use_external_deps[/] to [yellow]true[/] in project.json\n"
                    f"     and place the library under external/{dep_name}/",
                    title="[bold red]Cannot configure — dependency missing[/]",
                    border_style="red",
                    expand=False,
                ))
                CONSOLE.print()
                all_ok = False

    return all_ok


# ---------------------------------------------------------------------------
# Subprocess helpers
# ---------------------------------------------------------------------------

def _make_env(preset_name: str | None = None) -> dict:
    env = os.environ.copy()
    if _HOST != "Windows" or preset_name is None:
        return env

    meta = {}
    if PROJECT_JSON.exists():
        with open(PROJECT_JSON, encoding="utf-8") as fh:
            meta = json.load(fh)

    qt_root    = meta.get("qt_root",    "C:/eng_apps/Qt")
    qt_version = meta.get("qt_version", "6.10.1")

    if "gcc" in preset_name or "mingw" in preset_name:
        qt_mingw_bin = f"{qt_root}/Tools/mingw1310_64/bin"
        qt_lib_bin   = f"{qt_root}/{qt_version}/mingw_64/bin"
        qt_ninja     = f"{qt_root}/Tools/Ninja"
        prepend      = [qt_mingw_bin, qt_lib_bin, qt_ninja]
    else:
        return env

    sep          = os.pathsep
    env["PATH"]  = sep.join(prepend) + sep + env.get("PATH", "")
    return env


def run(cmd: list[str], preset_name: str | None = None) -> int:
    CONSOLE.print(f"[dim]$ {' '.join(cmd)}[/]")
    return subprocess.run(cmd, cwd=PROJECT_ROOT, env=_make_env(preset_name)).returncode


def run_capture(cmd: list[str], preset_name: str | None = None) -> tuple[int, str]:
    CONSOLE.print(f"[dim]$ {' '.join(cmd)}[/]")
    r = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True,
                       text=True, encoding="utf-8", errors="replace",
                       env=_make_env(preset_name))
    return r.returncode, r.stdout + r.stderr


# ---------------------------------------------------------------------------
# HTML test report
# ---------------------------------------------------------------------------

def _parse_junit(xml_path: Path) -> tuple[int, int, list[dict]]:
    tests: list[dict] = []
    total = failures = 0
    try:
        root = ET.parse(xml_path).getroot()
        for tc in root.iter("testcase"):
            total += 1
            f_el = tc.find("failure")
            e_el = tc.find("error")
            s_el = tc.find("skipped")
            if f_el is not None or e_el is not None:
                failures += 1
                status     = "FAIL"
                detail_el  = f_el if f_el is not None else e_el
                msg        = detail_el.get("message", "") if detail_el is not None else ""
            elif s_el is not None:
                status = "SKIP"
                msg    = ""
            else:
                status = "PASS"
                msg    = ""
            tests.append({
                "name":      tc.get("name", "?"),
                "classname": tc.get("classname", ""),
                "time":      float(tc.get("time", "0")),
                "status":    status,
                "message":   msg,
            })
    except Exception as exc:
        CONSOLE.print(f"[yellow]Warning: could not parse JUnit XML: {exc}[/]")
    return total, failures, tests


def _generate_html(out: Path, raw: str, junit: Optional[Path],
                   preset_name: str, meta: dict) -> None:
    total, failures, cases = 0, 0, []
    if junit and junit.exists():
        total, failures, cases = _parse_junit(junit)
    passed = total - failures
    ok = failures == 0
    sc = "#28a745" if ok else "#dc3545"

    rows = ""
    for tc in cases:
        bg = {"PASS": "#d4edda", "FAIL": "#f8d7da", "SKIP": "#fff3cd"}.get(tc["status"], "#fff")
        rows += (
            f"<tr style='background:{bg}'>"
            f"<td>{tc['classname']}</td><td>{tc['name']}</td>"
            f"<td style='text-align:center;font-weight:bold'>{tc['status']}</td>"
            f"<td style='text-align:right'>{tc['time']:.3f}s</td>"
            f"<td style='font-size:.8em;color:#555'>{tc['message'][:120]}</td>"
            f"</tr>\n"
        )
    tbl = (
        "<table border='1' cellspacing='0' cellpadding='6' "
        "style='border-collapse:collapse;width:100%;font-family:monospace;font-size:.9em'>"
        "<thead style='background:#343a40;color:white'>"
        "<tr><th>Suite</th><th>Test</th><th>Status</th><th>Time</th><th>Message</th></tr>"
        f"</thead><tbody>{rows}</tbody></table>"
    ) if cases else "<p><em>No test detail (JUnit XML unavailable).</em></p>"

    esc = raw.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    lbl = "ALL PASSED" if ok else f"{failures} FAILED"

    html = textwrap.dedent(f"""<!DOCTYPE html>
    <html lang="en"><head><meta charset="UTF-8"/>
    <title>Test Report — {meta['name']} {meta['version']}</title>
    <style>
      body{{font-family:'Segoe UI',Arial,sans-serif;margin:24px;background:#f5f5f5}}
      h1{{color:#1a1a2e}}
      .summary{{background:{sc};color:white;padding:12px 20px;border-radius:6px;
                font-size:1.2em;margin-bottom:20px}}
      .badge{{display:inline-block;padding:4px 12px;border-radius:12px;
              font-weight:bold;font-size:.85em}}
      .pass{{background:#28a745;color:white}} .fail{{background:#dc3545;color:white}}
      pre{{background:#1e1e1e;color:#d4d4d4;padding:16px;border-radius:6px;
           overflow-x:auto;font-size:.85em}}
      details summary{{cursor:pointer;color:#007bff;font-weight:bold;margin-top:20px}}
    </style></head>
    <body>
    <h1>Test Report — {meta['name']} {meta['version']}</h1>
    <div class="summary">
      Preset: <strong>{preset_name}</strong> &nbsp;|&nbsp;
      Total: <strong>{total}</strong> &nbsp;|&nbsp;
      <span class="badge {'pass' if ok else 'fail'}">{lbl}</span>
      &nbsp;({passed} passed, {failures} failed)
    </div>
    {tbl}
    <details open><summary>Raw CTest output</summary><pre>{esc}</pre></details>
    </body></html>
    """)
    out.write_text(html, encoding="utf-8")


# ---------------------------------------------------------------------------
# WSL / Docker helpers
# ---------------------------------------------------------------------------

def _check_wsl() -> bool:
    if _HOST == "Linux":
        return True
    wsl = shutil.which("wsl") or shutil.which("wsl.exe")
    if not wsl:
        CONSOLE.print("[red]WSL not found. Install WSL: wsl --install[/]")
        return False
    return True


def _check_docker(meta: dict) -> bool:
    docker = shutil.which("docker")
    if not docker:
        CONSOLE.print("[red]docker not found in PATH. Is Docker Desktop running?[/]")
        return False
    image = meta.get("docker_image", "signal-editor-builder")
    r = subprocess.run(["docker", "image", "inspect", image], capture_output=True, text=True)
    if r.returncode != 0:
        CONSOLE.print(
            f"[bold red]Docker image '[yellow]{image}[/]' not found.[/]\n"
            f"Build it first:\n"
            f"  [bold]docker build -t {image} .[/bold]  (from project root)"
        )
        return False
    return True


def _check_executor(preset: dict, meta: dict) -> bool:
    ex = preset["executor"]
    if ex == "native":
        if preset["host"] == "Windows" and _HOST != "Windows":
            CONSOLE.print("[red]Windows presets can only run on Windows.[/]")
            return False
        return True
    if ex == "wsl":
        return _check_wsl()
    if ex == "docker":
        return _check_docker(meta)
    return True


# ---------------------------------------------------------------------------
# Qt helpers
# ---------------------------------------------------------------------------

def _find_windeployqt(meta: dict) -> Optional[Path]:
    qt_root    = meta.get("qt_root",    "C:/eng_apps/Qt")
    qt_version = meta.get("qt_version", "6.10.1")
    qt_bin     = Path(f"{qt_root}/{qt_version}/mingw_64/bin")

    for candidate in ["windeployqt6.exe", "windeployqt.exe"]:
        p = qt_bin / candidate
        if p.exists():
            return p

    for name in ("windeployqt6", "windeployqt"):
        found = shutil.which(name)
        if found:
            return Path(found)

    return None


def _has_executed_tests(output: str, junit_xml: Optional[Path]) -> bool:
    if junit_xml and junit_xml.exists():
        total, _, _ = _parse_junit(junit_xml)
        if total > 0:
            return True
    return "no tests were found" not in output.lower()


# ---------------------------------------------------------------------------
# Stale cache detection
# ---------------------------------------------------------------------------

def _clear_stale_cache(preset_name: str) -> None:
    """
    If the build directory was generated from a different source directory
    (e.g. after the project was renamed or moved), delete the entire build
    directory so CMake can start fresh.  Deleting only the top-level
    CMakeCache.txt is not enough: FetchContent sub-builds also have their own
    stale CMakeCache.txt files that trigger the same error.
    """
    build_dir = PROJECT_ROOT / "build" / preset_name
    cache     = build_dir / "CMakeCache.txt"
    if not cache.exists():
        return

    source_dir: Optional[str] = None
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
            source_dir = line.split("=", 1)[1].strip()
            break

    if source_dir is None:
        return

    cached_root  = Path(source_dir).resolve()
    current_root = PROJECT_ROOT.resolve()

    if cached_root != current_root:
        CONSOLE.print(
            f"\n[bold yellow]⚠  Stale build directory detected[/]\n"
            f"  Cached source : [dim]{cached_root}[/]\n"
            f"  Current source: [dim]{current_root}[/]\n"
            f"  The build directory was created from a different path.\n"
            f"  Deleting entire build directory to allow a clean reconfigure...",
        )
        shutil.rmtree(build_dir)
        CONSOLE.print("[bold green]✓  Stale build directory removed — CMake will reconfigure from scratch.[/]\n")


# ---------------------------------------------------------------------------
# Core actions
# ---------------------------------------------------------------------------

def action_select_preset(state: dict, meta: dict) -> None:
    CONSOLE.print()
    table = Table(title="All Presets", box=box.ROUNDED, header_style="bold cyan")
    table.add_column("#",              justify="right",  style="bold yellow")
    table.add_column("Preset Name",    style="white")
    table.add_column("Description",    style="dim")
    table.add_column("Arch",           justify="center")
    table.add_column("Type",           justify="center")
    table.add_column("Runner",         justify="center")
    table.add_column("Deps Variant",   style="dim")
    table.add_column("Ready",          justify="center")

    available_indices: list[int] = []
    for idx, p in enumerate(PRESETS, start=1):
        avail  = preset_is_available(p)
        marker = "[bold green]→[/]" if p["name"] == state["preset"] else "  "
        ready  = "[green]✓[/]" if avail else "[red]✗[/]"
        if avail:
            available_indices.append(idx)
        table.add_row(
            f"{marker}{idx}", p["name"], p["display"],
            p["arch"], p["build_type"], executor_label(p),
            p.get("shared_variant", p["name"]), ready,
        )
    CONSOLE.print(table)

    if not available_indices:
        CONSOLE.print("[red]No presets available on this host.[/]")
        return

    CONSOLE.print("[dim]✓ = ready  |  ✗ = unavailable (WSL/Docker not found or Windows-only)[/]")
    choice = Prompt.ask(
        "Select preset number",
        choices=[str(i) for i in range(1, len(PRESETS) + 1)],
        default=str(available_indices[0]),
    )
    chosen = PRESETS[int(choice) - 1]
    if not preset_is_available(chosen):
        CONSOLE.print(f"[bold yellow]⚠ Preset '{chosen['name']}' is marked as unavailable — "
                      "proceeding anyway.[/]")
    state["preset"] = chosen["name"]
    CONSOLE.print(f"[bold green]✓[/] Preset set to [bold]{state['preset']}[/]"
                  f" (executor: {chosen['executor']},"
                  f" deps-variant: {chosen.get('shared_variant', chosen['name'])})")


def _get_preset(name: str) -> dict:
    for p in PRESETS:
        if p["name"] == name:
            return p
    raise KeyError(name)


def action_configure(state: dict, cmake: str, meta: dict) -> bool:
    preset = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Configure — {state['preset']} [{preset['executor']}]"))
    if not _check_executor(preset, meta):
        return False

    # Remove stale cache before anything else (catches project renames/moves)
    _clear_stale_cache(state["preset"])

    # ── Dependency check before invoking cmake ────────────────────────────────
    CONSOLE.print("[dim]Checking dependencies...[/]")
    if not _validate_dependencies(preset, meta):
        return False
    CONSOLE.print("[bold green]✓ All dependencies verified[/]")

    # Pass the shared-lib variant and external-deps flag as cmake cache variables.
    shared_variant  = preset.get("shared_variant", preset["name"])
    use_external    = meta.get("use_external_deps", False)
    extra_args      = [
        f"-DSIGNAL_EDITOR_DEPS_VARIANT={shared_variant}",
        f"-DSIGNAL_EDITOR_USE_EXTERNAL_DEPS={'ON' if use_external else 'OFF'}",
        "-DSIGNAL_EDITOR_BUILD_GUI=ON",
    ]

    base = [cmake, "--preset", state["preset"]] + extra_args
    if preset["executor"] in ("wsl", "docker"):
        base = ["cmake", "--preset", state["preset"]] + extra_args
    cmd  = build_cmd(preset, base, meta)
    code = run(cmd, preset_name=state["preset"] if preset["executor"] == "native" else None)
    if code == 0:
        CONSOLE.print("[bold green]✓ Configure succeeded[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Configure failed (exit {code})[/]")
    return code == 0


def action_build(state: dict, cmake: str, meta: dict) -> bool:
    preset = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Build — {state['preset']} [{preset['executor']}]"))
    if not _check_executor(preset, meta):
        return False
    base = [cmake, "--build", "--preset", state["preset"]]
    if preset["executor"] in ("wsl", "docker"):
        base = ["cmake", "--build", "--preset", state["preset"]]
    cmd  = build_cmd(preset, base, meta)
    code = run(cmd, preset_name=state["preset"] if preset["executor"] == "native" else None)
    if code == 0:
        CONSOLE.print("[bold green]✓ Build succeeded[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Build failed (exit {code})[/]")
    return code == 0


def action_clean(state: dict) -> None:
    build_dir = PROJECT_ROOT / "build" / state["preset"]
    if not build_dir.exists():
        CONSOLE.print(f"[yellow]Build directory not found: {build_dir}[/]")
        return
    if Confirm.ask(f"Delete [bold]{build_dir}[/]?", default=False):
        shutil.rmtree(build_dir)
        CONSOLE.print(f"[bold green]✓[/] Deleted {build_dir}")
    else:
        CONSOLE.print("[dim]Cancelled.[/]")


def action_clean_all() -> None:
    build_root = PROJECT_ROOT / "build"
    if not build_root.exists() or not any(build_root.iterdir()):
        CONSOLE.print("[yellow]Build directory is already empty.[/]")
        return
    subdirs = [d for d in build_root.iterdir() if d.is_dir()]
    for d in subdirs:
        CONSOLE.print(f"  [dim]{d}[/]")
    if Confirm.ask("Delete ALL build directories?", default=False):
        shutil.rmtree(build_root)
        CONSOLE.print("[bold green]✓[/] All build directories deleted.")
    else:
        CONSOLE.print("[dim]Cancelled.[/]")


def action_run_tests(state: dict, ctest: str, meta: dict) -> None:
    preset    = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Tests — {state['preset']} [{preset['executor']}]"))
    build_dir = PROJECT_ROOT / "build" / state["preset"]
    if not build_dir.exists():
        CONSOLE.print("[red]Build directory not found. Configure + Build first.[/]")
        return
    if not _check_executor(preset, meta):
        return

    junit_xml   = build_dir / "test_results.xml"
    html_report = build_dir / "test_results.html"

    base = ["ctest", "--preset", state["preset"],
            "-V", "--output-on-failure", "--output-junit", str(junit_xml)]
    if preset["executor"] == "native":
        base[0] = ctest
    elif preset["executor"] == "wsl":
        wsl_junit = _win_to_wsl_path(junit_xml) if _HOST == "Windows" else str(junit_xml)
        base      = ["ctest", "--preset", state["preset"],
                     "-V", "--output-on-failure", "--output-junit", wsl_junit]
    elif preset["executor"] == "docker":
        docker_junit = f"/workspace/build/{state['preset']}/test_results.xml"
        base         = ["ctest", "--preset", state["preset"],
                        "-V", "--output-on-failure", "--output-junit", docker_junit]

    cmd   = build_cmd(preset, base, meta)
    pname = state["preset"] if preset["executor"] == "native" else None
    code, output = run_capture(cmd, preset_name=pname)
    CONSOLE.print(output)

    executed_tests = _has_executed_tests(output, junit_xml)
    if executed_tests:
        _generate_html(html_report, output, junit_xml, state["preset"], meta)
        CONSOLE.print(f"\n[bold]HTML report:[/] {html_report}")
        if Confirm.ask("Open HTML report in browser?", default=True):
            webbrowser.open(html_report.as_uri())
    else:
        CONSOLE.print("[yellow]No tests were discovered — HTML report skipped.[/]")

    if code == 0 and executed_tests:
        CONSOLE.print("[bold green]✓ All tests passed[/]")
    elif code == 0:
        CONSOLE.print("[yellow]No tests were executed.[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Some tests failed (exit {code})[/]")


_MINGW_WINPTHREAD = [
    "C:/eng_apps/msys64/mingw64/bin/libwinpthread-1.dll",
    "C:/eng_apps/msys64/mingw32/bin/libwinpthread-1.dll",
]

_MINGW_LIBZIP_RUNTIME_DLLS = [
    "libzip.dll",
    "zlib1.dll",
    "liblzma-5.dll",
    "libbz2-1.dll",
]


def _candidate_mingw_bin_dirs(preset_name: str) -> list[Path]:
    candidates: list[Path] = []
    if "mingw64" in preset_name:
        candidates.append(Path("C:/eng_apps/msys64/mingw64/bin"))
    if "mingw32" in preset_name:
        candidates.append(Path("C:/eng_apps/msys64/mingw32/bin"))
    candidates.extend([
        Path("C:/eng_apps/msys64/mingw64/bin"),
        Path("C:/eng_apps/msys64/mingw32/bin"),
    ])

    unique_candidates: list[Path] = []
    seen: set[str] = set()
    for candidate in candidates:
        key = str(candidate).lower()
        if key in seen:
            continue
        seen.add(key)
        unique_candidates.append(candidate)
    return unique_candidates


def _copy_mingw_runtime(preset_name: str, bin_dir: Path) -> None:
    if _HOST != "Windows":
        return
    src: Optional[Path] = None
    for c in _MINGW_WINPTHREAD:
        p = Path(c)
        if not p.exists():
            continue
        if "mingw64" in preset_name and "mingw64" in str(p):
            src = p
            break
        if "mingw32" in preset_name and "mingw32" in str(p):
            src = p
            break
    if src is None:
        for c in _MINGW_WINPTHREAD:
            p = Path(c)
            if p.exists():
                src = p
                break
    if src is None:
        CONSOLE.print("[yellow]  libwinpthread-1.dll not found — add it manually if needed.[/]")
        return
    dst = bin_dir / "libwinpthread-1.dll"
    shutil.copy2(src, dst)
    CONSOLE.print(f"  [dim]Bundled:[/] {dst.name} ← {src}")


def _copy_mingw_libzip_runtime(preset_name: str, bin_dir: Path) -> set[str]:
    if _HOST != "Windows":
        return set()

    copied: set[str] = set()
    search_dirs = [d for d in _candidate_mingw_bin_dirs(preset_name) if d.exists()]
    if not search_dirs:
        CONSOLE.print("[yellow]  MinGW bin directory not found — libzip runtime was not bundled.[/]")
        return copied

    missing: list[str] = []
    for dll_name in _MINGW_LIBZIP_RUNTIME_DLLS:
        src: Optional[Path] = None
        for directory in search_dirs:
            candidate = directory / dll_name
            if candidate.exists():
                src = candidate
                break
        if src is None:
            missing.append(dll_name)
            continue

        shutil.copy2(src, bin_dir / dll_name)
        copied.add(dll_name)
        CONSOLE.print(f"  [dim]Bundled:[/] {dll_name} ← {src}")

    if missing:
        CONSOLE.print(
            "[yellow]  Missing optional libzip runtime DLL(s): "
            + ", ".join(missing)
            + "[/]"
        )

    return copied


def _resolve_app_deploy_dir(state: dict, meta: dict) -> Path:
    configured = meta.get("deploy_app_folder") or meta.get("app_deploy_folder")
    if configured:
        root = Path(configured)
        if not root.is_absolute():
            root = PROJECT_ROOT / root
        return root / state["preset"]
    return PROJECT_ROOT / "deploy" / "app" / state["preset"]


def action_deploy_app(state: dict, cmake: str, meta: dict) -> None:
    """Deploy the GUI executable with windeployqt to deploy/app/<preset>/."""
    CONSOLE.print(Rule(f"[bold cyan]Deploy App — {state['preset']}"))

    preset    = _get_preset(state["preset"])
    build_dir = PROJECT_ROOT / "build" / state["preset"]
    bin_dir   = build_dir / "bin"

    if not bin_dir.exists():
        CONSOLE.print(f"[red]Bin directory not found: {bin_dir}[/]")
        CONSOLE.print("[dim]Configure + Build first.[/]")
        return

    if _HOST != "Windows":
        CONSOLE.print("[yellow]windeployqt is Windows-only. Skipping Qt DLL bundling.[/]")
        return

    windeployqt = _find_windeployqt(meta)
    if windeployqt is None:
        qt_root    = meta.get("qt_root",    "C:/eng_apps/Qt")
        qt_version = meta.get("qt_version", "6.10.1")
        CONSOLE.print(
            f"[red]windeployqt not found.[/]\n"
            f"Expected at: {qt_root}/{qt_version}/mingw_64/bin/windeployqt6.exe\n"
            f"Check qt_root and qt_version in project.json."
        )
        return

    exe = bin_dir / "signal-editor-gui.exe"
    if not exe.exists():
        CONSOLE.print(f"[red]Executable not found: {exe}[/]")
        return

    out_dir = _resolve_app_deploy_dir(state, meta)
    if not Confirm.ask(f"Deploy to [bold]{out_dir}[/]?", default=True):
        CONSOLE.print("[dim]Cancelled.[/]")
        return

    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # Copy exe
    shutil.copy2(exe, out_dir / exe.name)

    # Copy runtime DLLs produced by this project build itself.
    # This covers shared libraries such as libsignal_editor_core.dll that the
    # executable depends on but that are not part of shared_lib/ or Qt.
    project_runtime_dlls: set[str] = set()
    for dll in sorted(bin_dir.glob("*.dll")):
        shutil.copy2(dll, out_dir / dll.name)
        project_runtime_dlls.add(dll.name)
        CONSOLE.print(f"  [dim]Project runtime:[/] {dll.name}")

    # Copy declared dependency DLLs from shared_lib
    shared_lib_base = Path(meta.get("shared_lib_base", ""))
    shared_variant  = preset.get("shared_variant", preset["name"])
    copied_dependency_dlls: set[str] = set()
    for dep in meta.get("dependencies", []):
        dll = (shared_lib_base / dep["name"] / dep["version"] /
               shared_variant / "bin" / f"{dep['name']}.dll")
        if dll.exists():
            shutil.copy2(dll, out_dir / dll.name)
            copied_dependency_dlls.add(dll.name)
            CONSOLE.print(f"  [dim]Dependency:[/] {dll.name}")
        else:
            CONSOLE.print(f"  [yellow]Missing dependency DLL:[/] {dll}")

    # windeployqt
    cmd  = [
        str(windeployqt),
        "--dir", str(out_dir),
        "--compiler-runtime",
        str(out_dir / exe.name),
    ]
    code = run(cmd, preset_name=state["preset"])
    if code == 0:
        CONSOLE.print("[green]✓ Qt DLLs bundled[/]")
    else:
        CONSOLE.print(f"[red]✗ windeployqt failed (exit {code})[/]")
        return

    # Copy translations
    out_trans = out_dir / "translations"
    out_trans.mkdir(exist_ok=True)
    qm_candidates = []
    bin_translations = build_dir / "bin" / "translations"
    if bin_translations.exists():
        qm_candidates.extend(sorted(bin_translations.glob("*.qm")))
    else:
        qm_candidates.extend(sorted(build_dir.glob("apps/signal_editor/gui/*.qm")))
    for qm in qm_candidates:
        shutil.copy2(qm, out_trans / qm.name)
        CONSOLE.print(f"  [dim]Translation:[/] {qm.name}")

    _copy_mingw_runtime(state["preset"], out_dir)
    bundled_libzip_runtime = _copy_mingw_libzip_runtime(state["preset"], out_dir)

    deployed_files = {p.name for p in out_dir.iterdir() if p.is_file()}
    missing_dependencies = sorted(name for name in copied_dependency_dlls if name not in deployed_files)
    if missing_dependencies:
        CONSOLE.print(
            f"[red]✗ Deploy incomplete — missing dependency DLL(s): {', '.join(missing_dependencies)}[/]"
        )
        return

    missing_project_runtime = sorted(name for name in project_runtime_dlls if name not in deployed_files)
    if missing_project_runtime:
        CONSOLE.print(
            f"[red]✗ Deploy incomplete — missing project runtime DLL(s): "
            f"{', '.join(missing_project_runtime)}[/]"
        )
        return

    missing_libzip_runtime = sorted(name for name in bundled_libzip_runtime if name not in deployed_files)
    if missing_libzip_runtime:
        CONSOLE.print(
            f"[red]✗ Deploy incomplete — missing libzip runtime DLL(s): "
            f"{', '.join(missing_libzip_runtime)}[/]"
        )
        return

    CONSOLE.print(f"\n[bold green]✓ App deployed to:[/] {out_dir}")


def action_docker_build_image(meta: dict) -> None:
    CONSOLE.print(Rule("[bold cyan]Build Docker image"))
    image      = meta.get("docker_image", "signal-editor-builder")
    dockerfile = PROJECT_ROOT / "Dockerfile.ubuntu"
    if not dockerfile.exists():
        dockerfile = PROJECT_ROOT / "Dockerfile.alpine"
    if not dockerfile.exists():
        CONSOLE.print("[red]Dockerfile not found at project root.[/]")
        return
    CONSOLE.print(f"Building image [bold]{image}[/] from {dockerfile} ...")
    code = run(["docker", "build", "-f", str(dockerfile), "-t", image, str(PROJECT_ROOT)])
    if code == 0:
        CONSOLE.print(f"[bold green]✓ Image '{image}' built.[/]")
    else:
        CONSOLE.print(f"[bold red]✗ docker build failed (exit {code})[/]")


def _print_tree(prefix: Path) -> None:
    table = Table(box=box.SIMPLE, header_style="bold magenta", show_header=True)
    table.add_column("Path",  style="white")
    table.add_column("Size",  justify="right", style="dim")
    for p in sorted(prefix.rglob("*")):
        if p.is_file():
            rel = p.relative_to(prefix)
            sz  = p.stat().st_size
            table.add_row(
                str(rel),
                f"{sz/1024:.1f} KB" if sz >= 1024 else f"{sz} B"
            )
    CONSOLE.print(table)


# ---------------------------------------------------------------------------
# Menu / header
# ---------------------------------------------------------------------------

def _executor_for_current(name: str) -> str:
    try:
        p = _get_preset(name)
        if p["executor"] == "native":
            return "native"
        if p["executor"] == "wsl":
            return "WSL" if _HOST == "Windows" else "native"
        return p["executor"]
    except KeyError:
        return "?"


def print_header(state: dict, meta: dict) -> None:
    try:
        preset = _get_preset(state["preset"])
        variant = preset.get("shared_variant", state["preset"])
    except KeyError:
        variant = "?"

    CONSOLE.print()
    CONSOLE.print(Panel(
        f"[bold white]{meta['name']}[/]  [dim]v{meta['version']}[/]\n"
        f"{meta.get('description', '')}\n\n"
        f"Preset       : [bold yellow]{state['preset']}[/]  "
        f"([italic]{_executor_for_current(state['preset'])}[/italic])\n"
        f"Deps variant : [dim]{variant}[/]\n"
        f"Build dir    : [dim]{PROJECT_ROOT / 'build' / state['preset']}[/]",
        title="[bold cyan]Signal Editor  —  Project Manager[/]",
        border_style="cyan",
        expand=False,
    ))


def print_menu() -> None:
    txt = (
        "[bold cyan]Preset[/]\n"
        "  [bold yellow][1][/] Select Preset\n\n"
        "[bold cyan]Build[/]\n"
        "  [bold yellow][2][/] Configure  (validates deps first)\n"
        "  [bold yellow][3][/] Build\n"
        "  [bold yellow][4][/] Clean current preset\n"
        "  [bold yellow][5][/] Clean ALL presets\n\n"
        "[bold cyan]Test[/]\n"
        "  [bold yellow][6][/] Run Tests + HTML report\n\n"
        "[bold cyan]Deploy[/]\n"
        "  [bold yellow][7][/] Deploy app  (deploy/app/ + windeployqt)\n\n"
        "[bold cyan]Docker[/]\n"
        "  [bold yellow][8][/] Build Docker image\n\n"
        "[bold cyan]Exit[/]\n"
        "  [bold yellow][0][/] Quit"
    )
    CONSOLE.print(Panel(txt, title="Menu", border_style="dim white", expand=False))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    meta  = load_metadata()
    cmake = find_cmake()
    ctest = find_ctest()

    if not cmake:
        CONSOLE.print(
            "[bold red]cmake not found.[/]\n"
            "Add CMake to PATH or install at C:/eng_apps/cmake/cmake-4.2.0/"
        )
        sys.exit(1)

    CONSOLE.print(f"[dim]cmake  : {cmake}[/]")
    CONSOLE.print(f"[dim]ctest  : {ctest or 'NOT FOUND'}[/]")
    CONSOLE.print(f"[dim]host   : {_HOST}[/]")
    docker_ok = shutil.which("docker") is not None
    CONSOLE.print(f"[dim]docker : {'available' if docker_ok else 'not found'}[/]")
    wsl_ok    = shutil.which("wsl") is not None or shutil.which("wsl.exe") is not None
    CONSOLE.print(f"[dim]wsl    : {'available' if wsl_ok or _HOST == 'Linux' else 'not found'}[/]")

    qt_root     = meta.get("qt_root",    "C:/eng_apps/Qt")
    qt_version  = meta.get("qt_version", "?")
    windeployqt = _find_windeployqt(meta)
    CONSOLE.print(f"[dim]Qt     : {qt_root}/{qt_version} "
                  f"({'windeployqt found' if windeployqt else 'windeployqt NOT found'})[/]")

    default_preset = "windows-mingw64-debug" if _HOST == "Windows" else "linux-gcc-debug"
    state: dict    = {"preset": default_preset}

    while True:
        print_header(state, meta)
        print_menu()
        choice = Prompt.ask(
            "[bold]Choice",
            choices=["0", "1", "2", "3", "4", "5", "6", "7", "8"],
            default="0",
        )
        if choice == "0":
            CONSOLE.print("[bold]Bye![/]")
            break
        elif choice == "1":
            action_select_preset(state, meta)
        elif choice == "2":
            action_configure(state, cmake, meta)
        elif choice == "3":
            action_build(state, cmake, meta)
        elif choice == "4":
            action_clean(state)
        elif choice == "5":
            action_clean_all()
        elif choice == "6":
            if not ctest and _get_preset(state["preset"])["executor"] == "native":
                CONSOLE.print("[red]ctest not found.[/]")
            else:
                action_run_tests(state, ctest or "ctest", meta)
        elif choice == "7":
            action_deploy_app(state, cmake, meta)
        elif choice == "8":
            action_docker_build_image(meta)

        CONSOLE.print()
        input("Press Enter to continue...")


if __name__ == "__main__":
    main()
