#!/usr/bin/env python3
"""
Extract power contributions for selected hierarchical scopes from a
-hierarchy power report (e.g. Synopsys Power Compiler).

The script:
  - Keeps the original report header
  - Selects only the specified scopes and all their children
  - Writes a new report file without overwriting existing ones
"""

from pathlib import Path
import sys

# -------------------- configuration --------------------

INPUT_FILE = "crheepto_top_hier.rpt"
OUTPUT_BASENAME = "crheepto_top_power.rpt"

# Scopes of interest (full logical hierarchy, last component is matched)
TARGET_SCOPES = {
    "cpu_subsystem_i",
    "memory_subsystem_i",
    "system_bus_i",
    "coproc_wrapper_i",
}

# -------------------- helpers --------------------

def next_available_filename(base: Path) -> Path:
    """
    Return a non-existing filename by appending _N if needed.
    """
    if not base.exists():
        return base

    stem = base.stem
    suffix = base.suffix
    parent = base.parent

    i = 1
    while True:
        candidate = parent / f"{stem}_{i}{suffix}"
        if not candidate.exists():
            return candidate
        i += 1


def leading_spaces(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


# -------------------- main logic --------------------

def main():
    in_path = Path(INPUT_FILE)
    if not in_path.exists():
        print(f"Error: input file '{INPUT_FILE}' not found", file=sys.stderr)
        sys.exit(1)

    out_path = next_available_filename(Path(OUTPUT_BASENAME))

    with in_path.open() as f:
        lines = f.readlines()

    output_lines = []

    header_done = False
    active_indent = None

    for line in lines:
        # Always copy header and separators
        if not header_done:
            output_lines.append(line)
            if line.strip().startswith("---"):
                header_done = True
            continue

        if not line.strip():
            continue

        indent = leading_spaces(line)
        stripped = line.strip()

        # Check if this line starts a target scope
        scope_name = stripped.split()[0]
        if scope_name in TARGET_SCOPES:
            active_indent = indent
            output_lines.append(line)
            continue

        # If inside a selected scope, keep all deeper levels
        if active_indent is not None:
            if indent > active_indent:
                output_lines.append(line)
                continue
            else:
                active_indent = None

    with out_path.open("w") as f:
        f.writelines(output_lines)

    print(f"Generated: {out_path}")


if __name__ == "__main__":
    main()