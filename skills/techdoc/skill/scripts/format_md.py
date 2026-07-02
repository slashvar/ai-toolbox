#!/usr/bin/env python3
"""Mechanical Markdown hygiene for the techdoc skill.

Enforces the deterministic rules from STYLE_RULES.md that a model should not
hand-apply: aligned table separators/cells, no trailing whitespace, and
(in check mode) bare code fences.

Stdlib only — no external dependency, so the skill stays portable.

Usage:
    format_md.py FILE ...            # rewrite files in place (align + strip)
    format_md.py --check FILE ...    # report issues, exit 1 if any, no write

The formatter is idempotent: running it on already-formatted output is a no-op.
"""

import argparse
import re
import sys

SEP_CELL = re.compile(r":?-+:?")


def is_row(line):
    return line.lstrip().startswith("|")


def split_cells(line):
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|"):
        s = s[:-1]
    return [c.strip() for c in s.split("|")]


def is_sep(cells):
    return any(c for c in cells) and all(
        SEP_CELL.fullmatch(c) for c in cells if c
    )


def align_block(block):
    """Return the aligned lines for one contiguous table block."""
    rows = [split_cells(l) for l in block]
    ncol = max(len(r) for r in rows)
    rows = [r + [""] * (ncol - len(r)) for r in rows]
    seps = [is_sep(r) for r in rows]

    widths = [0] * ncol
    for r, sep in zip(rows, seps):
        if sep:
            continue
        for k, c in enumerate(r):
            widths[k] = max(widths[k], len(c))

    out = []
    for r, sep in zip(rows, seps):
        if sep:
            out.append(
                "|" + "|".join("-" * (widths[k] + 2) for k in range(ncol)) + "|"
            )
        else:
            out.append(
                "| " + " | ".join(r[k].ljust(widths[k]) for k in range(ncol)) + " |"
            )
    return out


def format_lines(lines):
    out = []
    i = 0
    while i < len(lines):
        if is_row(lines[i]):
            j = i
            while j < len(lines) and is_row(lines[j]):
                j += 1
            out.extend(align_block(lines[i:j]))
            i = j
        else:
            out.append(lines[i].rstrip())
            i += 1
    return out


def format_text(text):
    return "\n".join(format_lines(text.split("\n")))


def find_issues(text):
    """Return a list of human-readable hygiene problems (check mode)."""
    issues = []
    lines = text.split("\n")
    if format_lines(lines) != lines:
        issues.append("tables not aligned or trailing whitespace present")
    in_fence = False
    for n, line in enumerate(lines, 1):
        stripped = line.strip()
        if stripped.startswith("```"):
            if not in_fence and stripped == "```":
                issues.append(f"line {n}: code fence has no language")
            in_fence = not in_fence
    return issues


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("files", nargs="+")
    ap.add_argument(
        "--check",
        action="store_true",
        help="report issues and exit 1 if any; do not write",
    )
    args = ap.parse_args(argv)

    failed = False
    for path in args.files:
        with open(path) as f:
            text = f.read()
        if args.check:
            issues = find_issues(text)
            if issues:
                failed = True
                print(f"{path}:")
                for issue in issues:
                    print(f"  - {issue}")
        else:
            formatted = format_text(text)
            if formatted != text:
                with open(path, "w") as f:
                    f.write(formatted)
                print(f"formatted: {path}")
            else:
                print(f"clean: {path}")

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
