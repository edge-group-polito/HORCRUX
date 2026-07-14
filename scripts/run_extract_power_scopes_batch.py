#!/usr/bin/env python3
"""
Run scripts/extract_power_scopes.py across multiple subfolders.
"""

import argparse
from pathlib import Path
import subprocess
import sys
from typing import Tuple


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Execute extract_power_scopes.py in a set of subfolders."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path.cwd(),
        help="Root directory containing the subfolders (default: current directory).",
    )
    parser.add_argument(
        "--folders",
        nargs="+",
        help="Subfolder names relative to --root. If omitted, immediate subfolders starting with 'reports_' are used.",
    )
    parser.add_argument(
        "--script",
        type=Path,
        default=Path(__file__).with_name("extract_power_scopes.py"),
        help="Path to extract_power_scopes.py (default: scripts/extract_power_scopes.py).",
    )
    parser.add_argument(
        "--python",
        default=sys.executable,
        help="Python executable to use (default: current interpreter).",
    )
    parser.add_argument(
        "--continue-on-error",
        action="store_true",
        help="Continue running other folders if one fails.",
    )
    return parser.parse_args()


def run_one_folder(script_path: Path, folder: Path, python_exe: str) -> Tuple[bool, str]:
    if not folder.is_dir():
        return False, f"[FAIL] {folder}: folder does not exist"

    result = subprocess.run(
        [python_exe, str(script_path)],
        cwd=folder,
        text=True,
        capture_output=True,
    )

    if result.returncode == 0:
        output = result.stdout.strip() or "(no output)"
        return True, f"[ OK ] {folder}: {output}"

    err = result.stderr.strip() or result.stdout.strip() or "(no output)"
    return False, f"[FAIL] {folder}: {err}"


def main() -> int:
    args = parse_args()

    root = args.root.resolve()
    script_path = args.script.resolve()

    if not root.exists():
        print(f"Error: root directory not found: {root}", file=sys.stderr)
        print(
            "Hint: if you run from repository root, use "
            "--root implementation/power_analysis",
            file=sys.stderr,
        )
        return 1
    if not root.is_dir():
        print(f"Error: root is not a directory: {root}", file=sys.stderr)
        return 1

    if args.folders:
        folders = [root / subfolder for subfolder in args.folders]
    else:
        folders = sorted(
            path for path in root.iterdir() if path.is_dir() and path.name.startswith("reports_")
        )

    if not script_path.exists():
        print(f"Error: script not found: {script_path}", file=sys.stderr)
        return 1

    if not folders:
        print(f"Error: no matching subfolders found under {root}", file=sys.stderr)
        return 1

    failed = False
    for folder in folders:
        ok, message = run_one_folder(script_path=script_path, folder=folder, python_exe=args.python)
        print(message)
        if not ok:
            failed = True
            if not args.continue_on_error:
                break

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
