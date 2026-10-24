#!/usr/bin/env python3
"""Lightweight, dependency-free repository-skill consistency check.

This is not a YAML parser or a complete Agent Skills conformance suite.
It enforces the intentionally simple one-line frontmatter values used
by this skill, verifies local references and the public-header includes
from the sample consumer, and catches accidental file moves early.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    repo = root.parents[2]
    skill_file = root / "SKILL.md"
    if not skill_file.is_file():
        raise ValueError(f"Required manifest missing: {skill_file}")

    source = skill_file.read_text(encoding="utf-8")
    match = re.match(r"\A---\r?\n(.*?)\r?\n---\r?\n", source, re.DOTALL)
    if not match:
        raise ValueError("SKILL.md must begin with YAML frontmatter delimited by --- lines")

    fields = {}
    for line in match.group(1).splitlines():
        key, separator, value = line.partition(":")
        if not separator or not value.strip() or key in fields:
            raise ValueError(f"Unsupported/duplicate/empty frontmatter field: {line!r}")
        fields[key] = value.strip()

    name = fields.get("name", "")
    if not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", name):
        raise ValueError("Skill name must use lowercase alphanumerics and single hyphens")
    if name != root.name or len(name) > 64:
        raise ValueError("Skill frontmatter name must match the enclosing directory, <=64 chars")

    description = fields.get("description", "")
    if not (1 <= len(description) <= 1024):
        raise ValueError("Skill description must contain 1..1024 characters")

    declared = re.findall(r"\]\((references/[^)#]+|examples/[^)#]+)\)", source)
    if not declared:
        raise ValueError("SKILL.md should link to at least one local reference")
    for relative in declared:
        if not (root / relative).is_file():
            raise ValueError(f"Broken skill-relative link: {relative}")

    sample = root / "examples" / "core-consumer" / "main.cpp"
    cmake = root / "examples" / "core-consumer" / "CMakeLists.txt"
    if not cmake.is_file() or not sample.is_file():
        raise ValueError("Missing compiled sample consumer CMakeLists/main.cpp")
    cpp = sample.read_text(encoding="utf-8")
    for header in re.findall(r"#include <(sasd/ui/[^>]+)>", cpp):
        if not (repo / "include" / header).is_file():
            raise ValueError(f"Consumer uses missing toolkit header: {header}")

    for path in (
        "AGENTS.md", "CMakeLists.txt", "examples/terminal_form_demo.cpp",
        "cmake/SASDUIToolkitConfig.cmake.in",
    ):
        if not (repo / path).is_file():
            raise ValueError(f"Missing canonical repository reference: {path}")

    print(f"Skill manifest and {len(declared)} local links OK; "
          f"{len(re.findall(r'#include <sasd/ui/', cpp))} public headers exist")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as exc:
        print(f"AI consumer skill validation failed: {exc}", file=sys.stderr)
        sys.exit(1)
